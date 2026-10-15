#include <efi.h>
#include <efilib.h>
#include <efiprot.h>


/* ==========================================================
 * Configuration
 * ========================================================== */

#define FRAME_INTERVAL_100NS  166667ULL   /* 約60Hz */


/* ==========================================================
 * Graphics
 * ========================================================== */

static EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;

static UINT32 *fb = NULL;

static UINTN screen_w = 0;
static UINTN screen_h = 0;
static UINTN stride = 0;


/* ==========================================================
 * Pointer
 * ========================================================== */

static EFI_ABSOLUTE_POINTER_PROTOCOL *pointer = NULL;

static UINT64 pointer_min_x = 0;
static UINT64 pointer_max_x = 0;

static UINT64 pointer_min_y = 0;
static UINT64 pointer_max_y = 0;


/* ==========================================================
 * System state
 * ========================================================== */

typedef struct
{
    INTN mouse_x;
    INTN mouse_y;

    BOOLEAN mouse_down;
    BOOLEAN mouse_pressed;
    BOOLEAN mouse_released;

    BOOLEAN running;

    UINT64 frame_count;

} SYSTEM_STATE;


static SYSTEM_STATE system_state;


/* ==========================================================
 * Graphics functions
 * ========================================================== */

static void put_pixel(
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

    fb[
        (UINTN)y * stride +
        (UINTN)x
    ] = color;
}


/* ---------------------------------------------------------- */

static void clear_screen(
    UINT32 color)
{
    UINTN x;
    UINTN y;

    for (y = 0; y < screen_h; y++)
    {
        UINT32 *line =
            fb + y * stride;

        for (x = 0; x < screen_w; x++)
        {
            line[x] = color;
        }
    }
}


/* ---------------------------------------------------------- */

static void fill_rect(
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
            put_pixel(
                x + (INTN)xx,
                y + (INTN)yy,
                color);
        }
    }
}


/* ---------------------------------------------------------- */

static void draw_cursor(
    INTN x,
    INTN y,
    UINT32 color)
{
    fill_rect(
        x - 10,
        y - 1,
        21,
        3,
        color);

    fill_rect(
        x - 1,
        y - 10,
        3,
        21,
        color);
}


/* ==========================================================
 * GUI test button
 * ========================================================== */

#define BUTTON_W 200
#define BUTTON_H 80


static BOOLEAN point_in_button(
    INTN x,
    INTN y)
{
    INTN bx;
    INTN by;

    bx =
        ((INTN)screen_w - BUTTON_W) / 2;

    by =
        ((INTN)screen_h - BUTTON_H) / 2;

    if (x < bx)
        return FALSE;

    if (x >= bx + BUTTON_W)
        return FALSE;

    if (y < by)
        return FALSE;

    if (y >= by + BUTTON_H)
        return FALSE;

    return TRUE;
}


/* ==========================================================
 * Pointer coordinate conversion
 * ========================================================== */

static INTN pointer_to_screen_x(
    UINT64 value)
{
    UINT64 range;
    UINT64 position;

    range =
        pointer_max_x -
        pointer_min_x;

    if (range == 0)
        return 0;

    if (value <= pointer_min_x)
        return 0;

    if (value >= pointer_max_x)
        return (INTN)screen_w - 1;

    position =
        value -
        pointer_min_x;

    return
        (INTN)(
            position *
            (screen_w - 1) /
            range
        );
}


/* ---------------------------------------------------------- */

static INTN pointer_to_screen_y(
    UINT64 value)
{
    UINT64 range;
    UINT64 position;

    range =
        pointer_max_y -
        pointer_min_y;

    if (range == 0)
        return 0;

    if (value <= pointer_min_y)
        return 0;

    if (value >= pointer_max_y)
        return (INTN)screen_h - 1;

    position =
        value -
        pointer_min_y;

    return
        (INTN)(
            position *
            (screen_h - 1) /
            range
        );
}


/* ==========================================================
 * Input
 * ========================================================== */

