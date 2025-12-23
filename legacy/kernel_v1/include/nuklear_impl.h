/* kernel/include/nuklear_impl.h */
#ifndef NUKLEAR_IMPL_H
#define NUKLEAR_IMPL_H

// Shim for Nuklear in Kernel Mode
#define NK_ASSERT(x) (void)0
//#define NK_INCLUDE_FIXED_TYPES // Might be needed
//#define NK_INCLUDE_STANDARD_IO // we don't have stdio

#include "nuklear.h"
#include "gui.h"

/* Initialize Nuklear context and font */
void nk_mithl_init(struct nk_context *ctx);

/* Process input from Mithl-OS devices and feed to Nuklear */
void nk_mithl_handle_input(struct nk_context *ctx);
int nk_mithl_handle_event(struct nk_context *ctx, gui_event_t *ev);

/* Render Nuklear command buffer to screen */
void nk_mithl_render(struct nk_context *ctx);

/* Run Demo GUI */
void nk_mithl_demo(struct nk_context *ctx);

#endif
