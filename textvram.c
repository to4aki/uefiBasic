#include "textvram.h"
#include "graphics.h"
#include "font8x16.h"


/* ==========================================================
 * TEXT VRAM
 * ========================================================== */

static TEXT_CELL vram[TEXT_ROWS][TEXT_COLS];


/* ==========================================================
 * Initialize
 * ========================================================== */

void textvram_init(void)
{
    textvram_clear(
        0x00FFFFFF,
        0x00000000);
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
}


/* ==========================================================
 * Put character
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
 * Put string
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


    /*
     * TEXT VRAM内部座標
     *
     *      +
     * origin
     */

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
 * Render TEXT VRAM
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
}

