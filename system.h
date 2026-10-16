#ifndef SYSTEM_H
#define SYSTEM_H

#include <efi.h>

void system_init(void);

void system_update(void);
void system_render(void);

UINT64 system_frame_count(void);

#endif

