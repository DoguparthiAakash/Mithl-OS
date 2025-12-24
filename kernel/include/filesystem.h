#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "graphics.h"

/* File system constants */
#define MAX_FILENAME 32
#define MAX_FILES 100
#define MAX_FILE_SIZE 4096
#define MAX_DIRECTORIES 20

/* File types */
typedef enum {
    FILE_TYPE_REGULAR,
    FILE_TYPE_DIRECTORY,
    FILE_TYPE_TEXT,
    FILE_TYPE_BINARY
} file_type_t;

/* File entry structure */
typedef struct {
    char name[MAX_FILENAME];
    file_type_t type;
    uint32_t size;
    uint32_t created_time;
    uint32_t modified_time;
    uint8_t data[MAX_FILE_SIZE];
    int is_open;
} file_entry_t;

/* Directory structure */
typedef struct {
    char name[MAX_FILENAME];
    file_entry_t files[MAX_FILES];
    int file_count;
    int is_open;
} directory_t;

/* File system structure */
typedef struct {
    directory_t root;
    directory_t directories[MAX_DIRECTORIES];
    int directory_count;
    int current_directory;
} filesystem_t;

/* File operations */
int fs_init(void);
int fs_create_file(const char *name, file_type_t type);
int fs_delete_file(const char *name);
int fs_open_file(const char *name);
int fs_close_file(const char *name);
int fs_read_file(const char *name, char *buffer, int size);
int fs_write_file(const char *name, const char *data, int size);
int fs_list_files(char *buffer, int buffer_size);

/* Directory operations */
int fs_create_directory(const char *name);
int fs_change_directory(const char *name);
int fs_list_directory(const char *path, char *buffer, int buffer_size);

/* Utility functions */
int fs_file_exists(const char *name);
int fs_get_file_size(const char *name);
file_type_t fs_get_file_type(const char *name);

/* Global filesystem instance */
extern filesystem_t fs;

#endif /* FILESYSTEM_H */
