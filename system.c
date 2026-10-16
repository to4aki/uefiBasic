#include "system.h"
#include "graphics.h"
#include "input.h"
#include "textvram.h"


#define BUTTON_W 200
#define BUTTON_H 80


static UINT64 frame_count = 0;


/* ==========================================================
 * Initialize
 * ========================================================== */

void system_init(void)
{
    frame_count = 0;


    textvram_init();


    textvram_put_string(
        0,
        0,
        "UEFI SYSTEM",
        0x0000FF00,
        0x00000000);


    textvram_put_string(
        0,
        2,
        "TEXT VRAM 80X25",
        0x00FFFFFF,
        0x00000000);
}


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
 * Update
 * ========================================================== */

void system_update(void)
{
    const INPUT_STATE *input =
        input_state();


    /*
     * 今後ここへ
     *
     * CPU実行
     * TEXT VRAM更新
     * UI状態更新
     * デバイス更新
     *
     * を追加する。
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
 * Cursor
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
        color);

    graphics_fill_rect(
        x - 1,
        y - 10,
        3,
        21,
        color);
}


/* ==========================================================
 * Render
 * ========================================================== */

void system_render(void)
{
    INTN bx;
    INTN by;

    UINT32 button_color;
    UINT32 cursor_color;

    const INPUT_STATE *input =
        input_state();


    graphics_clear(
        0x00101020);

    textvram_render();


    /* ------------------------------------------------------
     * Button
     * ------------------------------------------------------ */

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
        button_color);


    /* ------------------------------------------------------
     * Cursor
     * ------------------------------------------------------ */

    if (input->mouse_down)
        cursor_color = 0x00FFFF00;
    else
        cursor_color = 0x00FFFFFF;


    draw_cursor(
        input->mouse_x,
        input->mouse_y,
        cursor_color);


    /* ------------------------------------------------------
     * Backbuffer -> GOP
     * ------------------------------------------------------ */

    graphics_present();
}


/* ==========================================================
 * Frame count
 * ========================================================== */

UINT64 system_frame_count(void)
{
    return frame_count;
}


