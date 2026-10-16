#include "system.h"
#include "graphics.h"
#include "input.h"
#include "textvram.h"
#include "hardware.h"
#include "fileio.h"
#include "z80emu.h"


static BOOLEAN z80_running = FALSE;
static UINT32 z80_clock_remainder = 0;

#define Z80_CLOCK_HZ       4000000U
#define SYSTEM_FRAME_HZ    60U

#define BUTTON_W 200
#define BUTTON_H 80


static UINT64 frame_count = 0;


/* ==========================================================
 * Print unsigned integer
 * ========================================================== */

static void print_uint64(
    UINT64 value)
{
    char buffer[32];

    UINTN pos;


    if (value == 0)
    {
        textvram_putc(
            '0'
        );

        return;
    }


    pos = 0;


    while (value != 0 &&
           pos < sizeof(buffer))
    {
        buffer[pos] =
            (char)(
                '0' +
                (value % 10)
            );

        pos++;

        value /=
            10;
    }


    while (pos > 0)
    {
        pos--;

        textvram_putc(
            (UINT8)buffer[pos]
        );
    }
}


/* ==========================================================
 * Print hardware information
 * ========================================================== */

static void print_hardware_info(void)
{
    const HARDWARE_INFO *hw;

    UINT64 total_mb;
    UINT64 available_mb;


    hw =
        hardware_info();


    total_mb =
        hw->memory_total_bytes /
        (1024ULL * 1024ULL);


    available_mb =
        hw->memory_available_bytes /
        (1024ULL * 1024ULL);


    textvram_set_color(
        0x0000FF00,
        0x00000000
    );


    textvram_print(
        "UEFI SYSTEM\n"
    );


    textvram_set_color(
        0x00FFFFFF,
        0x00000000
    );


    textvram_print(
        "\nCPU\n"
    );


    textvram_print(
        " Vendor   : "
    );

    textvram_print(
        hw->cpu_vendor
    );

    textvram_putc(
        '\n'
    );


    textvram_print(
        " Brand    : "
    );

    textvram_print(
        hw->cpu_brand
    );

    textvram_putc(
        '\n'
    );


    textvram_print(
        " Family   : "
    );

    print_uint64(
        hw->cpu_family
    );


    textvram_print(
        "\n Model    : "
    );

    print_uint64(
        hw->cpu_model
    );


    textvram_print(
        "\n Stepping : "
    );

    print_uint64(
        hw->cpu_stepping
    );


    textvram_print(
        "\n\nMEMORY\n"
    );


    textvram_print(
        " Total    : "
    );

    print_uint64(
        total_mb
    );

    textvram_print(
        " MB\n"
    );


    textvram_print(
        " Free     : "
    );

    print_uint64(
        available_mb
    );

    textvram_print(
        " MB\n"
    );


    textvram_print(
        "\nGRAPHICS\n"
    );


    textvram_print(
        " Width    : "
    );

    print_uint64(
        graphics_width()
    );


    textvram_print(
        "\n Height   : "
    );

    print_uint64(
        graphics_height()
    );


    textvram_print(
        "\n\nINPUT\n"
    );


    textvram_print(
        " Simple   : "
    );

    if (input_has_simple_pointer())
    {
        textvram_print(
            "YES\n"
        );
    }
    else
    {
        textvram_print(
            "NO\n"
        );
    }


    textvram_print(
        " Absolute : "
    );

    if (input_has_absolute_pointer())
    {
        textvram_print(
            "YES\n"
        );
    }
    else
    {
        textvram_print(
            "NO\n"
        );
    }


    textvram_print(
        "\nREADY> "
    );
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
    EFI_STATUS status;


    frame_count =
        0;


    textvram_init();


    print_hardware_info();


    /*
     * Load RC2014 32K ROM from Intel HEX file.
     */

    status =
        z80emu_load_hex(
            L"\\Z80\\rc2014_32k.hex"
        );


    if (EFI_ERROR(status))
    {
        z80_running =
            FALSE;


        textvram_print(
            "\nRC2014 ROM LOAD FAILED\n"
        );


        return;
    }


    /*
     * Reset Z80.
     * PC starts from address 0x0000.
     */

    z80emu_reset();


    z80_running =
        TRUE;


    textvram_print(
        "\nRC2014 ROM LOADED\n"
    );


    textvram_print(
        "Z80 RUNNING\n"
    );
}


/* ==========================================================
 * Keyboard
 * ========================================================== */

static void process_keyboard(void)
{
    INPUT_KEY_EVENT key;

    CHAR16 ch;


    while (input_get_key(&key))
    {
        ch =
            key.unicode;


        if (ch == '\r')
        {
            textvram_putc(
                '\n'
            );

            continue;
        }


        if (ch == '\b')
        {
            textvram_putc(
                '\b'
            );

            continue;
        }


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
    UINT32 z80_cycles;


    /*
     * Z80が開始されていなければ実行しない。
     */

    if (z80_running)
    {
        /*
         * 4MHzを60Hzに分配する。
         *
         * 4000000 / 60 = 66666 remainder 40
         */

        z80_cycles =
            Z80_CLOCK_HZ /
            SYSTEM_FRAME_HZ;


        /*
         * 整数除算で失われる余りを累積する。
         */

        z80_clock_remainder +=
            Z80_CLOCK_HZ %
            SYSTEM_FRAME_HZ;


        /*
         * 1 T-state分たまったら、
         * 今回の実行量へ追加する。
         */

        if (z80_clock_remainder >=
            SYSTEM_FRAME_HZ)
        {
            z80_clock_remainder -=
                SYSTEM_FRAME_HZ;

            z80_cycles++;
        }


        /*
         * Z80を指定T-state分実行する。
         *
         * 命令境界による超過については
         * z80emu_run_cycles() 内の
         * cycle_debt が補正する。
         */

        z80emu_run_cycles(
            (int)z80_cycles
        );
    }


    /*
     * System frame counter.
     */

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


    graphics_clear(
        0x00101020
    );


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


    graphics_present();
}


/* ==========================================================
 * Frame count
 * ========================================================== */

UINT64 system_frame_count(void)
{
    return frame_count;
}

