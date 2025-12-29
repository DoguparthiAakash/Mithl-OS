#include "fat32.h"
#include "kernel.h"
#include "memory.h"
#include "string.h"
#include "console.h"
#include "ata.h"

// Hardcoded for now: Partition starts at LBA 0 (Superfloppy)
#define PARTITION_LBA_OFFSET 0

static fat32_fs_t fat_fs;
static uint32_t fat_lba_start = 0;

// Wrapper for ATA
// TODO: Replace with generic Block Device Interface
// function decl removed

// extern void ata_write_sector(uint32_t lba, uint8_t *buffer);

static void disk_read(uint32_t lba, uint8_t *buffer) {
    ata_read_sector(fat_lba_start + lba, buffer);
}

// Helper: Read a cluster
// Note: Only supports single sector clusters for simplicity in this V1, or requires loop
static void read_cluster(uint32_t cluster, uint8_t *buffer) {
    uint32_t lba = fat_fs.data_start_lba + ((cluster - 2) * fat_fs.sectors_per_cluster);
    for (uint32_t i = 0; i < fat_fs.sectors_per_cluster; i++) {
        ata_read_sector(fat_lba_start + lba + i, buffer + (i * 512));
    }
}

// Get next cluster from FAT table
static uint32_t get_next_cluster(uint32_t cluster) {
    uint32_t fat_sector = fat_fs.fat_start_lba + ((cluster * 4) / 512);
    uint32_t fat_offset = (cluster * 4) % 512;
    
    uint8_t buffer[512];
    ata_read_sector(fat_lba_start + fat_sector, buffer);
    
    uint32_t next_cluster = *(uint32_t*)&buffer[fat_offset];
    return next_cluster & 0x0FFFFFFF; // Mask top 4 bits
}

// VFS Hooks
static struct dirent dir_entry_ret; // Static buffer for return (from vfs.h dirent, simplified)
// Note: standard dirent struct definition needed. MITHL-OS vfs.h defines struct dirent { char name[128]; uint32_t ino; };

fs_node_t *fat32_finddir(fs_node_t *node, char *name);
struct dirent *fat32_readdir(fs_node_t *node, uint32_t index);
uint32_t fat32_read(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);

void fat32_init(uint32_t partition_lba) {
    fat_lba_start = partition_lba;
    
    uint8_t *bpb_buffer = (uint8_t*)memory_alloc(512);
    disk_read(0, bpb_buffer); // Read Partition Boot Sector
    
    fat32_bpb_t *bpb = (fat32_bpb_t*)bpb_buffer;
    
    if (bpb->boot_signature != 0x29 && bpb->boot_signature != 0x28) {
        // Warning: Might be 0x00 on some formatted disks? FAT check usually looks for JMP or BytesPerSector
    }
    
    fat_fs.partition_offset = partition_lba;
    fat_fs.sectors_per_cluster = bpb->sectors_per_cluster;
    fat_fs.bytes_per_cluster = bpb->sectors_per_cluster * 512;
    fat_fs.fat_start_lba = bpb->reserved_sectors;
    fat_fs.root_cluster = bpb->root_cluster;
    
    // Calculate Data Start
    // DataStart = Reserved + (FatCount * SectorsPerFAT)
    uint32_t fat_size = bpb->sectors_per_fat_32;
    fat_fs.data_start_lba = bpb->reserved_sectors + (bpb->fat_count * fat_size);
    
    serial_write("[FAT32] Initialized.\n");
    serial_write("  Sec/Clust: ");
    // print hex bpb->sectors_per_cluster
    serial_write("\n");
}

// VFS 2.0 Mount Callback
fs_node_t *fat32_mount_fs(const char *source, const char *target) {
    (void)target;
    // Parse source for partition LBA or device?
    // For now, hardcode partition 0 if source is "disk" or similar
    // fat32_init(0); // Assuming already initialized or init here?
    // Actually, mount usually initializes per instance.
    // We'll init strict LBA 0 for now.
    
    // fat32_init(0); // If we call this, we re-read BPB.
    // It's safe.
    // But `fat32_init` is void.
    
    // Existing fat32_mount returns the node.
    fs_node_t *root = (fs_node_t*)memory_alloc(sizeof(fs_node_t));
    strcpy(root->name, "fat_root");
    root->flags = FS_DIRECTORY;
    root->inode = fat_fs.root_cluster; // Use Cluster as Inode
    root->readdir = fat32_readdir;
    root->finddir = fat32_finddir;
    root->read = 0; 
    root->write = 0;
    root->open = 0;
    root->close = 0;
    return root;
}

// Wrapper to match signature (fat32_mount was name, let's keep it or rename?)
// fat32.h says fs_node_t *fat32_mount(void);
// We will change header later or just add wrapper.

void fat32_register(void) {
    vfs_register_driver("fat32", fat32_mount_fs);
}


// READ DIRECTORY
struct dirent *fat32_readdir(fs_node_t *node, uint32_t index) {
    // This is tricky. Index 0 = First Entry.
    // We need to iterate clusters.
    
