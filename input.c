#include "input.h"
#include "graphics.h"


static EFI_ABSOLUTE_POINTER_PROTOCOL *pointer = NULL;

static UINT64 pointer_min_x = 0;
static UINT64 pointer_max_x = 0;

static UINT64 pointer_min_y = 0;
static UINT64 pointer_max_y = 0;

static INPUT_STATE state;


/* ==========================================================
 * Coordinate conversion
 * ========================================================== */

static INTN pointer_to_screen_x(
    UINT64 value)
{
    UINT64 range;
    UINT64 position;

    UINTN screen_w =
        graphics_width();


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
            range);
}


static INTN pointer_to_screen_y(
    UINT64 value)
{
    UINT64 range;
    UINT64 position;

    UINTN screen_h =
        graphics_height();


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
            range);
}


/* ==========================================================
 * Initialize
 * ========================================================== */

EFI_STATUS input_init(void)
{
    EFI_STATUS status;

    EFI_GUID pointer_guid =
        EFI_ABSOLUTE_POINTER_PROTOCOL_GUID;


    status =
        uefi_call_wrapper(
            BS->LocateProtocol,
            3,
            &pointer_guid,
            NULL,
            (VOID **)&pointer);

    if (EFI_ERROR(status))
        return status;


    status =
        uefi_call_wrapper(
            pointer->Reset,
            2,
            pointer,
            FALSE);

    if (EFI_ERROR(status))
        return status;


    pointer_min_x =
        pointer->Mode->AbsoluteMinX;

    pointer_max_x =
        pointer->Mode->AbsoluteMaxX;

    pointer_min_y =
        pointer->Mode->AbsoluteMinY;

    pointer_max_y =
        pointer->Mode->AbsoluteMaxY;


    state.mouse_x =
        (INTN)graphics_width() / 2;

    state.mouse_y =
        (INTN)graphics_height() / 2;

    state.mouse_down = FALSE;
    state.mouse_pressed = FALSE;
    state.mouse_released = FALSE;


    return EFI_SUCCESS;
}


/* ==========================================================
 * Events
 * ========================================================== */

EFI_EVENT input_pointer_event(void)
{
    return pointer->WaitForInput;
}


EFI_EVENT input_keyboard_event(void)
{
    return ST->ConIn->WaitForKey;
}


/* ==========================================================
 * Pointer
 * ========================================================== */

void input_handle_pointer(void)
{
    EFI_STATUS status;

    EFI_ABSOLUTE_POINTER_STATE pointer_state;

    BOOLEAN old_down;


    status =
        uefi_call_wrapper(
            pointer->GetState,
            2,
            pointer,
            &pointer_state);

    if (EFI_ERROR(status))
        return;


    state.mouse_x =
        pointer_to_screen_x(
            pointer_state.CurrentX);

    state.mouse_y =
        pointer_to_screen_y(
            pointer_state.CurrentY);


    old_down =
        state.mouse_down;

    state.mouse_down =
        (pointer_state.ActiveButtons != 0);


    if (!old_down &&
        state.mouse_down)
    {
        state.mouse_pressed = TRUE;
    }


    if (old_down &&
        !state.mouse_down)
    {
        state.mouse_released = TRUE;
    }
}


/* ==========================================================
 * Keyboard
 *
 * FALSEを返したらシステム終了
 * ========================================================== */

BOOLEAN input_handle_keyboard(void)
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
        return TRUE;


    if (key.ScanCode == SCAN_ESC)
        return FALSE;


    return TRUE;
}


/* ==========================================================
 * End frame
 * ========================================================== */

void input_end_frame(void)
{
    state.mouse_pressed = FALSE;
    state.mouse_released = FALSE;
}


/* ==========================================================
 * State
 * ========================================================== */

const INPUT_STATE *input_state(void)
{
    return &state;
}