static void handle_pointer_input(void)
{
    EFI_STATUS status;

    EFI_ABSOLUTE_POINTER_STATE state;

    BOOLEAN old_down;


    status =
        uefi_call_wrapper(
            pointer->GetState,
            2,
            pointer,
            &state);

    if (EFI_ERROR(status))
        return;


    /* ------------------------------------------------------
     * Absolute coordinates
     * ------------------------------------------------------ */

    system_state.mouse_x =
        pointer_to_screen_x(
            state.CurrentX);

    system_state.mouse_y =
        pointer_to_screen_y(
            state.CurrentY);


    /* ------------------------------------------------------
     * Buttons
     * ------------------------------------------------------ */

    old_down =
        system_state.mouse_down;


    system_state.mouse_down =
        (state.ActiveButtons != 0);


    /*
     * Edge detection
     */

    if (!old_down &&
        system_state.mouse_down)
    {
        system_state.mouse_pressed =
            TRUE;
    }


    if (old_down &&
        !system_state.mouse_down)
    {
        system_state.mouse_released =
            TRUE;
    }
}


/* ---------------------------------------------------------- */

static void handle_keyboard_input(void)
{
    EFI_STATUS status;

    EFI_INPUT_KEY key;


    status =
        uefi_call_wrapper(
            ST->ConIn->ReadKeyStroke,
            2,
            ST->ConIn,
            &key);


    if (EFI_ERROR(status))
        return;


    if (key.ScanCode == SCAN_ESC)
    {
        system_state.running =
            FALSE;
    }
}


/* ==========================================================
 * System update
 * ========================================================== */

static void system_update(void)
{
    /*
     * ここがゲーム/エミュレータでいう tick()。
     *
     * 将来:
     *
     * CPU実行
     * UI更新
     * タイマー更新
     * デバイス更新
     *
     * などをここから呼ぶ。
     */


    /*
     * GUI click example
     */

    if (system_state.mouse_pressed)
    {
        if (point_in_button(
                system_state.mouse_x,
                system_state.mouse_y))
        {
            /*
             * Button clicked.
             *
             * 今は状態変更なし。
             * 実際のUI処理をここに入れる。
             */
        }
    }


    system_state.frame_count++;
}


/* ==========================================================
 * Render
 * ========================================================== */

static void system_render(void)
{
    INTN bx;
    INTN by;

    UINT32 button_color;
    UINT32 cursor_color;


    clear_screen(
        0x00101020);


    /* ------------------------------------------------------
     * Button
     * ------------------------------------------------------ */

    bx =
        ((INTN)screen_w - BUTTON_W) / 2;

    by =
        ((INTN)screen_h - BUTTON_H) / 2;


    button_color =
        0x00008000;


    /*
     * Hover
     */

    if (point_in_button(
            system_state.mouse_x,
            system_state.mouse_y))
    {
        button_color =
            0x0000C000;
    }


    /*
     * Press
     */

    if (system_state.mouse_down &&
        point_in_button(
            system_state.mouse_x,
            system_state.mouse_y))
    {
        button_color =
            0x00C08000;
    }


    fill_rect(
        bx,
        by,
        BUTTON_W,
        BUTTON_H,
        button_color);


    /* ------------------------------------------------------
     * Cursor
     * ------------------------------------------------------ */

    if (system_state.mouse_down)
    {
        cursor_color =
            0x00FFFF00;
    }
    else
    {
        cursor_color =
            0x00FFFFFF;
    }


    draw_cursor(
        system_state.mouse_x,
        system_state.mouse_y,
        cursor_color);
}


/* ==========================================================
 * End of frame
 * ========================================================== */

static void system_end_frame(void)
{
    /*
     * pressed / released は
     * 1フレームだけ有効。
     */

    system_state.mouse_pressed =
        FALSE;

    system_state.mouse_released =
        FALSE;
}


/* ==========================================================
 * Main
 * ========================================================== */

