#ifndef HAL_CPU_H
#define HAL_CPU_H

#include "types.h"
#include <stddef.h>

// CPU Feature Detection
int cpu_has_sse(void);
int cpu_has_avx(void);
int cpu_has_avx2(void);
int cpu_has_aes(void);

// CPU Control
void cpu_enable_sse(void);
void cpu_enable_avx(void);
void cpu_enable_fpu(void);

// MSR Access
uint64_t cpu_rdmsr(uint32_t msr);
void cpu_wrmsr(uint32_t msr, uint64_t value);

// Atomic Operations
uint64_t atomic_add(volatile uint64_t *ptr, uint64_t value);
uint64_t atomic_sub(volatile uint64_t *ptr, uint64_t value);
uint64_t atomic_cmpxchg(volatile uint64_t *ptr, uint64_t old_val, uint64_t new_val);
void atomic_inc(volatile uint64_t *ptr);
void atomic_dec(volatile uint64_t *ptr);

// Memory Barriers
void memory_barrier(void);
void read_barrier(void);
void write_barrier(void);

// SIMD Operations
void simd_memcpy_sse(void *dest, const void *src, size_t size);
void simd_memcpy_avx(void *dest, const void *src, size_t size);

// Port I/O
void outb(uint16_t port, uint8_t value);
void outw(uint16_t port, uint16_t value);
void outl(uint16_t port, uint32_t value);
uint8_t inb(uint16_t port);
uint16_t inw(uint16_t port);
uint32_t inl(uint16_t port);

#endif // HAL_CPU_H
