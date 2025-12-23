#ifndef WINDOW_HPP
#define WINDOW_HPP

extern "C" {
#include "gui.h"
#include "graphics.h"
}

#include <stdint.h>

namespace mithl {
namespace wm {

// Forward declarations
class WindowManager;
class Animation;

// Window state
enum class WindowState {
    Normal,
    Maximized,
    Minimized,
    Fullscreen
};

// Base Window class
class Window {
protected:
    rect_t bounds_;
    rect_t restore_bounds_;  // For restoring from maximized
    char title_[128];
    bool visible_;
    bool focused_;
    int z_index_;
    WindowState state_;
    
    // Animation state
    float animation_progress_;
    bool is_animating_;
    
    // Drag state
    bool is_dragging_;
    point_t drag_offset_;
    
public:
    Window(const char* title, int x, int y, int width, int height);
    virtual ~Window() = default;
    
    // Core functionality
    virtual void draw();
    virtual void handle_event(const gui_event_t* event);
    virtual void update(float delta_time);
    
    // Window operations
    void maximize();
    void minimize();
    void restore();
    void close();
    
    // Drag operations
    void start_drag(point_t mouse_pos);
    void update_drag(point_t mouse_pos);
    void end_drag();
    
    // Getters
    const rect_t& get_bounds() const { return bounds_; }
    const char* get_title() const { return title_; }
    bool is_visible() const { return visible_; }
    bool is_focused() const { return focused_; }
    int get_z_index() const { return z_index_; }
    WindowState get_state() const { return state_; }
    bool is_dragging() const { return is_dragging_; }
    
    // Setters
    void set_bounds(const rect_t& bounds);
    void set_visible(bool visible) { visible_ = visible; }
    void set_focused(bool focused) { focused_ = focused; }
    void set_z_index(int z) { z_index_ = z; }
    
    // Utility
    bool contains_point(point_t p) const;
    rect_t get_title_bar_rect() const;
    rect_t get_content_rect() const;
    
protected:
    void draw_title_bar();
    void draw_content();
    void draw_shadow();
};

// Application window (has content panel)
class ApplicationWindow : public Window {
    gui_panel_t* content_panel_;
    
public:
    ApplicationWindow(const char* title, int x, int y, int width, int height);
    ~ApplicationWindow() override;
    
    void draw() override;
    void handle_event(const gui_event_t* event) override;
    
    gui_panel_t* get_content_panel() { return content_panel_; }
};

} // namespace wm
} // namespace mithl

#endif // WINDOW_HPP
