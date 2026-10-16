#include "textvram.h"
#include "graphics.h"
#include "font8x16.h"


/* ==========================================================
 * TEXT VRAM
 * ========================================================== */

static TEXT_CELL vram[TEXT_ROWS][TEXT_COLS];


/* ==========================================================
 * Cursor
 * ========================================================== */

static UINTN cursor_x = 0;
static UINTN cursor_y = 0;

static BOOLEAN cursor_visible = TRUE;


/* ==========================================================
 * Current color
 * ========================================================== */

static UINT32 current_foreground =
    0x00FFFFFF;

static UINT32 current_background =
    0x00000000;


/* ==========================================================
 * Scroll
 * ========================================================== */

static void textvram_scroll(void)
{
    UINTN x;
    UINTN y;


    /*
     * Row 1 -> Row 0
     * Row 2 -> Row 1
     * ...
     */

    for (y = 1; y < TEXT_ROWS; y++)
    {
        for (x = 0; x < TEXT_COLS; x++)
        {
            vram[y - 1][x] =
                vram[y][x];
        }
    }


    /*
     * Clear last row
     */

    for (x = 0; x < TEXT_COLS; x++)
    {
        vram[TEXT_ROWS - 1][x].character =
            ' ';

        vram[TEXT_ROWS - 1][x].foreground =
            current_foreground;

        vram[TEXT_ROWS - 1][x].background =
            current_background;
    }
}


/* ==========================================================
 * New line
 * ========================================================== */

static void textvram_newline(void)
{
    cursor_x = 0;

    cursor_y++;


    if (cursor_y >= TEXT_ROWS)
    {
        textvram_scroll();

        cursor_y =
            TEXT_ROWS - 1;
    }
}


/* ==========================================================
 * Initialize
 * ========================================================== */

void textvram_init(void)
{
    current_foreground =
        0x00FFFFFF;

    current_background =
        0x00000000;


    cursor_x = 0;
    cursor_y = 0;

    cursor_visible =
        TRUE;


    textvram_clear(
        current_foreground,
        current_background);
}


/* ==========================================================
 * Clear
 * ========================================================== */

void textvram_clear(
    UINT32 foreground,
    UINT32 background)
{
    UINTN x;
    UINTN y;


    current_foreground =
        foreground;

    current_background =
        background;


    for (y = 0; y < TEXT_ROWS; y++)
    {
        for (x = 0; x < TEXT_COLS; x++)
        {
            vram[y][x].character =
                ' ';

            vram[y][x].foreground =
                foreground;

            vram[y][x].background =
                background;
        }
    }


    cursor_x = 0;
    cursor_y = 0;
}


/* ==========================================================
 * Color
 * ========================================================== */

void textvram_set_color(
    UINT32 foreground,
    UINT32 background)
{
    current_foreground =
        foreground;

    current_background =
        background;
}


/* ==========================================================
 * Direct character
 * ========================================================== */

void textvram_put_char(
    UINTN x,
    UINTN y,
    UINT8 character,
    UINT32 foreground,
    UINT32 background)
{
    if (x >= TEXT_COLS)
        return;

    if (y >= TEXT_ROWS)
        return;


    vram[y][x].character =
        character;

    vram[y][x].foreground =
        foreground;

    vram[y][x].background =
        background;
}


/* ==========================================================
 * Direct string
 * ========================================================== */

void textvram_put_string(
    UINTN x,
    UINTN y,
    const char *text,
    UINT32 foreground,
    UINT32 background)
{
    if (text == NULL)
        return;


    while (*text != '\0')
    {
        if (x >= TEXT_COLS)
            break;


        textvram_put_char(
            x,
            y,
            (UINT8)*text,
            foreground,
            background);


        x++;
        text++;
    }
}


/* ==========================================================
 * Get cell
 * ========================================================== */

const TEXT_CELL *textvram_get_cell(
    UINTN x,
    UINTN y)
{
    if (x >= TEXT_COLS)
        return NULL;

    if (y >= TEXT_ROWS)
        return NULL;


    return &vram[y][x];
}


/* ==========================================================
 * Cursor
 * ========================================================== */

