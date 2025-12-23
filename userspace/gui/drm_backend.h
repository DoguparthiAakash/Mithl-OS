#ifndef DRM_BACKEND_H
#define DRM_BACKEND_H

#include <stdint.h>
#include <stdbool.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>

typedef struct {
    uint32_t handle;
    uint32_t pitch;
    uint32_t size;
    uint32_t fb_id;
    int dmabuf_fd;
    void *map;
} struct_bo;

typedef struct {
    int fd;
    drmModeRes *res;
    drmModeConnector *connector;
    drmModeEncoder *encoder;
    drmModeCrtc *crtc; // Saved CRTC to restore on exit
    drmModeModeInfo mode;
    
    struct gbm_device *gbm;
    struct gbm_surface *gbm_surface;
    uint32_t crtc_id;
    uint32_t connector_id;
    
    // Double buffering
    struct gbm_bo *previous_bo;
    uint32_t previous_fb;
} mithl_drm_context_t;

// API
mithl_drm_context_t* drm_init(void);
void drm_cleanup(mithl_drm_context_t *ctx);
void drm_page_flip(mithl_drm_context_t *ctx);
void* drm_get_buffer(mithl_drm_context_t *ctx); // Returns pointer to write pixels to

#endif
