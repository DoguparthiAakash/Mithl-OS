// cxx_runtime.cpp
// Minimal C++ runtime support for the kernel (free store, pure virtuals, guards)

#include <stddef.h>

extern "C" {
    // Pure virtual function call handler
    void __cxa_pure_virtual() {
        // Panic or hang
        while (1) { __asm__ volatile("hlt"); }
    }

    // Static object destruction (not really used in kernel usually if no exit)
    int __cxa_atexit(void (*destructor) (void *), void *arg, void *dso_handle) {
         (void)destructor; (void)arg; (void)dso_handle;
         return 0;
    }
    
    // Guard variables for static initialization
    int __cxa_guard_acquire(long long *guard) {
        if (*((volatile char *)guard) == 0) {
            return 1;
        }
        return 0;
    }
    
    void __cxa_guard_release(long long *guard) {
        *((volatile char *)guard) = 1;
    }
    
    void __cxa_guard_abort(long long *guard) {
        (void)guard;
    }
}

// Global New/Delete operators (map to malloc/free)
// Assuming we have memory.h/malloc via extern "C"
extern "C" void *memory_alloc(size_t size);
extern "C" void memory_free(void *ptr);

void *operator new(size_t size) {
    return memory_alloc(size);
}

void *operator new[](size_t size) {
    return memory_alloc(size);
}

void operator delete(void *p) {
    memory_free(p);
}

void operator delete(void *p, size_t size) {
    (void)size;
    memory_free(p);
}

void operator delete[](void *p) {
    memory_free(p);
}

void operator delete[](void *p, size_t size) {
    (void)size;
    memory_free(p);
}

extern "C" void call_constructors(void) {
    // Dummy implementation. 
    // In a real C++ kernel, we would walk the .ctors/.init_array section here.
    // Since we have no global objects yet, this is safe.
}
