#include "filesystem.h"

/* Define size_t for freestanding environment */
typedef unsigned int size_t;

/* Global filesystem instance */
filesystem_t fs = { 0 };

/* Helper functions */
static void memset(void *ptr, int value, size_t num) {
    unsigned char *p = ptr;
    for (size_t i = 0; i < num; i++) {
        p[i] = (unsigned char)value;
    }
}

static void memcpy(void *dest, const void *src, size_t num) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    for (size_t i = 0; i < num; i++) {
        d[i] = s[i];
    }
}

static size_t strlen(const char *str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

static int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

/* Simple snprintf implementation */
static int snprintf(char *str, size_t size, const char *format, ...)
{
    // Very simple implementation - just copy the format string
    // In a real system, you'd implement proper formatting
    int i = 0;
    while (format[i] && i < (int)size - 1) {
        str[i] = format[i];
        i++;
    }
    str[i] = '\0';
    return i;
}

/* Initialize filesystem */
int fs_init(void)
{
    memset(&fs, 0, sizeof(filesystem_t));
    
    // Initialize root directory
    memcpy(fs.root.name, "root", 5);
    fs.root.file_count = 0;
    fs.current_directory = 0;
    
    // Create some default files
    fs_create_file("README.txt", FILE_TYPE_TEXT);
    fs_create_file("welcome.txt", FILE_TYPE_TEXT);
    fs_create_file("system.log", FILE_TYPE_TEXT);
    
    // Create folders for UI testing
    fs_create_file("Documents", FILE_TYPE_DIRECTORY);
    fs_create_file("Pictures", FILE_TYPE_DIRECTORY);
    fs_create_file("Music", FILE_TYPE_DIRECTORY);
    fs_create_file("Downloads", FILE_TYPE_DIRECTORY);
    
    // Write some default content
    const char *readme_content = "Welcome to Mythl OS!\n\nThis is a modern operating system with a macOS-inspired interface.\n\nFeatures:\n- File management system\n- Text editor\n- File browser\n- System utilities\n\nEnjoy using Mythl OS!";
    fs_write_file("README.txt", readme_content, strlen(readme_content));
    
    const char *welcome_content = "Hello and welcome to Mythl OS!\n\nThis is your new operating system. You can:\n- Create and edit text files\n- Browse the file system\n- Use various system utilities\n- Enjoy a beautiful interface\n\nStart exploring!";
    fs_write_file("welcome.txt", welcome_content, strlen(welcome_content));
    
    const char *log_content = "System booted successfully\nFilesystem initialized\nGUI system ready\nAll systems operational";
    fs_write_file("system.log", log_content, strlen(log_content));
    
    return 0;
}

/* Create a new file */
int fs_create_file(const char *name, file_type_t type)
{
    if (fs.root.file_count >= MAX_FILES) return -1;
    
    file_entry_t *file = &fs.root.files[fs.root.file_count];
    
    memcpy(file->name, name, strlen(name) + 1);
    file->type = type;
    file->size = 0;
    file->created_time = 0; // Simple timestamp
    file->modified_time = 0;
    file->is_open = 0;
    
    memset(file->data, 0, MAX_FILE_SIZE);
    
    fs.root.file_count++;
    return 0;
}

/* Delete a file */
int fs_delete_file(const char *name)
{
    for (int i = 0; i < fs.root.file_count; i++) {
        if (strcmp(fs.root.files[i].name, name) == 0) {
            // Move remaining files up
            for (int j = i; j < fs.root.file_count - 1; j++) {
                memcpy(&fs.root.files[j], &fs.root.files[j + 1], sizeof(file_entry_t));
            }
            fs.root.file_count--;
            return 0;
        }
    }
    return -1;
}

/* Open a file */
int fs_open_file(const char *name)
{
    for (int i = 0; i < fs.root.file_count; i++) {
        if (strcmp(fs.root.files[i].name, name) == 0) {
            fs.root.files[i].is_open = 1;
            return i; // Return file index
        }
    }
    return -1;
}

/* Close a file */
int fs_close_file(const char *name)
{
    for (int i = 0; i < fs.root.file_count; i++) {
        if (strcmp(fs.root.files[i].name, name) == 0) {
            fs.root.files[i].is_open = 0;
            return 0;
        }
    }
    return -1;
}

/* Read from a file */
int fs_read_file(const char *name, char *buffer, int size)
{
    for (int i = 0; i < fs.root.file_count; i++) {
        if (strcmp(fs.root.files[i].name, name) == 0) {
            file_entry_t *file = &fs.root.files[i];
            int read_size = (size < (int)file->size) ? size : (int)file->size;
            memcpy(buffer, file->data, read_size);
            return read_size;
        }
    }
    return -1;
}

/* Write to a file */
int fs_write_file(const char *name, const char *data, int size)
{
    for (int i = 0; i < fs.root.file_count; i++) {
        if (strcmp(fs.root.files[i].name, name) == 0) {
            file_entry_t *file = &fs.root.files[i];
            if (size > MAX_FILE_SIZE) size = MAX_FILE_SIZE;
            
            memcpy(file->data, data, size);
            file->size = size;
            file->modified_time = 0; // Simple timestamp update
            
            return size;
        }
    }
    return -1;
}

/* List all files */
int fs_list_files(char *buffer, int buffer_size)
{
    int offset = 0;
    offset += snprintf(buffer + offset, buffer_size - offset, "Files in current directory:\n\n");
    
    for (int i = 0; i < fs.root.file_count; i++) {
        file_entry_t *file = &fs.root.files[i];
        const char *type_str = (file->type == FILE_TYPE_TEXT) ? "TEXT" : 
                              (file->type == FILE_TYPE_DIRECTORY) ? "DIR" : "FILE";
        
        offset += snprintf(buffer + offset, buffer_size - offset, 
                          "%-20s %-6s %6d bytes %s\n", 
                          file->name, type_str, file->size,
                          file->is_open ? "[OPEN]" : "");
        
        if (offset >= buffer_size - 100) break; // Leave some buffer space
    }
    
    return offset;
}

/* Create a directory */
int fs_create_directory(const char *name)
{
    if (fs.directory_count >= MAX_DIRECTORIES) return -1;
    
    directory_t *dir = &fs.directories[fs.directory_count];
    memcpy(dir->name, name, strlen(name) + 1);
    dir->file_count = 0;
    dir->is_open = 0;
    
    fs.directory_count++;
    return 0;
}

/* Change directory */
int fs_change_directory(const char *name)
{
    if (strcmp(name, "..") == 0) {
        fs.current_directory = 0; // Go back to root
        return 0;
    }
    
    for (int i = 0; i < fs.directory_count; i++) {
        if (strcmp(fs.directories[i].name, name) == 0) {
            fs.current_directory = i;
            return 0;
        }
    }
    return -1;
}

/* List directory contents */
int fs_list_directory(const char *path, char *buffer, int buffer_size)
{
    (void)path; // Suppress unused parameter warning
    return fs_list_files(buffer, buffer_size);
}

/* Check if file exists */
int fs_file_exists(const char *name)
{
    for (int i = 0; i < fs.root.file_count; i++) {
        if (strcmp(fs.root.files[i].name, name) == 0) {
            return 1;
        }
    }
    return 0;
}

/* Get file size */
int fs_get_file_size(const char *name)
{
    for (int i = 0; i < fs.root.file_count; i++) {
        if (strcmp(fs.root.files[i].name, name) == 0) {
            return fs.root.files[i].size;
        }
    }
    return -1;
}

/* Get file type */
file_type_t fs_get_file_type(const char *name)
{
    for (int i = 0; i < fs.root.file_count; i++) {
        if (strcmp(fs.root.files[i].name, name) == 0) {
            return fs.root.files[i].type;
        }
    }
    return FILE_TYPE_REGULAR;
}
