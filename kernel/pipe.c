#include "pipe.h"
#include "memory.h"
#include "string.h"
#include "process.h"

#define PIPE_SIZE 4096

typedef struct {
    uint8_t buffer[PIPE_SIZE];
    uint32_t read_ptr;
    uint32_t write_ptr;
    uint32_t bytes_available;
    int readers;
    int writers;
    // Simple verification lock mechanism
    int lock; 
} pipe_context_t;

static uint32_t pipe_read(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
static uint32_t pipe_write(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
static void pipe_close(fs_node_t *node);
static void pipe_open(fs_node_t *node);

static pipe_context_t* pipe_create_context() {
    pipe_context_t *ctx = (pipe_context_t*)memory_alloc(sizeof(pipe_context_t));
    memset(ctx, 0, sizeof(pipe_context_t));
    ctx->readers = 0;
    ctx->writers = 0;
    return ctx;
}

int make_pipe(fs_node_t **read_node, fs_node_t **write_node) {
    pipe_context_t *ctx = pipe_create_context();
    
    // Read Node
    fs_node_t *r = (fs_node_t*)memory_alloc(sizeof(fs_node_t));
    memset(r, 0, sizeof(fs_node_t));
    strcpy(r->name, "pipe_r");
    r->flags = FS_PIPE;
    r->read = pipe_read;
    r->write = NULL; // Read end cannot write
    r->open = pipe_open;
    r->close = pipe_close;
    r->ptr = (struct fs_node*)ctx; // Store context
    r->impl = 0; // 0 for read end?
    
    // Write Node
    fs_node_t *w = (fs_node_t*)memory_alloc(sizeof(fs_node_t));
    memset(w, 0, sizeof(fs_node_t));
    strcpy(w->name, "pipe_w");
    w->flags = FS_PIPE;
    w->read = NULL; // Write end cannot read
    w->write = pipe_write;
    w->open = pipe_open;
    w->close = pipe_close;
    w->ptr = (struct fs_node*)ctx;
    w->impl = 1; // 1 for write end?
    
    ctx->readers++; // Initially 1 of each passed back
    ctx->writers++;
    
    *read_node = r;
    *write_node = w;
    
    return 0;
}

static void pipe_open(fs_node_t *node) {
    // Usually called when dup'd
    // We increment ref count logic if we tracked FDs per process properly
    // For now, simple stubs.
    (void)node;
}

static void pipe_close(fs_node_t *node) {
    pipe_context_t *ctx = (pipe_context_t*)node->ptr;
    if (node->impl == 0) ctx->readers--; // Read end
    else ctx->writers--; // Write end
    
    // If no one left, free context (and buffer)
    if (ctx->readers == 0 && ctx->writers == 0) {
        memory_free(ctx);
    }
    
    // Free the node itself? 
    // Usually yes, since make_pipe allocated it and it's not in VFS tree.
    memory_free(node);
}

static uint32_t pipe_read(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)offset;
    pipe_context_t *ctx = (pipe_context_t*)node->ptr;
    
    uint32_t collected = 0;
    
    while (collected < size) {
        if (ctx->bytes_available > 0) {
            // Read byte
            buffer[collected] = ctx->buffer[ctx->read_ptr];
            ctx->read_ptr = (ctx->read_ptr + 1) % PIPE_SIZE;
            ctx->bytes_available--;
            collected++;
        } else {
             // Empty
             if (ctx->writers == 0) {
                 // EOF
                 return collected;
             }
             
             // Block/Yield
             // Ideally: event_wait(&ctx->wait_queue);
             // For now: yield
             extern void switch_task(void);
             switch_task();
        }
    }
    return collected;
}

static uint32_t pipe_write(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)offset;
    pipe_context_t *ctx = (pipe_context_t*)node->ptr;
    
    uint32_t written = 0;
    
    while (written < size) {
        if (ctx->bytes_available < PIPE_SIZE) {
            ctx->buffer[ctx->write_ptr] = buffer[written];
            ctx->write_ptr = (ctx->write_ptr + 1) % PIPE_SIZE;
            ctx->bytes_available++;
            written++;
        } else {
            // Full
            if (ctx->readers == 0) {
                // Broken pipe
                return written; // Or signal SIGPIPE
            }
            extern void switch_task(void);
            switch_task();
        }
    }
    return written;
}

void pipe_init(void) {
    // Nothing to do globally
}
