#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include "drm_backend.h"

// Helper: Open the first available DRM card
static int open_drm_device(void) {
    int fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        // Try card1 if card0 failed
        fd = open("/dev/dri/card1", O_RDWR | O_CLOEXEC);
    }
    return fd;
}

mithl_drm_context_t* drm_init(void) {
    mithl_drm_context_t *ctx = calloc(1, sizeof(mithl_drm_context_t));
    if (!ctx) return NULL;

    ctx->fd = open_drm_device();
    if (ctx->fd < 0) {
        perror("[DRM] Failed to open DRM device");
        free(ctx);
        return NULL;
    }

    ctx->res = drmModeGetResources(ctx->fd);
    if (!ctx->res) {
        perror("[DRM] Failed to get resources");
        close(ctx->fd);
        free(ctx);
        return NULL;
    }

    // Find a connected connector
    for (int i = 0; i < ctx->res->count_connectors; i++) {
        drmModeConnector *conn = drmModeGetConnector(ctx->fd, ctx->res->connectors[i]);
        if (conn->connection == DRM_MODE_CONNECTED && conn->count_modes > 0) {
            ctx->connector = conn;
            break;
        }
        drmModeFreeConnector(conn);
    }

    if (!ctx->connector) {
        fprintf(stderr, "[DRM] No connected connector found\n");
        return NULL;
    }

    ctx->connector_id = ctx->connector->connector_id;
    ctx->mode = ctx->connector->modes[0]; // Use preferred/first mode
    printf("[DRM] Mode: %dx%d @ %dHz\n", ctx->mode.hdisplay, ctx->mode.vdisplay, ctx->mode.vrefresh);

    // Find Encoder
    drmModeEncoder *enc = NULL;
    if (ctx->connector->encoder_id) {
        enc = drmModeGetEncoder(ctx->fd, ctx->connector->encoder_id);
    }

    if (enc) {
        ctx->crtc_id = enc->crtc_id;
        drmModeFreeEncoder(enc);
    } else {
        // Simple fallback: pick first CRTC
        // In real code, we should check possible_crtcs bitmask
         ctx->crtc_id = ctx->res->crtcs[0];
    }
    
    // Save original CRTC state to restore later
    ctx->crtc = drmModeGetCrtc(ctx->fd, ctx->crtc_id);

    return ctx;
}

// For this initial step, we will use "Dumb Buffers" instead of GBM
// Dumb buffers are simpler for software rendering (our CPU painter)
// GBM is for hardware acceleation (OpenGL) which we might not have set up yet.

struct dumb_rb {
    uint32_t handle;
    uint32_t pitch;
    uint32_t size;
    uint32_t fb_id;
    uint8_t *map;
};

static struct dumb_rb active_fb;
static struct dumb_rb back_fb; // Double buffering

static int create_dumb_buffer(int fd, int width, int height, struct dumb_rb *bo) {
    struct drm_mode_create_dumb create_req = {0};
    create_req.width = width;
    create_req.height = height;
    create_req.bpp = 32;

    if (drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_req) < 0) {
        perror("Create dumb buffer failed");
        return -1;
    }

    bo->handle = create_req.handle;
    bo->pitch = create_req.pitch;
    bo->size = create_req.size;

    // Create Framebuffer Object
    if (drmModeAddFB(fd, width, height, 24, 32, bo->pitch, bo->handle, &bo->fb_id)) {
        perror("AddFB failed");
        return -1;
    }

    // Map it
    struct drm_mode_map_dumb map_req = {0};
    map_req.handle = bo->handle;
    if (drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map_req)) {
        perror("Map dumb failed");
        return -1;
    }

    bo->map = mmap(0, bo->size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, map_req.offset);
    if (bo->map == MAP_FAILED) {
        perror("mmap failed");
        return -1;
    }

    // Clear to black
    memset(bo->map, 0, bo->size);
    return 0;
}

void* drm_get_buffer(mithl_drm_context_t *ctx) {
    // Initialize if not done
    if (!active_fb.map) {
        create_dumb_buffer(ctx->fd, ctx->mode.hdisplay, ctx->mode.vdisplay, &active_fb);
        create_dumb_buffer(ctx->fd, ctx->mode.hdisplay, ctx->mode.vdisplay, &back_fb);
        
        // Set Mode
        drmModeSetCrtc(ctx->fd, ctx->crtc_id, active_fb.fb_id, 0, 0, &ctx->connector_id, 1, &ctx->mode);
    }
    return back_fb.map; // Draw to back buffer
}

void drm_page_flip(mithl_drm_context_t *ctx) {
    // Swap front/back identifiers
    struct dumb_rb temp = active_fb;
    active_fb = back_fb;
    back_fb = temp;

    // Page flip (schedule flip)
    drmModePageFlip(ctx->fd, ctx->crtc_id, active_fb.fb_id, DRM_MODE_PAGE_FLIP_EVENT, ctx);
    
    // In a real loop we would wait for the event, but for now we just spin or let it tear/block
    // Or just simple SetCrtc (slower but easier for v1)
    // drmModeSetCrtc(ctx->fd, ctx->crtc_id, active_fb.fb_id, 0, 0, &ctx->connector_id, 1, &ctx->mode);
}

void drm_cleanup(mithl_drm_context_t *ctx) {
    // Restore CRTC
    if (ctx->crtc) {
        drmModeSetCrtc(ctx->fd, ctx->crtc->crtc_id, ctx->crtc->buffer_id, 
                       ctx->crtc->x, ctx->crtc->y, &ctx->connector_id, 1, &ctx->crtc->mode);
        drmModeFreeCrtc(ctx->crtc);
    }
    close(ctx->fd);
    free(ctx);
}
