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


EFI_STATUS input_init(void);

EFI_EVENT input_pointer_event(void);
EFI_EVENT input_keyboard_event(void);

void input_handle_pointer(void);
BOOLEAN input_handle_keyboard(void);

void input_end_frame(void);

const INPUT_STATE *input_state(void);

#endif

