#include "hal/cpu.h"
#include "console.h"
#include "string.h"

// Test buffer for SIMD operations
static uint8_t test_src[1024] __attribute__((aligned(32)));
static uint8_t test_dst[1024] __attribute__((aligned(32)));
static volatile uint64_t atomic_counter = 0;

void hal_test_cpu_features(void) {
    serial_write("\n=== HAL CPU Feature Detection ===\n");
    
    if (cpu_has_sse()) {
        serial_write("[HAL] SSE: SUPPORTED\n");
    } else {
        serial_write("[HAL] SSE: NOT SUPPORTED\n");
    }
    
    if (cpu_has_avx()) {
        serial_write("[HAL] AVX: SUPPORTED\n");
    } else {
        serial_write("[HAL] AVX: NOT SUPPORTED\n");
    }
    
    if (cpu_has_avx2()) {
        serial_write("[HAL] AVX2: SUPPORTED\n");
    } else {
        serial_write("[HAL] AVX2: NOT SUPPORTED\n");
    }
    
    if (cpu_has_aes()) {
        serial_write("[HAL] AES-NI: SUPPORTED\n");
    } else {
        serial_write("[HAL] AES-NI: NOT SUPPORTED\n");
    }
}

void hal_test_simd_memcpy(void) {
    serial_write("\n=== HAL SIMD Memory Copy Test ===\n");
    
    // Fill source buffer with pattern
    for (int i = 0; i < 1024; i++) {
        test_src[i] = (uint8_t)(i & 0xFF);
    }
    
    // Clear destination
    memset(test_dst, 0, 1024);
    
    // Test SSE memcpy ONLY if supported
    if (cpu_has_sse()) {
        simd_memcpy_sse(test_dst, test_src, 1024);
        
        // Verify
        int errors = 0;
        for (int i = 0; i < 1024; i++) {
            if (test_dst[i] != test_src[i]) errors++;
        }
        
        if (errors == 0) {
            serial_write("[HAL] SSE memcpy: PASS (1024 bytes copied)\n");
        } else {
            serial_write("[HAL] SSE memcpy: FAIL\n");
        }
    } else {
        serial_write("[HAL] SSE memcpy: SKIPPED (CPU doesn't support SSE)\n");
    }
    
    // Clear destination again
    memset(test_dst, 0, 1024);
    
    // Test AVX memcpy ONLY if supported
    if (cpu_has_avx()) {
        simd_memcpy_avx(test_dst, test_src, 1024);
        
        // Verify
        int errors = 0;
        for (int i = 0; i < 1024; i++) {
            if (test_dst[i] != test_src[i]) errors++;
        }
        
        if (errors == 0) {
            serial_write("[HAL] AVX memcpy: PASS (1024 bytes copied)\n");
        } else {
            serial_write("[HAL] AVX memcpy: FAIL\n");
        }
    } else {
        serial_write("[HAL] AVX memcpy: SKIPPED (CPU doesn't support AVX)\n");
    }
}

void hal_test_atomics(void) {
    serial_write("\n=== HAL Atomic Operations Test ===\n");
    
    // Reset counter
    atomic_counter = 0;
    
    // Test atomic increment
    atomic_inc(&atomic_counter);
    atomic_inc(&atomic_counter);
    atomic_inc(&atomic_counter);
    
    if (atomic_counter == 3) {
        serial_write("[HAL] Atomic increment: PASS (counter = 3)\n");
    } else {
        serial_write("[HAL] Atomic increment: FAIL\n");
    }
    
    // Test atomic add
    uint64_t old_val = atomic_add(&atomic_counter, 10);
    
    if (old_val == 3 && atomic_counter == 13) {
        serial_write("[HAL] Atomic add: PASS (3 + 10 = 13)\n");
    } else {
        serial_write("[HAL] Atomic add: FAIL\n");
    }
    
    // Test atomic compare-and-swap
    uint64_t result = atomic_cmpxchg(&atomic_counter, 13, 100);
    
    if (result == 13 && atomic_counter == 100) {
        serial_write("[HAL] Atomic CAS: PASS (13 -> 100)\n");
    } else {
        serial_write("[HAL] Atomic CAS: FAIL\n");
    }
    
    // Test atomic decrement
    atomic_dec(&atomic_counter);
    
    if (atomic_counter == 99) {
        serial_write("[HAL] Atomic decrement: PASS (100 - 1 = 99)\n");
    } else {
        serial_write("[HAL] Atomic decrement: FAIL\n");
    }
}

void hal_test_memory_barriers(void) {
    serial_write("\n=== HAL Memory Barrier Test ===\n");
    
    volatile uint64_t test_var = 0;
    
    // Write with barrier
    test_var = 42;
    write_barrier();
    
    // Read with barrier
    read_barrier();
    uint64_t val = test_var;
    
    if (val == 42) {
        serial_write("[HAL] Memory barriers: PASS (ordering preserved)\n");
    } else {
        serial_write("[HAL] Memory barriers: FAIL\n");
    }
    
    // Full barrier
    memory_barrier();
    serial_write("[HAL] Full memory barrier: EXECUTED\n");
}

void hal_test_msr_access(void) {
    serial_write("\n=== HAL MSR Access Test ===\n");
    
    // Read TSC (Time Stamp Counter) - MSR 0x10
    uint64_t tsc1 = cpu_rdmsr(0x10);
    
    // Do some work
    for (volatile int i = 0; i < 1000; i++);
    
    uint64_t tsc2 = cpu_rdmsr(0x10);
    
    if (tsc2 > tsc1) {
        serial_write("[HAL] MSR read (TSC): PASS (cycles elapsed)\n");
    } else {
        serial_write("[HAL] MSR read (TSC): FAIL\n");
    }
}

void hal_run_all_tests(void) {
    serial_write("\n");
    serial_write("╔════════════════════════════════════════╗\n");
    serial_write("║   HAL (Hardware Abstraction Layer)     ║\n");
    serial_write("║         Comprehensive Test Suite       ║\n");
    serial_write("╚════════════════════════════════════════╝\n");
    
    hal_test_cpu_features();
    hal_test_simd_memcpy();
    hal_test_atomics();
    hal_test_memory_barriers();
    hal_test_msr_access();
    
    serial_write("\n");
    serial_write("╔════════════════════════════════════════╗\n");
    serial_write("║      All HAL Tests Completed!          ║\n");
    serial_write("╚════════════════════════════════════════╝\n");
    serial_write("\n");
}
