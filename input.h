#ifndef INPUT_H
#define INPUT_H

#include <efi.h>
#include <efilib.h>
#include <efiprot.h>


#define INPUT_KEY_QUEUE_SIZE 32


typedef struct
{
    CHAR16 unicode;
    UINT16 scan;

} INPUT_KEY_EVENT;


typedef struct
{
    INTN mouse_x;
    INTN mouse_y;

    BOOLEAN mouse_down;
    BOOLEAN mouse_pressed;
    BOOLEAN mouse_released;

} INPUT_STATE;


EFI_STATUS input_init(void);


BOOLEAN input_has_simple_pointer(void);
BOOLEAN input_has_absolute_pointer(void);


EFI_EVENT input_simple_pointer_event(void);
EFI_EVENT input_absolute_pointer_event(void);
EFI_EVENT input_keyboard_event(void);


void input_handle_simple_pointer(void);
void input_handle_absolute_pointer(void);

BOOLEAN input_handle_keyboard(void);


/*
 * Keyboard queue
 */

BOOLEAN input_key_available(void);

BOOLEAN input_get_key(
    INPUT_KEY_EVENT *event);


/*
 * End of frame
 */

void input_end_frame(void);


/*
 * Current input state
 */

const INPUT_STATE *input_state(void);


#endif

