#include "graphics.h"


static EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;

static UINT32 *fb = NULL;
static UINT32 *backbuffer = NULL;

static UINTN screen_w = 0;
static UINTN screen_h = 0;
static UINTN stride = 0;

static UINTN backbuffer_size = 0;


/* ==========================================================
 * Initialize
 * ========================================================== */

EFI_STATUS graphics_init(void)
{
    EFI_STATUS status;

    EFI_GUID gop_guid =
        EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;


    status =
        uefi_call_wrapper(
            BS->LocateProtocol,
            3,
            &gop_guid,
            NULL,
            (VOID **)&gop);

    if (EFI_ERROR(status))
        return status;


    fb =
        (UINT32 *)
        gop->Mode->FrameBufferBase;

    screen_w =
        gop->Mode->Info->HorizontalResolution;

    screen_h =
        gop->Mode->Info->VerticalResolution;

    stride =
        gop->Mode->Info->PixelsPerScanLine;


    backbuffer_size =
        screen_w *
        screen_h *
        sizeof(UINT32);


    backbuffer =
        AllocatePool(
            backbuffer_size);

    if (backbuffer == NULL)
        return EFI_OUT_OF_RESOURCES;


    SetMem(
        backbuffer,
        backbuffer_size,
        0);


    return EFI_SUCCESS;
}


/* ==========================================================
 * Shutdown
 * ========================================================== */

void graphics_shutdown(void)
{
    if (backbuffer != NULL)
    {
        FreePool(backbuffer);
        backbuffer = NULL;
    }
}


/* ==========================================================
 * Screen information
 * ========================================================== */

UINTN graphics_width(void)
{
    return screen_w;
}


UINTN graphics_height(void)
{
    return screen_h;
}


/* ==========================================================
 * Pixel
 * ========================================================== */

void graphics_put_pixel(
    INTN x,
    INTN y,
    UINT32 color)
{
    if (x < 0 || y < 0)
        return;

    if ((UINTN)x >= screen_w)
        return;

    if ((UINTN)y >= screen_h)
        return;


    backbuffer[
        (UINTN)y * screen_w +
        (UINTN)x
    ] = color;
}


/* ==========================================================
 * Clear
 * ========================================================== */

void graphics_clear(
    UINT32 color)
{
    UINTN x;
    UINTN y;


    for (y = 0; y < screen_h; y++)
    {
        UINT32 *line =
            backbuffer + y * screen_w;

        for (x = 0; x < screen_w; x++)
        {
            line[x] = color;
        }
    }
}


/* ==========================================================
 * Rectangle
 * ========================================================== */

void graphics_fill_rect(
    INTN x,
    INTN y,
    UINTN w,
    UINTN h,
    UINT32 color)
{
    UINTN xx;
    UINTN yy;


    for (yy = 0; yy < h; yy++)
    {
        for (xx = 0; xx < w; xx++)
        {
            graphics_put_pixel(
                x + (INTN)xx,
                y + (INTN)yy,
                color);
        }
    }
}


/* ==========================================================
 * Present
 * ========================================================== */

void graphics_present(void)
{
    UINTN y;


    for (y = 0; y < screen_h; y++)
    {
        CopyMem(
            fb + y * stride,
            backbuffer + y * screen_w,
            screen_w * sizeof(UINT32));
    }
}

