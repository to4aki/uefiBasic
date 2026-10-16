#include "input.h"
#include "graphics.h"


/* ==========================================================
 * Pointer protocols
 * ========================================================== */

static EFI_SIMPLE_POINTER_PROTOCOL *simple_pointer = NULL;

static EFI_ABSOLUTE_POINTER_PROTOCOL *absolute_pointer = NULL;


/* ==========================================================
 * Absolute pointer parameters
 * ========================================================== */

static UINT64 absolute_min_x = 0;
static UINT64 absolute_max_x = 0;

static UINT64 absolute_min_y = 0;
static UINT64 absolute_max_y = 0;


/* ==========================================================
 * Input state
 * ========================================================== */

static INPUT_STATE state;


/* ==========================================================
 * Keyboard queue
 * ========================================================== */

static INPUT_KEY_EVENT key_queue[INPUT_KEY_QUEUE_SIZE];

static UINTN key_queue_read = 0;
static UINTN key_queue_write = 0;
static UINTN key_queue_count = 0;


/* ==========================================================
 * Coordinate clamp
 * ========================================================== */

static INTN clamp_x(
    INTN x)
{
    INTN max_x;

    max_x =
        (INTN)graphics_width() - 1;


    if (x < 0)
        return 0;

    if (x > max_x)
        return max_x;


    return x;
}


static INTN clamp_y(
    INTN y)
{
    INTN max_y;

    max_y =
        (INTN)graphics_height() - 1;


    if (y < 0)
        return 0;

    if (y > max_y)
        return max_y;


    return y;
}


/* ==========================================================
 * Absolute pointer conversion
 * ========================================================== */

static INTN absolute_to_screen_x(
    UINT64 value)
{
    UINT64 range;
    UINT64 position;

    UINTN screen_w;


    screen_w =
        graphics_width();


    range =
        absolute_max_x -
        absolute_min_x;


    if (range == 0)
        return 0;


    if (value <= absolute_min_x)
        return 0;


    if (value >= absolute_max_x)
        return (INTN)screen_w - 1;


    position =
        value -
        absolute_min_x;


    return
        (INTN)(
            position *
            (screen_w - 1) /
            range
        );
}


static INTN absolute_to_screen_y(
    UINT64 value)
{
    UINT64 range;
    UINT64 position;

    UINTN screen_h;


    screen_h =
        graphics_height();


    range =
        absolute_max_y -
        absolute_min_y;


    if (range == 0)
        return 0;


    if (value <= absolute_min_y)
        return 0;


    if (value >= absolute_max_y)
        return (INTN)screen_h - 1;


    position =
        value -
        absolute_min_y;


    return
        (INTN)(
            position *
            (screen_h - 1) /
            range
        );
}


/* ==========================================================
 * Keyboard queue
 * ========================================================== */

static void key_queue_clear(void)
{
    key_queue_read = 0;
    key_queue_write = 0;
    key_queue_count = 0;
}


static BOOLEAN key_queue_push(
    CHAR16 unicode,
    UINT16 scan)
{
    if (key_queue_count >= INPUT_KEY_QUEUE_SIZE)
    {
        return FALSE;
    }


    key_queue[key_queue_write].unicode =
        unicode;

    key_queue[key_queue_write].scan =
        scan;


    key_queue_write++;


    if (key_queue_write >= INPUT_KEY_QUEUE_SIZE)
    {
        key_queue_write = 0;
    }


    key_queue_count++;


    return TRUE;
}


BOOLEAN input_key_available(void)
{
    return
        key_queue_count != 0;
}


BOOLEAN input_get_key(
    INPUT_KEY_EVENT *event)
{
    if (event == NULL)
        return FALSE;


    if (key_queue_count == 0)
        return FALSE;


    *event =
        key_queue[key_queue_read];


    key_queue_read++;


    if (key_queue_read >= INPUT_KEY_QUEUE_SIZE)
    {
        key_queue_read = 0;
    }


    key_queue_count--;


    return TRUE;
}


/* ==========================================================
 * Initialize
 * ========================================================== */

EFI_STATUS input_init(void)
{
    EFI_STATUS status;

    EFI_GUID simple_guid =
        EFI_SIMPLE_POINTER_PROTOCOL_GUID;

    EFI_GUID absolute_guid =
        EFI_ABSOLUTE_POINTER_PROTOCOL_GUID;


    simple_pointer = NULL;
    absolute_pointer = NULL;


    state.mouse_x =
        (INTN)graphics_width() / 2;

    state.mouse_y =
        (INTN)graphics_height() / 2;

    state.mouse_down =
        FALSE;

    state.mouse_pressed =
        FALSE;

    state.mouse_released =
        FALSE;


    key_queue_clear();


    /*
     * Simple Pointer
     */

    status =
        uefi_call_wrapper(
            BS->LocateProtocol,
            3,
            &simple_guid,
            NULL,
            (VOID **)&simple_pointer);


    if (!EFI_ERROR(status))
    {
        status =
            uefi_call_wrapper(
                simple_pointer->Reset,
                2,
                simple_pointer,
                FALSE);


        if (EFI_ERROR(status))
        {
            simple_pointer =
                NULL;
        }
    }
    else
    {
        simple_pointer =
            NULL;
    }


    /*
     * Absolute Pointer
     */

    status =
        uefi_call_wrapper(
            BS->LocateProtocol,
            3,
            &absolute_guid,
            NULL,
            (VOID **)&absolute_pointer);


    if (!EFI_ERROR(status))
    {
        status =
            uefi_call_wrapper(
                absolute_pointer->Reset,
                2,
                absolute_pointer,
                FALSE);


        if (!EFI_ERROR(status))
        {
            absolute_min_x =
                absolute_pointer->Mode->AbsoluteMinX;

            absolute_max_x =
                absolute_pointer->Mode->AbsoluteMaxX;

            absolute_min_y =
                absolute_pointer->Mode->AbsoluteMinY;

            absolute_max_y =
                absolute_pointer->Mode->AbsoluteMaxY;
        }
        else
        {
            absolute_pointer =
                NULL;
        }
    }
    else
    {
        absolute_pointer =
            NULL;
    }


    return EFI_SUCCESS;
}


