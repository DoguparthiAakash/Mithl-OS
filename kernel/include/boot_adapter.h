#ifndef BOOT_ADAPTER_H
#define BOOT_ADAPTER_H

#include <stdint.h>
#include "boot_info.h"

int parse_multiboot(uint32_t magic, void* addr, boot_info_t* info);

#endif
