#include "spinlock.h"

// Basic Uniprocessor Spinlock Implementation
// Since Mithl-OS is currently single-threaded/UP in many places (or cooperative),
// exact atomics might not be strictly required yet, but we provide them for the API.

// GCC atomics
bool spinlock_acquire(lock_t *spin) {
    // Check if locked
    if (__atomic_test_and_set(&spin->lock, __ATOMIC_ACQUIRE)) {
        return false; // Already locked
    }
    return true; // Acquired
}

// Spin until acquired
void spinlock_acquire_or_wait(lock_t *spin) {
    while (__atomic_test_and_set(&spin->lock, __ATOMIC_ACQUIRE)) {
        // Spin
        __builtin_ia32_pause(); 
    }
}

void spinlock_drop(lock_t *spin) {
    __atomic_clear(&spin->lock, __ATOMIC_RELEASE);
}

uint32_t spinlock_acquire_irqsave(lock_t *spin) {
    uint32_t flags;
    __asm__ volatile("pushfl; pop %0" : "=r"(flags));
    __asm__ volatile("cli");
    
    spinlock_acquire_or_wait(spin);
    return flags;
}

void spinlock_release_irqrestore(lock_t *spin, uint32_t flags) {
    spinlock_drop(spin);
    __asm__ volatile("push %0; popfl" :: "r"(flags) : "memory", "cc");
}
