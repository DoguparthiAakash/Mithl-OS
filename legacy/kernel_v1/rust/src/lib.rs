#![no_std]
pub mod shell;
// #![feature(alloc_error_handler)] // Removed for stable

use core::panic::PanicInfo;

#[no_mangle]
pub extern "C" fn rust_init() {
    // Placeholder for interaction
}

extern "C" {
    fn memory_alloc(size: usize) -> *mut u8;
    fn memory_free(ptr: *mut u8);
    fn serial_write(s: *const u8); // C function takes null-term string
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    unsafe {
        serial_write(b"\n\n!!! RUST PANIC !!!\n\0".as_ptr());
        // Simple attempt to print location (optional, requires fmt)
        // For now just halt visible
    }
    loop {}
}

#[no_mangle]
pub extern "C" fn rust_eh_personality() {}



// Minimal Allocator scaffolding
// To use 'alloc' crate, we need to define a GlobalAlloc.
// Since we are on stable (likely), we can implement GlobalAlloc.

extern crate alloc;
use alloc::alloc::{GlobalAlloc, Layout};

struct KernelAllocator;

unsafe impl GlobalAlloc for KernelAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        memory_alloc(layout.size())
    }

    unsafe fn dealloc(&self, ptr: *mut u8, _layout: Layout) {
        memory_free(ptr)
    }
}

#[global_allocator]
static ALLOCATOR: KernelAllocator = KernelAllocator;
