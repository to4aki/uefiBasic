#include <efi.h>
#include <efilib.h>

#include "graphics.h"
#include "input.h"
#include "system.h"


#define FRAME_INTERVAL_100NS 166667ULL


EFI_STATUS
efi_main(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS status;

    EFI_EVENT timer_event;
    EFI_EVENT events[3];

    UINTN event_index;

    BOOLEAN running =
        TRUE;


    /* ======================================================
     * GNU-EFI
     * ====================================================== */

    InitializeLib(
        ImageHandle,
        SystemTable);


    /* ======================================================
     * Graphics
     * ====================================================== */

    status =
        graphics_init();

    if (EFI_ERROR(status))
    {
        Print(
            u"GRAPHICS INIT FAILED: %r\r\n",
            status);

        return status;
    }


    /* ======================================================
     * Input
     * ====================================================== */

    status =
        input_init();

    if (EFI_ERROR(status))
    {
        Print(
            u"INPUT INIT FAILED: %r\r\n",
            status);

        graphics_shutdown();

        return status;
    }


    /* ======================================================
     * System
     * ====================================================== */

    system_init();


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
        graphics_shutdown();

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
        uefi_call_wrapper(
            BS->CloseEvent,
            1,
            timer_event);

        graphics_shutdown();

        return status;
    }


    /* ======================================================
     * Events
     * ====================================================== */

    events[0] =
        input_pointer_event();

    events[1] =
        input_keyboard_event();

    events[2] =
        timer_event;


    /* ======================================================
     * Initial render
     * ====================================================== */

    system_render();


    /* ======================================================
     * Main loop
     * ====================================================== */

    while (running)
    {
        status =
            uefi_call_wrapper(
                BS->WaitForEvent,
                3,
                3,
                events,
                &event_index);


        if (EFI_ERROR(status))
            break;


        switch (event_index)
        {
            case 0:

                input_handle_pointer();

                break;


            case 1:

                running =
                    input_handle_keyboard();

                break;


            case 2:

                system_update();

                system_render();

                input_end_frame();

                break;
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


    graphics_shutdown();


    return EFI_SUCCESS;
}

