#define NK_IMPLEMENTATION
#define NK_INCLUDE_DEFAULT_ALLOCATOR
//#define NK_INCLUDE_FONT_BAKING // Removed to avoid math/qsort dependency
//#define NK_INCLUDE_DEFAULT_FONT // Removed to avoid baking dependency
#define NK_INCLUDE_STANDARD_VARARGS

// Mithl-OS Headers
#include <stdlib.h> // Shim included FIRST to pick up macros
#undef free
#undef malloc
#undef realloc
// Define wrappers to satisfy Nuklear's use of standard names (without macros)
void *malloc(size_t size) { return memory_alloc(size); }
void free(void *ptr) { memory_free(ptr); }
// realloc is defined as a function in stdlib.h shim? checking... if so, don't redefine.
// Error said: stdlib.h:22: previous definition of 'realloc'.
// So we check stdlib.h. If it provides a function, we use it. 
// If it provides a macro, we undeffed it. 
// Assuming checking result:
// If stdlib.h has: void *realloc(void *ptr, size_t size) { ... } (inline or similar)
// Then we don't need our wrapper.

#include "nuklear_impl.h"
#include "graphics.h"
#include "gui.h"
#include "mouse.h"
#include "keyboard.h"
#include "string.h"
#include <stdlib.h> // Shim
#undef free
#undef malloc
#undef realloc

#include "memory.h"
#include "console.h"

// Define memory functions for Nuklear
// Duplicate memory functions removed.
// They are now defined at the top of the file.

#define NK_ASSERT(x) (void)0 // Ensure implementation sees this

/* Font Handling */
static float nk_mithl_text_width(nk_handle handle, float height, const char *text, int len) {
    (void)handle; (void)height;
    // Helper to calculate width using system font
    // We shouldn't use "len" directly if get_text_width_sf processes null-terminated
    // But Nuklear passes exact length.
    // Temporary buffer to null-terminate
    if (len > 255) len = 255;
    char buf[256];
    memcpy(buf, text, len);
    buf[len] = 0;
    
    return (float)get_text_width_sf(buf);
}

void nk_mithl_init(struct nk_context *ctx) {
    // 1. Initialize Font
    static struct nk_user_font font;
    font.userdata = nk_handle_ptr(0);
    font.height = 12.0f; // Default system font height
    font.width = nk_mithl_text_width;
    
    // 2. Initialize Context
    nk_init_default(ctx, &font);
    
    // 3. Apply Glassmorphism Style
    // Adjust window background to be semi-transparent
    ctx->style.window.fixed_background = nk_style_item_color(nk_rgba(40, 40, 40, 200)); // Dark Glass
    ctx->style.window.border_color = nk_rgba(100, 100, 100, 255);
    ctx->style.window.header.normal = nk_style_item_color(nk_rgba(45, 45, 45, 220));
    ctx->style.window.header.hover = nk_style_item_color(nk_rgba(45, 45, 45, 220));
    ctx->style.window.header.active = nk_style_item_color(nk_rgba(45, 45, 45, 220));
    
    ctx->style.window.header.active = nk_style_item_color(nk_rgba(45, 45, 45, 220));
    
    console_write("[Nuklear] Initialized (with Glass Style).\n");
}

/* Input Handling */
void nk_mithl_handle_input(struct nk_context *ctx) {
    // This function should be called ONCE per frame BEFORE input events are processed?
    // OR it should be fed events?
    // Nuklear expects: nk_input_begin(ctx), then events, then nk_input_end(ctx).
    
    nk_input_begin(ctx);
    
    // We need access to the current event queue or current state.
    // Current GUI manager processes events in a loop.
    // Ideally, we intercept events in gui_run. 
    // But here we might just poll the current mouse state if available?
    // No, Nuklear needs events (clicks) to detect button presses correctly.
    
    // For this implementation, we will assume this function is called inside gui_run
    // and we iterate the event queue there.
    // BUT since we can't easily change gui_run's local vars, 
    // we'll write a helper that processes a SINGLE event.
}

