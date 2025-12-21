#include "doomdef.h"
#include "d_main.h"
#include "i_system.h"
#include "i_video.h"
#include "i_sound.h"
#include "i_net.h"
#include "v_video.h"
#include "string.h"
#include "doomstat.h"

#include <stdlib.h>
#include <stdio.h>

#include "memory.h"
#include "console.h"
#include "graphics.h"
#include "keyboard.h"
#include "rtc.h"

// Globals required by Doom
int mb_used = 8;
extern boolean devparm; // Defined in d_main.c
extern byte* screens[5]; // Defined in v_video.c typically

// --- SYSTEM ---

void I_Init (void) {
    // console_write("[DOOM] I_Init\n");
}

void I_Quit (void) {
    console_write("[DOOM] Quit called.\n");
    // Hang forever
    for(;;);
}

byte* I_ZoneBase (int* size) {
    *size = mb_used * 1024 * 1024;
    return (byte *) memory_alloc(*size);
}

int I_GetTime (void) {
    // Return ticks (1/35 sec roughly)
    // rtc_get_seconds is missing and too slow.
    // Just increment for now (unlimited speed / fast forward)
    static int t = 0;
    return t++;
}

void I_Error (char *error, ...) {
    console_write("[DOOM ERROR] ");
    console_write(error);
    console_write("\n");
    for(;;);
}

int I_GetHeapSize (void) {
    return mb_used * 1024 * 1024;
}

byte* I_AllocLow (int length) {
    byte* mem = memory_alloc(length);
    if (mem) memset(mem, 0, length);
    return mem;
}

void I_Tactile (int on, int off, int total) { (void)on; (void)off; (void)total; }
void I_WaitVBL (int count) { 
    volatile int i;
    for(i=0; i<count*100000; i++); 
}
void I_BeginRead (void) { }
void I_EndRead (void) { }


// --- VIDEO ---

// Palette
static uint32_t doom_palette[256];

void I_InitGraphics (void) {
    console_write("[DOOM] Graphics Init\n");
}

void I_ShutdownGraphics (void) { }

void I_SetPalette (byte* palette) {
    int i;
    for (i = 0; i < 256; i++) {
        uint8_t r = gammatable[usegamma][*palette++];
        uint8_t g = gammatable[usegamma][*palette++];
        uint8_t b = gammatable[usegamma][*palette++];
        doom_palette[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
    }
}

#include "gui.h"

static gui_window_t *doom_window = NULL;

void doom_assign_window(gui_window_t *w) {
    doom_window = w;
}

// Custom Draw Handler for Doom Window
void doom_draw(gui_renderer_t *renderer, gui_element_t *element) {
    (void)element; // We use global doom_window or element (casted)
    
    // 1. Draw Window Frame (Title bar, etc)
    gui_draw_window(renderer, element);
    
    // 2. Draw Doom Content
    if (!screens[0]) {
        // Draw black placeholder if Doom not ready
        rect_t r = element->bounds;
        r.y += 24; // Title bar offset
        r.height -= 24;
        renderer->draw_rect(r, 0xFF000000);
        return;
    }

    int scale = 1; // Auto scale? Or 2x default?
    int doom_w = 320;
    int doom_h = 200;
    
    // Center in window
    int win_w = element->bounds.width;
    int win_h = element->bounds.height - 24; // Minus title bar
    
    // Calculate max integer scale
    int scale_x = win_w / doom_w;
    int scale_y = win_h / doom_h;
    scale = (scale_x < scale_y) ? scale_x : scale_y;
    if (scale < 1) scale = 1;
    
    int draw_w = doom_w * scale;
    int draw_h = doom_h * scale;
    
    int offset_x = element->bounds.x + (win_w - draw_w) / 2;
    int offset_y = element->bounds.y + 24 + (win_h - draw_h) / 2;

    // We manually draw pixels/rects using renderer
    // Optimized blit would be better (renderer->draw_bitmap?), but we don't have it.
    // We use draw_rect_filled for "pixels".
    
    byte* src = screens[0];
    int x, y;
    
    for (y = 0; y < doom_h; y++) {
        for (x = 0; x < doom_w; x++) {
            uint8_t pal_idx = *src++;
            uint32_t color = doom_palette[pal_idx];
            
            // Draw Scaled Pixel
            rect_t px = {
                offset_x + x*scale,
                offset_y + y*scale,
                scale,
                scale
            };
            renderer->draw_rect(px, color);
        }
    }
}

void I_UpdateNoBlit (void) { }

void I_FinishUpdate (void) {
    // Just mark window as dirty
    if (doom_window) {
        gui_invalidate_rect(doom_window->base.bounds);
    }
}

void I_ReadScreen (byte* scr) {
    memcpy(scr, screens[0], SCREENWIDTH*SCREENHEIGHT);
}

void I_StartFrame (void) { }
void I_StartTic (void) { 
    // Input Handling (Pending)
}

// --- SOUND / NET ---
void I_InitSound() {}
void I_UpdateSound() {}
void I_SubmitSound() {}
void I_ShutdownSound() {}
void I_SetChannels() {}
void I_InitMusic(void) {}
void I_ShutdownMusic(void) {}
void I_PlaySong(int handle, int looping) { (void)handle; (void)looping; }
void I_PauseSong(int handle) { (void)handle; }
void I_ResumeSong(int handle) { (void)handle; }
void I_StopSong(int handle) { (void)handle; }
void I_UnRegisterSong(int handle) { (void)handle; }
int I_RegisterSong(void *data) { (void)data; return 1; }
int I_QrySongPlaying(int handle) { (void)handle; return 0; }
int I_GetSfxLumpNum(struct sfxinfo_struct *sfx) { (void)sfx; return 0; }

// Fixed signature: 5 arguments, no pri
int I_StartSound(int id, int sing, int vol, int sep, int pitch) { 
    (void)id; (void)sing; (void)vol; (void)sep; (void)pitch;
    return 0; 
}
void I_StopSound(int handle) { (void)handle; }
int I_SoundIsPlaying(int handle) { (void)handle; return 0; }
void I_UpdateSoundParams(int handle, int vol, int sep, int pitch) { (void)handle; (void)vol; (void)sep; (void)pitch; }

ticcmd_t* I_BaseTiccmd(void) {
    static ticcmd_t ticcmd;
    return &ticcmd;
}

void I_InitNetwork (void) {
    doomcom = memory_alloc (sizeof (*doomcom) );
    memset (doomcom, 0, sizeof(*doomcom) );
    
    doomcom->id = DOOMCOM_ID;
    doomcom->numplayers = doomcom->numnodes = 1;
    doomcom->deathmatch = false;
    doomcom->consoleplayer = 0;
}

void I_NetCmd (void) { }
