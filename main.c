#include <efi.h>
#include <efilib.h>

#include "graphics.h"
#include "input.h"
#include "system.h"
#include "hardware.h"


#define FRAME_INTERVAL_100NS 166667ULL


EFI_STATUS
efi_main(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS status;

    EFI_EVENT timer_event;

    EFI_EVENT events[4];

    UINTN event_count;
    UINTN event_index;

    UINTN simple_pointer_index;
    UINTN absolute_pointer_index;
    UINTN keyboard_index;
    UINTN timer_index;

    BOOLEAN running;
    BOOLEAN render_required;


    running =
        TRUE;


    /* ======================================================
     * GNU-EFI
     * ====================================================== */

    InitializeLib(
        ImageHandle,
        SystemTable
    );


    /* ======================================================
     * Graphics
     * ====================================================== */

    status =
        graphics_init();


    if (EFI_ERROR(status))
    {
        Print(
            u"GRAPHICS INIT FAILED: %r\r\n",
            status
        );

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
            status
        );

        graphics_shutdown();

        return status;
    }

    /* ======================================================
     * Hardware
     * ====================================================== */

     hardware_detect();


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
            &timer_event
        );


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
            FRAME_INTERVAL_100NS
        );


    if (EFI_ERROR(status))
    {
        uefi_call_wrapper(
            BS->CloseEvent,
            1,
            timer_event
        );

        graphics_shutdown();

        return status;
    }


    /* ======================================================
     * Event list
     * ====================================================== */

    event_count = 0;


    simple_pointer_index =
        (UINTN)-1;

    absolute_pointer_index =
        (UINTN)-1;


    /*
     * Simple Pointer
     */

    if (input_has_simple_pointer())
    {
        simple_pointer_index =
            event_count;

        events[event_count] =
            input_simple_pointer_event();

        event_count++;
    }


    /*
     * Absolute Pointer
     */

    if (input_has_absolute_pointer())
    {
        absolute_pointer_index =
            event_count;

        events[event_count] =
            input_absolute_pointer_event();

        event_count++;
    }


    /*
     * Keyboard
     */

    keyboard_index =
        event_count;

    events[event_count] =
        input_keyboard_event();

    event_count++;


    /*
     * Timer
     */

    timer_index =
        event_count;

    events[event_count] =
        timer_event;

    event_count++;


    /* ======================================================
     * Initial render
     * ====================================================== */

    system_render();


    /* ======================================================
     * Main loop
     * ====================================================== */

    while (running)
    {
        render_required =
            FALSE;


        status =
            uefi_call_wrapper(
                BS->WaitForEvent,
                3,
                event_count,
                events,
                &event_index
            );


        if (EFI_ERROR(status))
            break;


        /*
         * --------------------------------------------------
         * Simple Pointer
         * --------------------------------------------------
         */

        if (input_has_simple_pointer() &&
            event_index == simple_pointer_index)
        {
            input_handle_simple_pointer();

            render_required =
                TRUE;
        }


        /*
         * --------------------------------------------------
         * Absolute Pointer
         * --------------------------------------------------
         */

        else if (
            input_has_absolute_pointer() &&
            event_index == absolute_pointer_index)
        {
            input_handle_absolute_pointer();

            render_required =
                TRUE;
        }


        /*
         * --------------------------------------------------
         * Keyboard
         * --------------------------------------------------
         */

        else if (
            event_index ==
            keyboard_index)
        {
            running =
                input_handle_keyboard();


            if (running)
            {
                /*
                 * Keyboard data is stored in the queue.
                 *
                 * Run system_update() now so the queued
                 * characters are consumed immediately.
                 */

                system_update();

                render_required =
                    TRUE;
            }
        }


        /*
         * --------------------------------------------------
         * System tick
         *
         * Timer remains the system clock.
         *
         * It no longer forces rendering every 16.67 ms.
         * --------------------------------------------------
         */

        else if (
            event_index ==
            timer_index)
        {
            system_update();


            /*
             * No unconditional render here.
             *
             * The timer remains available for future
             * periodic system processing.
             */
        }


        /*
         * --------------------------------------------------
         * Render
         *
         * Rendering is still FULL FRAME.
         * This is not dirty rectangle rendering.
         * --------------------------------------------------
         */

        if (render_required)
        {
            system_render();
        }


        /*
         * One-shot input states are consumed after the
         * current event has been processed.
         */

        input_end_frame();
    }


    /* ======================================================
     * Cleanup
     * ====================================================== */

    uefi_call_wrapper(
        BS->SetTimer,
        3,
        timer_event,
        TimerCancel,
        0
    );


    uefi_call_wrapper(
        BS->CloseEvent,
        1,
        timer_event
    );


    graphics_shutdown();


    return EFI_SUCCESS;
}

