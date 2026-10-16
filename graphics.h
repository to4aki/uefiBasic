#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <efi.h>
#include <efilib.h>
#include <efiprot.h>


EFI_STATUS graphics_init(void);
void graphics_shutdown(void);

UINTN graphics_width(void);
UINTN graphics_height(void);


/*
 * Direct backbuffer access.
 *
 * The backbuffer pitch is expressed in UINT32 pixels.
 */
UINT32 *graphics_backbuffer(void);
UINTN graphics_backbuffer_pitch(void);


void graphics_put_pixel(
    INTN x,
    INTN y,
    UINT32 color);


void graphics_clear(
    UINT32 color);


void graphics_fill_rect(
    INTN x,
    INTN y,
    UINTN w,
    UINTN h,
    UINT32 color);


void graphics_present(void);


#endif

