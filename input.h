#ifndef INPUT_H
#define INPUT_H

#include <efi.h>
#include <efilib.h>
#include <efiprot.h>


typedef struct
{
    INTN mouse_x;
    INTN mouse_y;

    BOOLEAN mouse_down;
    BOOLEAN mouse_pressed;
    BOOLEAN mouse_released;

} INPUT_STATE;


/*
 * Initialize input devices.
 *
 * Pointer devices are optional.
 */
EFI_STATUS input_init(void);


/*
 * Available pointer devices.
 */
BOOLEAN input_has_simple_pointer(void);
BOOLEAN input_has_absolute_pointer(void);


/*
 * Input events.
 */
EFI_EVENT input_simple_pointer_event(void);
EFI_EVENT input_absolute_pointer_event(void);
EFI_EVENT input_keyboard_event(void);


/*
 * Input handlers.
 */
void input_handle_simple_pointer(void);
void input_handle_absolute_pointer(void);

BOOLEAN input_handle_keyboard(void);


/*
 * End-of-frame processing.
 */
void input_end_frame(void);


/*
 * Current input state.
 */
const INPUT_STATE *input_state(void);


#endif

