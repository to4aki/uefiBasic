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
            (VOID **)&gop
        );

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
            backbuffer_size
        );

    if (backbuffer == NULL)
        return EFI_OUT_OF_RESOURCES;


    SetMem(
        backbuffer,
        backbuffer_size,
        0
    );


    return EFI_SUCCESS;
}


/* ==========================================================
 * Shutdown
 * ========================================================== */

void graphics_shutdown(void)
{
    if (backbuffer != NULL)
    {
        FreePool(
            backbuffer
        );

        backbuffer =
            NULL;
    }


    fb = NULL;
    gop = NULL;

    screen_w = 0;
    screen_h = 0;
    stride = 0;

    backbuffer_size = 0;
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
 * Backbuffer information
 * ========================================================== */

UINT32 *graphics_backbuffer(void)
{
    return backbuffer;
}


UINTN graphics_backbuffer_pitch(void)
{
    return screen_w;
}


/* ==========================================================
 * Pixel
 * ========================================================== */

void graphics_put_pixel(
    INTN x,
    INTN y,
    UINT32 color)
{
    UINT32 *line;


    if (backbuffer == NULL)
        return;


    if (x < 0)
        return;

    if (y < 0)
        return;

    if ((UINTN)x >= screen_w)
        return;

    if ((UINTN)y >= screen_h)
        return;


    line =
        backbuffer +
        ((UINTN)y * screen_w);


    line[(UINTN)x] =
        color;
}


/* ==========================================================
 * Clear
 * ========================================================== */

void graphics_clear(
    UINT32 color)
{
    UINTN x;
    UINTN y;

    UINT32 *line;


    if (backbuffer == NULL)
        return;


    for (y = 0; y < screen_h; y++)
    {
        line =
            backbuffer +
            (y * screen_w);


        for (x = 0; x < screen_w; x++)
        {
            line[x] =
                color;
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
    INTN x0;
    INTN y0;

    INTN x1;
    INTN y1;

    INTN xx;
    INTN yy;

    UINT32 *line;


    if (backbuffer == NULL)
        return;


    if (w == 0)
        return;

    if (h == 0)
        return;


    x0 = x;
    y0 = y;

    x1 =
        x +
        (INTN)w;

    y1 =
        y +
        (INTN)h;


    /*
     * Completely outside the screen.
     */

    if (x1 <= 0)
        return;

    if (y1 <= 0)
        return;

    if (x0 >= (INTN)screen_w)
        return;

    if (y0 >= (INTN)screen_h)
        return;


    /*
     * Clip once.
     */

    if (x0 < 0)
        x0 = 0;

    if (y0 < 0)
        y0 = 0;

    if (x1 > (INTN)screen_w)
        x1 = (INTN)screen_w;

    if (y1 > (INTN)screen_h)
        y1 = (INTN)screen_h;


    if (x0 >= x1)
        return;

    if (y0 >= y1)
        return;


    /*
     * Direct backbuffer drawing.
     */

    for (yy = y0; yy < y1; yy++)
    {
        line =
            backbuffer +
            ((UINTN)yy * screen_w);


        for (xx = x0; xx < x1; xx++)
        {
            line[(UINTN)xx] =
                color;
        }
    }
}


/* ==========================================================
 * Present
 * ========================================================== */

void graphics_present(void)
{
    UINTN y;

    UINT32 *src;
    UINT32 *dst;

    UINTN line_size;


    if (fb == NULL)
        return;

    if (backbuffer == NULL)
        return;


    /*
     * Fast path.
     *
     * Framebuffer and backbuffer have the same pitch.
     * Copy the complete frame in one operation.
     */

    if (stride == screen_w)
    {
        CopyMem(
            fb,
            backbuffer,
            backbuffer_size
        );

        return;
    }


    /*
     * Framebuffer has padding between scanlines.
     */

    line_size =
        screen_w *
        sizeof(UINT32);


    src =
        backbuffer;

    dst =
        fb;


    for (y = 0; y < screen_h; y++)
    {
        CopyMem(
            dst,
            src,
            line_size
        );


        src +=
            screen_w;

        dst +=
            stride;
    }
}


