#ifndef HAL_TEST_H
#define HAL_TEST_H

// Run all HAL tests
void hal_run_all_tests(void);

// Individual test functions
void hal_test_cpu_features(void);
void hal_test_simd_memcpy(void);
void hal_test_atomics(void);
void hal_test_memory_barriers(void);
void hal_test_msr_access(void);

#endif // HAL_TEST_H
