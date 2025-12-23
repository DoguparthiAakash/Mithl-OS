#ifndef WINDOW_MANAGER_HPP
#define WINDOW_MANAGER_HPP

#include "Window.hpp"
#include "memory.h"
#include <stddef.h>

namespace mithl {
namespace wm {

// Simple vector-like container (since we can't use std::vector in kernel)
template<typename T>
class SimpleVector {
    T* data_;
    size_t size_;
    size_t capacity_;
    
public:
    SimpleVector() : data_(nullptr), size_(0), capacity_(0) {}
    
    ~SimpleVector() {
        if (data_) memory_free(data_);
    }
    
    void push_back(T item) {
        if (size_ >= capacity_) {
            size_t new_capacity = capacity_ == 0 ? 4 : capacity_ * 2;
            T* new_data = (T*)memory_alloc(new_capacity * sizeof(T));
            
            if (data_) {
                for (size_t i = 0; i < size_; i++) {
                    new_data[i] = data_[i];
                }
                memory_free(data_);
            }
            
            data_ = new_data;
            capacity_ = new_capacity;
        }
        
        data_[size_++] = item;
    }
    
    void remove(size_t index) {
        if (index >= size_) return;
        
        for (size_t i = index; i < size_ - 1; i++) {
            data_[i] = data_[i + 1];
        }
        size_--;
    }
    
    T& operator[](size_t index) { return data_[index]; }
    const T& operator[](size_t index) const { return data_[index]; }
    
    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    
    T* begin() { return data_; }
    T* end() { return data_ + size_; }
};

// WindowManager class
class WindowManager {
    SimpleVector<Window*> windows_;
    Window* focused_window_;
    Window* dragging_window_;
    int next_z_index_;
    
    // Singleton instance
    static WindowManager* instance_;
    
    WindowManager();
    
public:
    ~WindowManager();
    
    // Singleton access
    static WindowManager* get_instance();
    static void initialize();
    static void destroy();
    
    // Window management
    void add_window(Window* window);
    void remove_window(Window* window);
    void focus_window(Window* window);
    void bring_to_front(Window* window);
    
    // Rendering
    void render_all();
    void update_all(float delta_time);
    
    // Event handling
    void dispatch_event(const gui_event_t* event);
    
    // Getters
    Window* get_focused_window() const { return focused_window_; }
    Window* get_window_at(point_t p) const;
    size_t get_window_count() const { return windows_.size(); }
    
private:
    void sort_by_z_index();
};

} // namespace wm
} // namespace mithl

// C interface for compatibility
extern "C" {
    void wm_cpp_init(void);
    void wm_cpp_add_window(void* window);
    void wm_cpp_render(void);
    void wm_cpp_handle_event(const gui_event_t* event);
}

#endif // WINDOW_MANAGER_HPP
