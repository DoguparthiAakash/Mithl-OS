#include "Window.hpp"
#include "console.h"

extern "C" {
#include "memory.h"
#include "string.h"

// Declare C global variables (must be outside namespace)
extern struct gui_manager gui_mgr;
extern gui_renderer_t renderer;
}

namespace mithl {
namespace wm {

// Window constructor
Window::Window(const char* title, int x, int y, int width, int height)
    : bounds_{x, y, width, height}
    , restore_bounds_{x, y, width, height}
    , visible_(true)
    , focused_(false)
    , z_index_(0)
    , state_(WindowState::Normal)
    , animation_progress_(0.0f)
    , is_animating_(false)
    , is_dragging_(false)
    , drag_offset_{0, 0}
{
    strncpy(title_, title, 127);
    title_[127] = '\0';
}

// Draw window
void Window::draw() {
    if (!visible_) return;
    
    // Draw shadow first (behind window)
    draw_shadow();
    
    // Draw window background
    draw_rect_filled(bounds_, 0xFFF5F5F5);
    
    // Draw title bar
    draw_title_bar();
    
    // Draw content
    draw_content();
    
    // Draw border
    uint32_t border_color = focused_ ? 0xFF0078D4 : 0xFFCCCCCC;
    draw_rect_outline(bounds_.x, bounds_.y, bounds_.width, bounds_.height, border_color);
}

// Draw title bar
void Window::draw_title_bar() {
    rect_t title_bar = get_title_bar_rect();
    
    // Title bar background
    uint32_t bg_color = focused_ ? 0xFFE0E0E0 : 0xFFF0F0F0;
    draw_rect_filled(title_bar, bg_color);
    
    // Title text
    draw_text_sf_mono(title_, title_bar.x + 10, title_bar.y + 8, 0xFF000000);
    
    // Traffic light buttons (macOS style)
    int btn_y = title_bar.y + 8;
    int btn_x = title_bar.x + title_bar.width - 70;
    
    // Close button (red)
    draw_circle_filled(btn_x, btn_y, 6, 0xFFFF5F56);
    btn_x += 20;
    
    // Minimize button (yellow)
    draw_circle_filled(btn_x, btn_y, 6, 0xFFFFBD2E);
    btn_x += 20;
    
    // Maximize button (green)
    draw_circle_filled(btn_x, btn_y, 6, 0xFF27C93F);
}

// Draw content (override in subclasses)
void Window::draw_content() {
    rect_t content = get_content_rect();
    draw_rect_filled(content, 0xFFFFFFFF);
}

// Draw shadow
void Window::draw_shadow() {
    // Simple shadow effect - draw darker rectangles offset
    const int shadow_offset = 4;
    const int shadow_blur = 8;
    
    for (int i = shadow_blur; i > 0; i--) {
        uint8_t alpha = (255 * i) / (shadow_blur * 2);
        uint32_t shadow_color = (alpha << 24);
        
        rect_t shadow = {
            bounds_.x + shadow_offset - i,
            bounds_.y + shadow_offset - i,
            bounds_.width + i * 2,
            bounds_.height + i * 2
        };
        
        draw_rect_filled(shadow, shadow_color);
    }
}

// Handle events
void Window::handle_event(const gui_event_t* event) {
    if (!visible_) return;
    
    if (event->type == GUI_EVENT_MOUSE_DOWN) {
        point_t mouse = event->mouse.pos;
        
        // Check if click is in title bar
        rect_t title_bar = get_title_bar_rect();
        if (mouse.x >= title_bar.x && mouse.x < title_bar.x + title_bar.width &&
            mouse.y >= title_bar.y && mouse.y < title_bar.y + title_bar.height) {
            
            // Check traffic light buttons
            int btn_y = title_bar.y + 8;
            int btn_x = title_bar.x + title_bar.width - 70;
            
            // Close button
            if (mouse.x >= btn_x - 6 && mouse.x <= btn_x + 6 &&
                mouse.y >= btn_y - 6 && mouse.y <= btn_y + 6) {
                close();
                return;
            }
            
            btn_x += 20;
            // Minimize button
            if (mouse.x >= btn_x - 6 && mouse.x <= btn_x + 6 &&
                mouse.y >= btn_y - 6 && mouse.y <= btn_y + 6) {
                minimize();
                return;
            }
            
            btn_x += 20;
            // Maximize button
            if (mouse.x >= btn_x - 6 && mouse.x <= btn_x + 6 &&
                mouse.y >= btn_y - 6 && mouse.y <= btn_y + 6) {
                if (state_ == WindowState::Maximized) {
                    restore();
                } else {
                    maximize();
                }
                return;
            }
            
            // Start dragging
            start_drag(mouse);
        }
    }
    else if (event->type == GUI_EVENT_MOUSE_MOVE && is_dragging_) {
        update_drag(event->mouse.pos);
    }
    else if (event->type == GUI_EVENT_MOUSE_UP && is_dragging_) {
        end_drag();
    }
}

// Update (for animations)
void Window::update(float delta_time) {
    if (is_animating_) {
        animation_progress_ += delta_time;
        if (animation_progress_ >= 1.0f) {
            animation_progress_ = 1.0f;
            is_animating_ = false;
        }
    }
}

// Window operations
void Window::maximize() {
    if (state_ == WindowState::Maximized) return;
    
    restore_bounds_ = bounds_;
    state_ = WindowState::Maximized;
    
    bounds_ = {0, 30, (int16_t)::gui_mgr.screen_width, (int16_t)(::gui_mgr.screen_height - 30)};
    
    is_animating_ = true;
    animation_progress_ = 0.0f;
}

void Window::minimize() {
    state_ = WindowState::Minimized;
    visible_ = false;
}

void Window::restore() {
    if (state_ == WindowState::Maximized) {
        bounds_ = restore_bounds_;
    }
    state_ = WindowState::Normal;
    visible_ = true;
}

void Window::close() {
    visible_ = false;
    // WindowManager will handle actual removal
}

// Drag operations
void Window::start_drag(point_t mouse_pos) {
    is_dragging_ = true;
    drag_offset_.x = mouse_pos.x - bounds_.x;
    drag_offset_.y = mouse_pos.y - bounds_.y;
}

void Window::update_drag(point_t mouse_pos) {
    if (!is_dragging_) return;
    
    bounds_.x = mouse_pos.x - drag_offset_.x;
    bounds_.y = mouse_pos.y - drag_offset_.y;
    
    // Request redraw
    ::gui_mgr.needs_redraw = 1;
}

void Window::end_drag() {
    is_dragging_ = false;
}

// Utility functions
bool Window::contains_point(point_t p) const {
    return p.x >= bounds_.x && p.x < bounds_.x + bounds_.width &&
           p.y >= bounds_.y && p.y < bounds_.y + bounds_.height;
}

rect_t Window::get_title_bar_rect() const {
    return {bounds_.x, bounds_.y, bounds_.width, 30};
}

rect_t Window::get_content_rect() const {
    return {bounds_.x, bounds_.y + 30, bounds_.width, bounds_.height - 30};
}

void Window::set_bounds(const rect_t& bounds) {
    bounds_ = bounds;
}

// ApplicationWindow implementation
ApplicationWindow::ApplicationWindow(const char* title, int x, int y, int width, int height)
    : Window(title, x, y, width, height)
    , content_panel_(nullptr)
{
    // Create content panel
    content_panel_ = gui_create_panel(x, y + 30, width, height - 30);
}

ApplicationWindow::~ApplicationWindow() {
    // Cleanup panel if needed
}

void ApplicationWindow::draw() {
    // Draw base window
    Window::draw();
    
    // Draw content panel if it exists
    if (content_panel_) {
        gui_element_t* element = (gui_element_t*)content_panel_;
        if (element->draw) {
            element->draw(&::renderer, element);
        }
    }
}

void ApplicationWindow::handle_event(const gui_event_t* event) {
    // Handle window-level events first
    Window::handle_event(event);
    
    // Forward events to content panel
    if (content_panel_) {
        gui_element_t* element = (gui_element_t*)content_panel_;
        if (element->event_handler) {
            element->event_handler(element, (gui_event_t*)event);
        }
    }
}

} // namespace wm
} // namespace mithl
