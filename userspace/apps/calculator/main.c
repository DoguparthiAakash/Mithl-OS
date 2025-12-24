#include "../../libc/stdlib.h"

// Basic String Helper
int strlen(const char *str) {
    int len = 0;
    while (str[len]) len++;
    return len;
}

void strcpy(char *dest, const char *src) {
    while ((*dest++ = *src++));
}

void itoa(int n, char *buf) {
    if (n == 0) {
        buf[0] = '0';
        buf[1] = 0;
        return;
    }
    
    int i = 0;
    int is_neg = 0;
    if (n < 0) {
        is_neg = 1;
        n = -n;
    }
    
    char temp[16];
    while (n > 0) {
        temp[i++] = (n % 10) + '0';
        n /= 10;
    }
    if (is_neg) temp[i++] = '-';
    
    int j = 0;
    while (i > 0) {
        buf[j++] = temp[--i];
    }
    buf[j] = 0;
}

// GUI Constants (Kora inspired colors)
#define COL_BG      0xFF2F343F
#define COL_BTN     0xFF4C566A
#define COL_ACCENT  0xFF88C0D0
#define COL_TEXT    0xFFECEFF4
#define COL_OP      0xFFBF616A // Red-ish for operators

typedef struct {
    int x, y, w, h;
    const char *label;
    uint32_t color;
} Button;

#define NUM_BTNS 16
Button buttons[NUM_BTNS];
char display_buf[32] = "0";

void init_buttons() {
    int w = 50;
    int h = 40;
    int margin = 5;
    int start_y = 60;
    
    // Row 1: 7 8 9 /
    buttons[0] = (Button){margin, start_y, w, h, "7", COL_BTN};
    buttons[1] = (Button){margin + (w+margin)*1, start_y, w, h, "8", COL_BTN};
    buttons[2] = (Button){margin + (w+margin)*2, start_y, w, h, "9", COL_BTN};
    buttons[3] = (Button){margin + (w+margin)*3, start_y, w, h, "/", COL_OP};
    
    // Row 2: 4 5 6 *
    start_y += h + margin;
    buttons[4] = (Button){margin, start_y, w, h, "4", COL_BTN};
    buttons[5] = (Button){margin + (w+margin)*1, start_y, w, h, "5", COL_BTN};
    buttons[6] = (Button){margin + (w+margin)*2, start_y, w, h, "6", COL_BTN};
    buttons[7] = (Button){margin + (w+margin)*3, start_y, w, h, "*", COL_OP};
    
    // Row 3: 1 2 3 -
    start_y += h + margin;
    buttons[8] = (Button){margin, start_y, w, h, "1", COL_BTN};
    buttons[9] = (Button){margin + (w+margin)*1, start_y, w, h, "2", COL_BTN};
    buttons[10] = (Button){margin + (w+margin)*2, start_y, w, h, "3", COL_BTN};
    buttons[11] = (Button){margin + (w+margin)*3, start_y, w, h, "-", COL_OP};
    
    // Row 4: C 0 = +
    start_y += h + margin;
    buttons[12] = (Button){margin, start_y, w, h, "C", COL_ACCENT};
    buttons[13] = (Button){margin + (w+margin)*1, start_y, w, h, "0", COL_BTN};
    buttons[14] = (Button){margin + (w+margin)*2, start_y, w, h, "=", COL_ACCENT};
    buttons[15] = (Button){margin + (w+margin)*3, start_y, w, h, "+", COL_OP};
}

void draw_ui() {
    // Clear Background
    draw_rect(0, 0, 240, 300, COL_BG);
    
    // Draw Display Box
    draw_rect(10, 10, 220, 40, 0xFF3B4252);
    draw_text(display_buf, 20, 22, COL_TEXT);
    
    // Draw Buttons
    for (int i = 0; i < NUM_BTNS; i++) {
        draw_rect(buttons[i].x, buttons[i].y, buttons[i].w, buttons[i].h, buttons[i].color);
        // Center Text
        int text_x = buttons[i].x + (buttons[i].w / 2) - 4; // Approx
        int text_y = buttons[i].y + (buttons[i].h / 2) - 6;
        draw_text(buttons[i].label, text_x, text_y, COL_TEXT);
    }
}

int check_click(int x, int y) {
    for (int i = 0; i < NUM_BTNS; i++) {
        if (x >= buttons[i].x && x < buttons[i].x + buttons[i].w &&
            y >= buttons[i].y && y < buttons[i].y + buttons[i].h) {
            return i;
        }
    }
    return -1;
}

// Logic State
int current_val = 0;
int pending_val = 0;
char last_op = 0;
int new_input = 1; // Flag to clear display on next digit

void handle_logic(int btn_idx) {
    const char *lbl = buttons[btn_idx].label;
    char c = lbl[0];
    
    if (c >= '0' && c <= '9') {
        int digit = c - '0';
        if (new_input) {
            current_val = digit;
            new_input = 0;
        } else {
            current_val = current_val * 10 + digit;
        }
    }
    else if (c == 'C') {
        current_val = 0;
        pending_val = 0;
        last_op = 0;
        new_input = 1;
    }
    else if (c == '+' || c == '-' || c == '*' || c == '/') {
        pending_val = current_val;
        last_op = c;
        new_input = 1;
    }
    else if (c == '=') {
        if (last_op) {
            if (last_op == '+') current_val = pending_val + current_val;
            if (last_op == '-') current_val = pending_val - current_val;
            if (last_op == '*') current_val = pending_val * current_val;
            if (last_op == '/') {
                if (current_val != 0) current_val = pending_val / current_val;
                else current_val = 0; // Err
            }
            last_op = 0;
            new_input = 1;
        }
    }
    
    // Update Display
    itoa(current_val, display_buf);
    draw_ui();
}

int main() {
    create_window("Calculator", 100, 100, 240, 260);
    init_buttons();
    draw_ui();
    
    gui_event_t event;
    while(1) {
        if (get_event(&event)) {
            if (event.type == GUI_EVENT_MOUSE_DOWN) {
                // Adjust for title bar (handled by kernel offset in SYS_DRAW_RECT but NOT key events?)
                // Wait, SYS_DRAW_RECT adds offset. SYS_GET_EVENT returns RAW screen coords usually?
                // NO, get_event returns struct.
                // `gui.c` logic: `event->mouse.pos` IS ABSOLUTE SCREEN COORDS.
                // WE NEED RELATIVE COORDS for hit testing.
                // We don't know window position in userspace easily unless we track it or kernel sends relative.
                // Assumption: Events sent to userspace should be RELATIVE to window client area?
                // Plan: I should update `gui.c` to translate coordinates before sending.
                // BUT for now, I'll hardcode offset logic (assuming window didn't move much or we assume 0,0 is window origin?)
                // Wait, if I create window at 100,100...
                // And I click at 110, 110.
                // Event says 110, 110.
                // My button is at 10, 10.
                // I need to subtract window position.
                // userspace doesn't know window position if it moves!
                // FIX: Update `gui.c` to send RELATIVE coordinates.
                
                int btn = check_click(event.mouse.pos.x, event.mouse.pos.y);
                if (btn >= 0) {
                    handle_logic(btn);
                }
            }
        }
    }
    return 0;
}