    uint32_t cluster = node->inode;
    uint32_t entry_count = 0;
    uint8_t *cluster_buf = (uint8_t*)memory_alloc(fat_fs.bytes_per_cluster);
    
    while (cluster < 0x0FFFFFF8) {
        read_cluster(cluster, cluster_buf);
        
        uint32_t entries_per_cluster = fat_fs.bytes_per_cluster / 32;
        
        for (uint32_t i = 0; i < entries_per_cluster; i++) {
             fat_dir_entry_t *entry = (fat_dir_entry_t*)(cluster_buf + (i * 32));
             
             if ((uint8_t)entry->name[0] == 0x00) break; // End of Dir
             if ((uint8_t)entry->name[0] == 0xE5) continue; // Deleted
             if (entry->attr & ATTR_LONG_NAME) continue; // Skip LFN for now
             if (entry->attr & ATTR_VOLUME_ID) continue;
             
             if (entry_count == index) {
                 // Found it
                 // Format Name (8.3)
                 int j=0;
                 for (int k=0; k<8; k++) {
                     if (entry->name[k] != ' ') dir_entry_ret.name[j++] = entry->name[k];
                 }
                 if (entry->ext[0] != ' ') {
                     dir_entry_ret.name[j++] = '.';
                     for (int k=0; k<3; k++) {
                         if (entry->ext[k] != ' ') dir_entry_ret.name[j++] = entry->ext[k];
                     }
                 }
                 dir_entry_ret.name[j] = 0;
                 dir_entry_ret.ino = (entry->cluster_high << 16) | entry->cluster_low;
                 
                 memory_free(cluster_buf);
                 return &dir_entry_ret;
             }
             entry_count++;
        }
        
        cluster = get_next_cluster(cluster);
    }
    
    memory_free(cluster_buf);
    return 0;
}

fs_node_t *fat32_finddir(fs_node_t *node, char *name) {
    // Iterate until name matches
    uint32_t cluster = node->inode;
    uint8_t *cluster_buf = (uint8_t*)memory_alloc(fat_fs.bytes_per_cluster);
    
    while (cluster < 0x0FFFFFF8) {
        read_cluster(cluster, cluster_buf);
        uint32_t entries_per_cluster = fat_fs.bytes_per_cluster / 32;
        for (uint32_t i = 0; i < entries_per_cluster; i++) {
             fat_dir_entry_t *entry = (fat_dir_entry_t*)(cluster_buf + (i * 32));
             
             if ((uint8_t)entry->name[0] == 0x00) break; 
             if ((uint8_t)entry->name[0] == 0xE5) continue;
             if (entry->attr == ATTR_LONG_NAME) continue;
             
             // Check Name Match
             char filename[13];
             int j=0;
             for (int k=0; k<8; k++) {
                 if (entry->name[k] != ' ') filename[j++] = entry->name[k];
             }
             if (entry->ext[0] != ' ' && entry->ext[0] != 0) {
                 filename[j++] = '.';
                 for (int k=0; k<3; k++) {
                     if (entry->ext[k] != ' ') filename[j++] = entry->ext[k];
                 }
             }
             filename[j] = 0;
             
             if (strcmp(name, filename) == 0) {
                 // Found!
                 fs_node_t *ret = (fs_node_t*)memory_alloc(sizeof(fs_node_t));
                 strcpy(ret->name, filename);
                 ret->inode = (entry->cluster_high << 16) | entry->cluster_low;
                 ret->length = entry->size;
                 
                 if (entry->attr & ATTR_DIRECTORY) {
                     ret->flags = FS_DIRECTORY;
                     ret->readdir = fat32_readdir;
                     ret->finddir = fat32_finddir;
                 } else {
                     ret->flags = FS_FILE;
                     ret->read = fat32_read;
                 }
                 memory_free(cluster_buf);
                 return ret;
             }
        }
        cluster = get_next_cluster(cluster);
    }
    memory_free(cluster_buf);
    return 0;
}

uint32_t fat32_read(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    // Read File
    uint32_t cluster = node->inode;
    uint32_t read_bytes = 0;
    
    uint8_t *cluster_buf = (uint8_t*)memory_alloc(fat_fs.bytes_per_cluster);
    
    // Skip clusters until offset
    uint32_t cluster_size = fat_fs.bytes_per_cluster;
    while (offset >= cluster_size) {
        cluster = get_next_cluster(cluster);
        if (cluster >= 0x0FFFFFF8) { memory_free(cluster_buf); return 0; }
        offset -= cluster_size;
    }
    
    // Read Data
    while (read_bytes < size && cluster < 0x0FFFFFF8) {
        read_cluster(cluster, cluster_buf);
        
        uint32_t chunk = size - read_bytes;
        uint32_t available = cluster_size - offset;
        
        if (chunk > available) chunk = available;
        
        // Copy
        memcpy(buffer + read_bytes, cluster_buf + offset, chunk);
        
        read_bytes += chunk;
        offset = 0; // Next cluster starts at 0
        
        cluster = get_next_cluster(cluster);
    }
    
    memory_free(cluster_buf);
    return read_bytes;
}
