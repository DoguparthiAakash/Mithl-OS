#include "acpi.h"
#include "string.h"
#include "ports.h"
#include "memory.h"
#include "console.h" // For debug printing

// -- ACPI Structures --

typedef struct {
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;
} __attribute__((packed)) rsdp_t;

typedef struct {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed)) sdt_header_t;

typedef struct {
    sdt_header_t h;
    uint32_t firmware_ctrl;
    uint32_t dsdt;
    uint8_t reserved;
    uint8_t preferred_power_management_profile;
    uint16_t sci_int;
    uint32_t smi_cmd;
    uint8_t acpi_enable;
    uint8_t acpi_disable;
    uint8_t s4bios_req;
    uint8_t pstate_cnt;
    uint32_t pm1a_evt_blk;
    uint32_t pm1b_evt_blk;
    uint32_t pm1a_cnt_blk; // This is what we want!
    uint32_t pm1b_cnt_blk;
    uint32_t pm2_cnt_blk;
    uint32_t pm_tmr_blk;
    uint32_t gpe0_blk;
    uint32_t gpe1_blk;
    // ... more fields follow but we mostly need pm1a_cnt_blk
} __attribute__((packed)) fadt_t;

// -- Globals --
static uint32_t pm1a_cnt_blk = 0;
static uint32_t pm1b_cnt_blk = 0;
static int acpi_enabled = 0;

// -- Helper: Checksum --
static int check_sum(uint8_t *ptr, int len) {
    uint8_t sum = 0;
    for (int i = 0; i < len; i++) {
        sum += ptr[i];
    }
    return sum == 0;
}

// -- Manual RSDP Override from Multiboot --
static uintptr_t rsdp_override = 0;

void acpi_set_rsdp(uintptr_t addr) {
    rsdp_override = addr;
}

// -- Helper: Find RSDP --
static rsdp_t *find_rsdp(void) {
    if (rsdp_override != 0) {
        return (rsdp_t *)rsdp_override;
    }

    // 1. Search in EBDA (Extended BIOS Data Area)
    uint8_t *ebda = (uint8_t *)(*(uint16_t *)0x40E << 4);
    for (int i = 0; i < 1024; i += 16) {
        if (strncmp((char *)(ebda + i), "RSD PTR ", 8) == 0) {
             if (check_sum(ebda + i, 20)) return (rsdp_t *)(ebda + i);
        }
    }
    
    // 2. Search in Main BIOS Area (0xE0000 - 0xFFFFF)
    for (uint8_t *p = (uint8_t *)0xE0000; p < (uint8_t *)0xFFFFF; p += 16) {
        if (strncmp((char *)p, "RSD PTR ", 8) == 0) {
             if (check_sum(p, 20)) return (rsdp_t *)p;
        }
    }
    
    return NULL;
}

// -- Helper: Hex String --
void acpi_print_hex(const char* prefix, uint32_t val) {
    char buf[32];
    int idx = 0;
    while(prefix[idx]) { buf[idx] = prefix[idx]; idx++; }
    
    buf[idx++] = '0'; buf[idx++] = 'x';
    
    for (int i = 28; i >= 0; i -= 4) {
        uint8_t nibble = (val >> i) & 0xF;
        buf[idx++] = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
    }
    buf[idx++] = '\n';
    buf[idx] = 0;
    serial_write(buf);
}

// -- Init --
void acpi_init(void) {
    serial_write("[ACPI] Finding RSDP...\n");
    rsdp_t *rsdp = find_rsdp();
    if (!rsdp) {
        serial_write("[ACPI] RSDP not found.\n");
        return;
    }
    
    // Validate RSDT
    sdt_header_t *rsdt = (sdt_header_t *)rsdp->rsdt_address;
    if (!check_sum((uint8_t*)rsdt, rsdt->length)) {
        serial_write("[ACPI] RSDT checksum failed. (Continuing anyway)\n");
    }
    
    // Find FACP (FADT)
    int entries = (rsdt->length - sizeof(sdt_header_t)) / 4;
    uint32_t *pointers = (uint32_t *)(rsdt + 1);
    
    fadt_t *fadt = NULL;
    
    serial_write("[ACPI] Scanning Tables:\n");
    for (int i = 0; i < entries; i++) {
        sdt_header_t *h = (sdt_header_t *)pointers[i];
        char sig[5];
        memcpy(sig, h->signature, 4); sig[4] = 0;
        serial_write(sig); serial_write(" ");
        
        if (strncmp(h->signature, "FACP", 4) == 0) {
            fadt = (fadt_t *)h;
            break;
        }
    }
    serial_write("\n");
    
    if (!fadt) {
        serial_write("[ACPI] FADT not found.\n");
        return;
    }
    
    // Store PM1a Control Block Address
    pm1a_cnt_blk = fadt->pm1a_cnt_blk;
    pm1b_cnt_blk = fadt->pm1b_cnt_blk;
    
    acpi_print_hex("[ACPI] PM1a_CNT_BLK: ", pm1a_cnt_blk);
    acpi_print_hex("[ACPI] PM1b_CNT_BLK: ", pm1b_cnt_blk);
    
    // Enable ACPI if needed
    if (fadt->smi_cmd && fadt->acpi_enable) {
        serial_write("[ACPI] Enabling ACPI Mode...\n");
        outb(fadt->smi_cmd, fadt->acpi_enable);
        
        // Wait for potential transition
        for(volatile int w=0; w<100000; w++);
    } else {
        serial_write("[ACPI] ACPI already enabled or not required.\n");
    }
    
    acpi_enabled = 1;
}

// -- Shutdown --
void acpi_shutdown(void) {
    serial_write("[ACPI] Requesting Shutdown...\n");
    
    if (!acpi_enabled) acpi_init();
    if (!acpi_enabled || pm1a_cnt_blk == 0) {
        serial_write("[ACPI] Error: ACPI not initialized or PM1a missing.\n");
        return;
    }
    
    uint16_t slp_en = 1 << 13;
    
    // VirtualBox Soft Off: SLP_TYP = 5
    uint16_t cmd = slp_en | (5 << 10); // 0x3400
    
    acpi_print_hex("[ACPI] Writing Opcode to PM1a: ", cmd);
    
    // Try both 16-bit and 32-bit writes just in case
    outw(pm1a_cnt_blk, cmd);
    if (pm1b_cnt_blk) outw(pm1b_cnt_blk, cmd);
    
    // Small delay
    for(volatile int w=0; w<100000; w++);
    
    // Try SLP_TYP = 0 (Older VBox / QEMU)
    outw(pm1a_cnt_blk, slp_en | (0 << 10)); 
    if (pm1b_cnt_blk) outw(pm1b_cnt_blk, slp_en | (0 << 10));
}
