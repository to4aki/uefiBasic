#include "system.h"
#include "graphics.h"
#include "input.h"
#include "textvram.h"


#define BUTTON_W 200
#define BUTTON_H 80


static UINT64 frame_count = 0;


/* ==========================================================
 * GUI
 * ========================================================== */

static BOOLEAN point_in_button(
    INTN x,
    INTN y)
{
    INTN bx;
    INTN by;


    bx =
        ((INTN)graphics_width() -
         BUTTON_W) / 2;

    by =
        ((INTN)graphics_height() -
         BUTTON_H) / 2;


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
 * Mouse cursor
 * ========================================================== */

static void draw_cursor(
    INTN x,
    INTN y,
    UINT32 color)
{
    graphics_fill_rect(
        x - 10,
        y - 1,
        21,
        3,
        color
    );


    graphics_fill_rect(
        x - 1,
        y - 10,
        3,
        21,
        color
    );
}


/* ==========================================================
 * Initialize
 * ========================================================== */

void system_init(void)
{
    frame_count = 0;


    textvram_init();


    textvram_set_color(
        0x0000FF00,
        0x00000000
    );


    textvram_print(
        "UEFI SYSTEM\n"
    );


    textvram_print(
        "\n"
    );


    textvram_set_color(
        0x00FFFFFF,
        0x00000000
    );


    textvram_print(
        "TEXT VRAM 80X25\n"
    );


    textvram_print(
        "READY> "
    );
}


/* ==========================================================
 * Keyboard processing
 * ========================================================== */

static void process_keyboard(void)
{
    INPUT_KEY_EVENT key;

    CHAR16 ch;


    /*
     * Process every key currently queued.
     */

    while (input_get_key(&key))
    {
        ch =
            key.unicode;


        /*
         * Enter
         */

        if (ch == '\r')
        {
            textvram_putc(
                '\n'
            );

            continue;
        }


        /*
         * Backspace
         */

        if (ch == '\b')
        {
            textvram_putc(
                '\b'
            );

            continue;
        }


        /*
         * Printable ASCII
         */

        if (ch >= 0x20 &&
            ch <= 0x7E)
        {
            textvram_putc(
                (UINT8)ch
            );
        }
    }
}


/* ==========================================================
 * Update
 * ========================================================== */

void system_update(void)
{
    const INPUT_STATE *input;


    input =
        input_state();


    /*
     * Keyboard
     */

    process_keyboard();


    /*
     * GUI
     */

    if (input->mouse_pressed)
    {
        if (point_in_button(
                input->mouse_x,
                input->mouse_y))
        {
            /*
             * Button clicked.
             */
        }
    }


    frame_count++;
}


/* ==========================================================
 * Render
 * ========================================================== */

void system_render(void)
{
    INTN bx;
    INTN by;

    INTN text_x;
    INTN text_y;

    UINT32 button_color;
    UINT32 cursor_color;

    const INPUT_STATE *input;


    input =
        input_state();


    /*
     * Background
     */

    graphics_clear(
        0x00101020
    );


    /*
     * TEXT VRAM
     */

    text_x =
        ((INTN)graphics_width() -
         TEXT_WIDTH) / 2;


    text_y =
        ((INTN)graphics_height() -
         TEXT_HEIGHT) / 2;


    textvram_render(
        text_x,
        text_y
    );


    /*
     * GUI button
     */

    bx =
        ((INTN)graphics_width() -
         BUTTON_W) / 2;


    by =
        ((INTN)graphics_height() -
         BUTTON_H) / 2;


    button_color =
        0x00008000;


    if (point_in_button(
            input->mouse_x,
            input->mouse_y))
    {
        button_color =
            0x0000C000;
    }


    if (input->mouse_down &&
        point_in_button(
            input->mouse_x,
            input->mouse_y))
    {
        button_color =
            0x00C08000;
    }


    graphics_fill_rect(
        bx,
        by,
        BUTTON_W,
        BUTTON_H,
        button_color
    );


    /*
     * Mouse cursor
     */

    if (input->mouse_down)
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
        input->mouse_x,
        input->mouse_y,
        cursor_color
    );


    /*
     * Present
     */

    graphics_present();
}


/* ==========================================================
 * Frame count
 * ========================================================== */

UINT64 system_frame_count(void)
{
    return frame_count;
}

