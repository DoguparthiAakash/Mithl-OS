#include <stdlib.h>

int main() {
    print("Hello from Userspace App!\n");
    print("Creating a window...\n");
    
    create_window("Hello App", 150, 150, 300, 200);
    
    // Loop and handle events
    gui_event_t event;
    while(1) {
        if (get_event(&event)) {
            if (event.type == GUI_EVENT_MOUSE_DOWN) {
                print("Clicked!\n");
            }
        }
    }
    
    return 0;
}
