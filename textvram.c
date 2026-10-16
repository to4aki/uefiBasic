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


    for (y = 1; y < TEXT_ROWS; y++)
    {
        for (x = 0; x < TEXT_COLS; x++)
        {
            vram[y - 1][x] =
                vram[y][x];
        }
    }


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
        current_background
    );
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
            background
        );


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
    if (character == '\n')
    {
        textvram_newline();

        return;
    }


    if (character == '\r')
    {
        cursor_x = 0;

        return;
    }


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
                current_background
            );
        }

        return;
    }


    textvram_put_char(
        cursor_x,
        cursor_y,
        character,
        current_foreground,
        current_background
    );


    cursor_x++;


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
            (UINT8)*text
        );

        text++;
    }
}


/* ==========================================================
 * Render one cell
 *
 * Direct backbuffer access.
 *
 * No graphics_put_pixel() calls are used here.
 * ========================================================== */

static void render_cell(
    INTN origin_x,
    INTN origin_y,
    UINTN cell_x,
    UINTN cell_y,
    const TEXT_CELL *cell)
{
    UINTN glyph_y;
    UINTN glyph_x;

    INTN px;
    INTN py;

    INTN screen_x;
    INTN screen_y;

    UINT8 bits;

    const UINT8 *glyph;

    UINT32 *buffer;
    UINT32 *line;

    UINTN pitch;
    UINTN screen_width;
    UINTN screen_height;


    buffer =
        graphics_backbuffer();

    if (buffer == NULL)
        return;


    pitch =
        graphics_backbuffer_pitch();

    screen_width =
        graphics_width();

    screen_height =
        graphics_height();


    px =
        origin_x +
        (INTN)(cell_x * TEXT_FONT_W);

    py =
        origin_y +
        (INTN)(cell_y * TEXT_FONT_H);


    /*
     * Fast rejection for cells completely outside
     * the screen.
     */

    if (px >= (INTN)screen_width)
        return;

    if (py >= (INTN)screen_height)
        return;

    if (px + TEXT_FONT_W <= 0)
        return;

    if (py + TEXT_FONT_H <= 0)
        return;


    glyph =
        font8x16_get(
            cell->character
        );


    for (glyph_y = 0;
         glyph_y < TEXT_FONT_H;
         glyph_y++)
    {
        screen_y =
            py +
            (INTN)glyph_y;


        if (screen_y < 0)
            continue;

        if ((UINTN)screen_y >= screen_height)
            continue;


        line =
            buffer +
            ((UINTN)screen_y * pitch);


        bits =
            glyph[glyph_y];


        for (glyph_x = 0;
             glyph_x < TEXT_FONT_W;
             glyph_x++)
        {
            screen_x =
                px +
                (INTN)glyph_x;


            if (screen_x < 0)
                continue;

            if ((UINTN)screen_x >= screen_width)
                continue;


            if (bits & (0x80U >> glyph_x))
            {
                line[(UINTN)screen_x] =
                    cell->foreground;
            }
            else
            {
                line[(UINTN)screen_x] =
                    cell->background;
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


    graphics_fill_rect(
        px,
        py + TEXT_FONT_H - 2,
        TEXT_FONT_W,
        2,
        color
    );
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
                &vram[y][x]
            );
        }
    }


    render_cursor(
        origin_x,
        origin_y
    );
}