void textvram_set_cursor(
    UINTN x,
    UINTN y)
{
    if (x >= TEXT_COLS)
        x = TEXT_COLS - 1;

    if (y >= TEXT_ROWS)
        y = TEXT_ROWS - 1;


    cursor_x = x;
    cursor_y = y;
}


void textvram_get_cursor(
    UINTN *x,
    UINTN *y)
{
    if (x != NULL)
        *x = cursor_x;

    if (y != NULL)
        *y = cursor_y;
}


void textvram_show_cursor(
    BOOLEAN visible)
{
    cursor_visible =
        visible;
}


/* ==========================================================
 * Terminal character output
 * ========================================================== */

void textvram_putc(
    UINT8 character)
{
    /*
     * Line Feed
     */

    if (character == '\n')
    {
        textvram_newline();

        return;
    }


    /*
     * Carriage Return
     */

    if (character == '\r')
    {
        cursor_x = 0;

        return;
    }


    /*
     * Backspace
     */

    if (character == '\b')
    {
        if (cursor_x > 0)
        {
            cursor_x--;

            textvram_put_char(
                cursor_x,
                cursor_y,
                ' ',
                current_foreground,
                current_background);
        }

        return;
    }


    /*
     * Normal character
     */

    textvram_put_char(
        cursor_x,
        cursor_y,
        character,
        current_foreground,
        current_background);


    cursor_x++;


    /*
     * Automatic wrap
     */

    if (cursor_x >= TEXT_COLS)
    {
        textvram_newline();
    }
}


/* ==========================================================
 * Terminal string output
 * ========================================================== */

void textvram_print(
    const char *text)
{
    if (text == NULL)
        return;


    while (*text != '\0')
    {
        textvram_putc(
            (UINT8)*text);

        text++;
    }
}


/* ==========================================================
 * Render one cell
 * ========================================================== */

static void render_cell(
    INTN origin_x,
    INTN origin_y,
    UINTN cell_x,
    UINTN cell_y,
    const TEXT_CELL *cell)
{
    UINTN x;
    UINTN y;

    INTN px;
    INTN py;

    UINT8 bits;

    const UINT8 *glyph;


    px =
        origin_x +
        (INTN)(cell_x * TEXT_FONT_W);

    py =
        origin_y +
        (INTN)(cell_y * TEXT_FONT_H);


    glyph =
        font8x16_get(
            cell->character);


    for (y = 0; y < TEXT_FONT_H; y++)
    {
        bits =
            glyph[y];


        for (x = 0; x < TEXT_FONT_W; x++)
        {
            if (bits & (0x80 >> x))
            {
                graphics_put_pixel(
                    px + (INTN)x,
                    py + (INTN)y,
                    cell->foreground);
            }
            else
            {
                graphics_put_pixel(
                    px + (INTN)x,
                    py + (INTN)y,
                    cell->background);
            }
        }
    }
}


/* ==========================================================
 * Render cursor
 * ========================================================== */

static void render_cursor(
    INTN origin_x,
    INTN origin_y)
{
    INTN px;
    INTN py;

    UINT32 color;


    if (!cursor_visible)
        return;


    px =
        origin_x +
        (INTN)(cursor_x * TEXT_FONT_W);

    py =
        origin_y +
        (INTN)(cursor_y * TEXT_FONT_H);


    color =
        current_foreground;


    /*
     * Underline cursor
     *
     * 8 x 2 pixels
     */

    graphics_fill_rect(
        px,
        py + TEXT_FONT_H - 2,
        TEXT_FONT_W,
        2,
        color);
}


/* ==========================================================
 * Render
 * ========================================================== */

void textvram_render(
    INTN origin_x,
    INTN origin_y)
{
    UINTN x;
    UINTN y;


    for (y = 0; y < TEXT_ROWS; y++)
    {
        for (x = 0; x < TEXT_COLS; x++)
        {
            render_cell(
                origin_x,
                origin_y,
                x,
                y,
                &vram[y][x]);
        }
    }


    /*
     * TEXT cursor is drawn after character cells.
     */

    render_cursor(
        origin_x,
        origin_y);
}