/* ==========================================================
 * Availability
 * ========================================================== */

BOOLEAN input_has_simple_pointer(void)
{
    return
        simple_pointer != NULL;
}


BOOLEAN input_has_absolute_pointer(void)
{
    return
        absolute_pointer != NULL;
}


/* ==========================================================
 * Input events
 * ========================================================== */

EFI_EVENT input_simple_pointer_event(void)
{
    if (simple_pointer == NULL)
        return NULL;


    return
        simple_pointer->WaitForInput;
}


EFI_EVENT input_absolute_pointer_event(void)
{
    if (absolute_pointer == NULL)
        return NULL;


    return
        absolute_pointer->WaitForInput;
}


EFI_EVENT input_keyboard_event(void)
{
    return
        ST->ConIn->WaitForKey;
}


/* ==========================================================
 * Mouse button state
 * ========================================================== */

static void update_mouse_button(
    BOOLEAN new_down)
{
    BOOLEAN old_down;


    old_down =
        state.mouse_down;


    state.mouse_down =
        new_down;


    if (!old_down &&
        new_down)
    {
        state.mouse_pressed =
            TRUE;
    }


    if (old_down &&
        !new_down)
    {
        state.mouse_released =
            TRUE;
    }
}


/* ==========================================================
 * Simple Pointer
 * ========================================================== */

void input_handle_simple_pointer(void)
{
    EFI_STATUS status;

    EFI_SIMPLE_POINTER_STATE pointer_state;

    INTN dx;
    INTN dy;


    if (simple_pointer == NULL)
        return;


    status =
        uefi_call_wrapper(
            simple_pointer->GetState,
            2,
            simple_pointer,
            &pointer_state);


    if (EFI_ERROR(status))
        return;


    /*
     * Temporary scaling.
     */

    dx =
        (INTN)(
            pointer_state.RelativeMovementX /
            8
        );

    dy =
        (INTN)(
            pointer_state.RelativeMovementY /
            8
        );


    if (dx == 0 &&
        pointer_state.RelativeMovementX != 0)
    {
        if (pointer_state.RelativeMovementX > 0)
            dx = 1;
        else
            dx = -1;
    }


    if (dy == 0 &&
        pointer_state.RelativeMovementY != 0)
    {
        if (pointer_state.RelativeMovementY > 0)
            dy = 1;
        else
            dy = -1;
    }


    state.mouse_x =
        clamp_x(
            state.mouse_x + dx
        );


    state.mouse_y =
        clamp_y(
            state.mouse_y + dy
        );


    update_mouse_button(
        pointer_state.LeftButton
    );
}


/* ==========================================================
 * Absolute Pointer
 * ========================================================== */

void input_handle_absolute_pointer(void)
{
    EFI_STATUS status;

    EFI_ABSOLUTE_POINTER_STATE pointer_state;


    if (absolute_pointer == NULL)
        return;


    status =
        uefi_call_wrapper(
            absolute_pointer->GetState,
            2,
            absolute_pointer,
            &pointer_state);


    if (EFI_ERROR(status))
        return;


    state.mouse_x =
        absolute_to_screen_x(
            pointer_state.CurrentX
        );


    state.mouse_y =
        absolute_to_screen_y(
            pointer_state.CurrentY
        );


    update_mouse_button(
        pointer_state.ActiveButtons != 0
    );
}


/* ==========================================================
 * Keyboard
 * ========================================================== */

BOOLEAN input_handle_keyboard(void)
{
    EFI_STATUS status;

    EFI_INPUT_KEY key;


    /*
     * Read everything currently waiting in the
     * firmware keyboard buffer.
     */

    for (;;)
    {
        status =
            uefi_call_wrapper(
                ST->ConIn->ReadKeyStroke,
                2,
                ST->ConIn,
                &key);


        if (EFI_ERROR(status))
        {
            break;
        }


        /*
         * ESC exits immediately.
         */

        if (key.ScanCode == SCAN_ESC)
        {
            return FALSE;
        }


        /*
         * Queue the key.
         *
         * If our queue is full, the newest key
         * is dropped.
         */

        key_queue_push(
            key.UnicodeChar,
            key.ScanCode
        );
    }


    return TRUE;
}


/* ==========================================================
 * End frame
 * ========================================================== */

void input_end_frame(void)
{
    state.mouse_pressed =
        FALSE;

    state.mouse_released =
        FALSE;
}


/* ==========================================================
 * Current state
 * ========================================================== */

const INPUT_STATE *input_state(void)
{
    return &state;
}