EFI_STATUS
efi_main(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS status;

    EFI_GUID gop_guid =
        EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

    EFI_GUID pointer_guid =
        EFI_ABSOLUTE_POINTER_PROTOCOL_GUID;

    EFI_EVENT timer_event;

    EFI_EVENT events[3];

    UINTN event_index;


    /* ------------------------------------------------------
     * GNU-EFI
     * ------------------------------------------------------ */

    InitializeLib(
        ImageHandle,
        SystemTable);


    /* ======================================================
     * GOP
     * ====================================================== */

    status =
        uefi_call_wrapper(
            BS->LocateProtocol,
            3,
            &gop_guid,
            NULL,
            (VOID **)&gop);


    if (EFI_ERROR(status))
    {
        Print(
            u"GOP NOT FOUND: %r\r\n",
            status);

        return status;
    }


    fb =
        (UINT32 *)
        gop->Mode->FrameBufferBase;


    screen_w =
        gop->Mode->Info->HorizontalResolution;


    screen_h =
        gop->Mode->Info->VerticalResolution;


    stride =
        gop->Mode->Info->PixelsPerScanLine;


    /* ======================================================
     * Absolute Pointer
     * ====================================================== */

    status =
        uefi_call_wrapper(
            BS->LocateProtocol,
            3,
            &pointer_guid,
            NULL,
            (VOID **)&pointer);


    if (EFI_ERROR(status))
    {
        Print(
            u"ABSOLUTE POINTER NOT FOUND: %r\r\n",
            status);

        return status;
    }


    status =
        uefi_call_wrapper(
            pointer->Reset,
            2,
            pointer,
            FALSE);


    if (EFI_ERROR(status))
    {
        Print(
            u"POINTER RESET FAILED: %r\r\n",
            status);

        return status;
    }


    pointer_min_x =
        pointer->Mode->AbsoluteMinX;

    pointer_max_x =
        pointer->Mode->AbsoluteMaxX;

    pointer_min_y =
        pointer->Mode->AbsoluteMinY;

    pointer_max_y =
        pointer->Mode->AbsoluteMaxY;


    /* ======================================================
     * Timer
     * ====================================================== */

    status =
        uefi_call_wrapper(
            BS->CreateEvent,
            5,
            EVT_TIMER,
            TPL_CALLBACK,
            NULL,
            NULL,
            &timer_event);


    if (EFI_ERROR(status))
    {
        Print(
            u"TIMER CREATE FAILED: %r\r\n",
            status);

        return status;
    }


    status =
        uefi_call_wrapper(
            BS->SetTimer,
            3,
            timer_event,
            TimerPeriodic,
            FRAME_INTERVAL_100NS);


    if (EFI_ERROR(status))
    {
        Print(
            u"TIMER START FAILED: %r\r\n",
            status);

        uefi_call_wrapper(
            BS->CloseEvent,
            1,
            timer_event);

        return status;
    }


    /* ======================================================
     * Initial state
     * ====================================================== */

    system_state.mouse_x =
        (INTN)screen_w / 2;

    system_state.mouse_y =
        (INTN)screen_h / 2;

    system_state.mouse_down =
        FALSE;

    system_state.mouse_pressed =
        FALSE;

    system_state.mouse_released =
        FALSE;

    system_state.frame_count =
        0;

    system_state.running =
        TRUE;


    /* ======================================================
     * Events
     * ====================================================== */

    /*
     * Event 0:
     * Absolute Pointer
     */

    events[0] =
        pointer->WaitForInput;


    /*
     * Event 1:
     * Keyboard
     */

    events[1] =
        ST->ConIn->WaitForKey;


    /*
     * Event 2:
     * 60Hz system timer
     */

    events[2] =
        timer_event;


    /* ======================================================
     * First render
     * ====================================================== */

    system_render();


    /* ======================================================
     * Main system loop
     * ====================================================== */

    while (system_state.running)
    {
        status =
            uefi_call_wrapper(
                BS->WaitForEvent,
                3,
                3,
                events,
                &event_index);


        if (EFI_ERROR(status))
        {
            break;
        }


        /* --------------------------------------------------
         * Pointer event
         * -------------------------------------------------- */

        if (event_index == 0)
        {
            handle_pointer_input();
        }


        /* --------------------------------------------------
         * Keyboard event
         * -------------------------------------------------- */

        else if (event_index == 1)
        {
            handle_keyboard_input();
        }


        /* --------------------------------------------------
         * System tick
         * -------------------------------------------------- */

        else if (event_index == 2)
        {
            /*
             * Update simulation / machine
             */

            system_update();


            /*
             * Draw current state
             */

            system_render();


            /*
             * Clear one-frame flags
             */

            system_end_frame();
        }
    }


    /* ======================================================
     * Cleanup
     * ====================================================== */

    uefi_call_wrapper(
        BS->SetTimer,
        3,
        timer_event,
        TimerCancel,
        0);


    uefi_call_wrapper(
        BS->CloseEvent,
        1,
        timer_event);


    return EFI_SUCCESS;
}