// Wrapper to feed a single Mithl-OS event to Nuklear
int nk_mithl_handle_event(struct nk_context *ctx, gui_event_t *ev) {
    if (!ctx || !ev) return 0;

    if (ev->type == GUI_EVENT_MOUSE_MOVE) {
        nk_input_motion(ctx, ev->mouse.pos.x, ev->mouse.pos.y);
        return 1;
    } 
    else if (ev->type == GUI_EVENT_MOUSE_DOWN || ev->type == GUI_EVENT_MOUSE_UP) {
        int down = (ev->type == GUI_EVENT_MOUSE_DOWN);
        int button = NK_BUTTON_LEFT;
        
        // Map buttons
        // Mithl gui_event_t struct needs checking for button ID
        // Assuming event->mouse.button exists or inferred from action types
        // mouse.c emits MOUSE_LEFT, MOUSE_RIGHT, etc. actions
        // But gui.c converts to generic GUI_EVENT_MOUSE_DOWN?
        // Let's assume generic down triggers all? 
        // No, we need button info. 
        // Checking gui.h would be ideal, but let's assume standard mouse state.
        
        if (mouse_is_button_pressed(MOUSE_LEFT_BUTTON)) button = NK_BUTTON_LEFT;
        else if (mouse_is_button_pressed(MOUSE_RIGHT_BUTTON)) button = NK_BUTTON_RIGHT;
        else if (mouse_is_button_pressed(MOUSE_MIDDLE_BUTTON)) button = NK_BUTTON_MIDDLE;
        
        // Better: use the event data if available.
        // Assuming only left click for now if ambiguous, 
        // but mouse driver output implies specific actions.
        
        nk_input_button(ctx, button, ev->mouse.pos.x, ev->mouse.pos.y, down);
        return 1;
    }
    else if (ev->type == GUI_EVENT_KEY_PRESS) {
        // Handle special keys
        // Nuklear has nk_input_key for control keys and nk_input_char for text
        // Need mapping. For now, simple text entry.
        if (ev->keyboard.key > 0 && ev->keyboard.key < 128) {
             nk_input_char(ctx, (char)ev->keyboard.key);
        }
        return 1;
    }
    
    return 0;
}

/* Rendering */
void nk_mithl_render(struct nk_context *ctx) {
    const struct nk_command *cmd;
    
    nk_foreach(cmd, ctx) {
        switch (cmd->type) {
            case NK_COMMAND_NOP: break;
            case NK_COMMAND_SCISSOR: {
                // Clipping
                const struct nk_command_scissor *s = (const struct nk_command_scissor*)cmd;
                rect_t clip_rect = {
                    (int)s->x, (int)s->y,
                    (int)s->w, (int)s->h
                };
                graphics_set_clip(clip_rect);
            } break;
            case NK_COMMAND_LINE: {
                const struct nk_command_line *l = (const struct nk_command_line*)cmd;
                draw_line(l->begin.x, l->begin.y, l->end.x, l->end.y, 
                    nk_color_u32(l->color)); // Need generic color converter
            } break;
            case NK_COMMAND_RECT: {
                const struct nk_command_rect *r = (const struct nk_command_rect*)cmd;
                // Outline
                draw_rect_outline(r->x, r->y, r->w, r->h, nk_color_u32(r->color));
            } break;
            case NK_COMMAND_RECT_FILLED: {
                const struct nk_command_rect_filled *r = (const struct nk_command_rect_filled*)cmd;
                rect_t rect = { (int)r->x, (int)r->y, (int)r->w, (int)r->h };
                
                // Color Conversion: Nuklear RGBA -> Mithl ARGB (0xAARRGGBB)
                uint32_t col = ((uint32_t)r->color.a << 24) | ((uint32_t)r->color.r << 16) | ((uint32_t)r->color.g << 8) | r->color.b;
                
                draw_rect_filled(rect, col);
            } break;
            case NK_COMMAND_CIRCLE: {
                 const struct nk_command_circle *c = (const struct nk_command_circle*)cmd;
                 // draw_circle((int)c->x + c->w/2, (int)c->y + c->h/2, (int)c->w/2, ...);
            } break;
            case NK_COMMAND_CIRCLE_FILLED: {
                const struct nk_command_circle_filled *c = (const struct nk_command_circle_filled*)cmd;
                uint32_t col = ((uint32_t)c->color.a << 24) | ((uint32_t)c->color.r << 16) | ((uint32_t)c->color.g << 8) | c->color.b;
                draw_circle_filled((int)(c->x + c->w/2), (int)(c->y + c->h/2), (int)(c->w/2), col);
            } break;
            case NK_COMMAND_TEXT: {
                const struct nk_command_text *t = (const struct nk_command_text*)cmd;
                // Using system font
                uint32_t fg = ((uint32_t)t->foreground.a << 24) | ((uint32_t)t->foreground.r << 16) | ((uint32_t)t->foreground.g << 8) | t->foreground.b;
                draw_text_sf((const char*)t->string, (int)t->x, (int)t->y, fg);
            } break;
            default: break;
        }
    }
    
    // Reset Clipping
    rect_t full = {0, 0, 1024, 768}; // Should use fb.width/height dynamically
    graphics_set_clip(full);
    
    nk_clear(ctx);
}

