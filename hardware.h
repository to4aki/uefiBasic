#ifndef HARDWARE_H
#define HARDWARE_H

#include <efi.h>
#include <efilib.h>


typedef struct
{
    char cpu_vendor[16];
    char cpu_brand[64];

    UINT32 cpu_family;
    UINT32 cpu_model;
    UINT32 cpu_stepping;

    UINT64 memory_total_bytes;
    UINT64 memory_available_bytes;

} HARDWARE_INFO;


EFI_STATUS hardware_detect(void);

const HARDWARE_INFO *hardware_info(void);


#endif

