#ifndef ACPI_H
#define ACPI_H

#include <stdint.h>

// 32-bit address (uintptr_t)
void acpi_set_rsdp(uint32_t addr);
void acpi_init(void);
void acpi_shutdown(void);

#endif
