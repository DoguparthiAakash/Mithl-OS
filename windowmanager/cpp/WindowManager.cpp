#include "WindowManager.hpp"

extern "C" {
#include "console.h"
#include "memory.h"
}

namespace mithl {
namespace wm {

// Static instance
WindowManager* WindowManager::instance_ = nullptr;

// Constructor
WindowManager::WindowManager()
    : focused_window_(nullptr)
    , dragging_window_(nullptr)
    , next_z_index_(0)
{
    serial_write("[WM++] C++ WindowManager initialized\n");
}

// Destructor
WindowManager::~WindowManager() {
    // Clean up windows
    for (size_t i = 0; i < windows_.size(); i++) {
        delete windows_[i];
    }
}

// Singleton access
WindowManager* WindowManager::get_instance() {
    if (!instance_) {
        instance_ = new WindowManager();
    }
    return instance_;
}

void WindowManager::initialize() {
    get_instance();
}

void WindowManager::destroy() {
    if (instance_) {
        delete instance_;
        instance_ = nullptr;
    }
}

// Add window
void WindowManager::add_window(Window* window) {
    if (!window) return;
    
    window->set_z_index(next_z_index_++);
    windows_.push_back(window);
    
    // Focus new window
    focus_window(window);
    
    serial_write("[WM++] Window added\n");
}

// Remove window
void WindowManager::remove_window(Window* window) {
    for (size_t i = 0; i < windows_.size(); i++) {
        if (windows_[i] == window) {
            windows_.remove(i);
            
            if (focused_window_ == window) {
                focused_window_ = nullptr;
            }
            if (dragging_window_ == window) {
                dragging_window_ = nullptr;
            }
            
            delete window;
            break;
        }
    }
}

// Focus window
void WindowManager::focus_window(Window* window) {
    if (focused_window_) {
        focused_window_->set_focused(false);
    }
    
    focused_window_ = window;
    
    if (window) {
        window->set_focused(true);
        bring_to_front(window);
    }
}

// Bring window to front
void WindowManager::bring_to_front(Window* window) {
    if (!window) return;
    
    // Give it the highest z-index
    window->set_z_index(next_z_index_++);
    sort_by_z_index();
}

// Sort windows by z-index (simple bubble sort - fine for small number of windows)
void WindowManager::sort_by_z_index() {
    for (size_t i = 0; i < windows_.size(); i++) {
        for (size_t j = i + 1; j < windows_.size(); j++) {
            if (windows_[i]->get_z_index() > windows_[j]->get_z_index()) {
                Window* temp = windows_[i];
                windows_[i] = windows_[j];
                windows_[j] = temp;
            }
        }
    }
}

// Render all windows
void WindowManager::render_all() {
    // Sort by z-index first (back to front)
    sort_by_z_index();
    
    // Draw all windows
    for (size_t i = 0; i < windows_.size(); i++) {
        windows_[i]->draw();
    }
}

// Update all windows
void WindowManager::update_all(float delta_time) {
    for (size_t i = 0; i < windows_.size(); i++) {
        windows_[i]->update(delta_time);
    }
}

// Dispatch event to appropriate window
void WindowManager::dispatch_event(const gui_event_t* event) {
    if (event->type == GUI_EVENT_MOUSE_DOWN) {
        // Find window at mouse position (top to bottom)
        Window* clicked_window = get_window_at(event->mouse.pos);
        
        if (clicked_window) {
            focus_window(clicked_window);
            clicked_window->handle_event(event);
            
            // Check if window started dragging
            if (clicked_window->is_dragging()) {
                dragging_window_ = clicked_window;
            }
        }
    }
    else if (event->type == GUI_EVENT_MOUSE_MOVE) {
        if (dragging_window_) {
            dragging_window_->handle_event(event);
        }
    }
    else if (event->type == GUI_EVENT_MOUSE_UP) {
        if (dragging_window_) {
            dragging_window_->handle_event(event);
            dragging_window_ = nullptr;
        }
    }
    else if (event->type == GUI_EVENT_KEY_PRESS) {
        // Forward keyboard events to focused window
        if (focused_window_) {
            focused_window_->handle_event(event);
        }
    }
}

// Get window at point (top to bottom)
Window* WindowManager::get_window_at(point_t p) const {
    // Search from top to bottom (reverse order)
    for (int i = windows_.size() - 1; i >= 0; i--) {
        if (windows_[i]->is_visible() && windows_[i]->contains_point(p)) {
            return windows_[i];
        }
    }
    return nullptr;
}

} // namespace wm
} // namespace mithl

// C interface implementation
extern "C" {

void wm_cpp_init(void) {
    mithl::wm::WindowManager::initialize();
}

void wm_cpp_add_window(void* window) {
    mithl::wm::WindowManager::get_instance()->add_window(
        static_cast<mithl::wm::Window*>(window)
    );
}

void wm_cpp_render(void) {
    mithl::wm::WindowManager::get_instance()->render_all();
}

void wm_cpp_handle_event(const gui_event_t* event) {
    mithl::wm::WindowManager::get_instance()->dispatch_event(event);
}

}
