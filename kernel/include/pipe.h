#ifndef PIPE_H
#define PIPE_H

#include "vfs.h"

// Initialize pipe system (if needed)
void pipe_init(void);

// Create a pipe, returning two nodes (read and write)
int make_pipe(fs_node_t **read_node, fs_node_t **write_node);

#endif
