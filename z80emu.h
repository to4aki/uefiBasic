#ifndef Z80EMU_H
#define Z80EMU_H

#include <efi.h>


EFI_STATUS z80emu_init(void);

void z80emu_shutdown(void);

void z80emu_reset(void);

void z80emu_run_cycles(
    int cycles
);

void z80emu_interrupt(void);

UINT8 *z80emu_memory(void);


EFI_STATUS z80emu_load(
    const CHAR16 *path,
    UINT16 address
);


EFI_STATUS z80emu_load_hex(
    const CHAR16 *path
);


void z80emu_test(void);


#endif