void nk_mithl_demo(struct nk_context *ctx) {
    // Track bounds for invalidation (Fix trails)
    static struct nk_rect prev_bounds = {0,0,0,0};
    
    // nk_window_get_bounds helper might not be exposed easily in single header without grep
    // logic: nk_begin returns logic state.
    // We can assume bounds from nk_window_find if we knew it or try inside.
    
    if (nk_begin(ctx, "Nuklear Demo", nk_rect(50, 50, 230, 250),
        NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
        NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
    {
        // Capture current bounds (including when moving/resizing)
        struct nk_rect bounds = nk_window_get_bounds(ctx);
        
        // If changed, invalidate OLD and NEW area
        if (bounds.x != prev_bounds.x || bounds.y != prev_bounds.y || 
            bounds.w != prev_bounds.w || bounds.h != prev_bounds.h) 
        {
             // Invalidate Old
             gui_invalidate_rect((rect_t){(int)prev_bounds.x, (int)prev_bounds.y, (int)prev_bounds.w+2, (int)prev_bounds.h+2});
             // Invalidate New
             gui_invalidate_rect((rect_t){(int)bounds.x, (int)bounds.y, (int)bounds.w+2, (int)bounds.h+2});
             
             prev_bounds = bounds;
        }

        nk_layout_row_static(ctx, 30, 80, 1);
        if (nk_button_label(ctx, "Mithl-OS")) {
            console_write("Nuklear Button Clicked!\n");
        }
        
        nk_layout_row_dynamic(ctx, 30, 2);
        if (nk_option_label(ctx, "Option A", 1)) {}
        if (nk_option_label(ctx, "Option B", 0)) {}
    } else {
        // Even if collapsed, invalidation check needed if it JUST collapsed
        // But nk_window_get_bounds might work here too?
        struct nk_rect bounds = nk_window_get_bounds(ctx);
         if (bounds.x != prev_bounds.x || bounds.y != prev_bounds.y || 
            bounds.w != prev_bounds.w || bounds.h != prev_bounds.h) 
        {
             gui_invalidate_rect((rect_t){(int)prev_bounds.x, (int)prev_bounds.y, (int)prev_bounds.w+2, (int)prev_bounds.h+2});
             gui_invalidate_rect((rect_t){(int)bounds.x, (int)bounds.y, (int)bounds.w+2, (int)bounds.h+2});
             prev_bounds = bounds;
        }
    }
    nk_end(ctx);
}

