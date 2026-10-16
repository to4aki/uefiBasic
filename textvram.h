#ifndef TEXTVRAM_H
#define TEXTVRAM_H

#include <efi.h>


#define TEXT_COLS 80
#define TEXT_ROWS 25

#define TEXT_FONT_W 8
#define TEXT_FONT_H 16

#define TEXT_WIDTH  (TEXT_COLS * TEXT_FONT_W)
#define TEXT_HEIGHT (TEXT_ROWS * TEXT_FONT_H)


typedef struct
{
    UINT8 character;

    UINT32 foreground;
    UINT32 background;

} TEXT_CELL;


/* ==========================================================
 * Initialize
 * ========================================================== */

void textvram_init(void);


/* ==========================================================
 * Clear
 * ========================================================== */

void textvram_clear(
    UINT32 foreground,
    UINT32 background);


/* ==========================================================
 * Direct cell access
 * ========================================================== */

void textvram_put_char(
    UINTN x,
    UINTN y,
    UINT8 character,
    UINT32 foreground,
    UINT32 background);


const TEXT_CELL *textvram_get_cell(
    UINTN x,
    UINTN y);


/* ==========================================================
 * Cursor
 * ========================================================== */

void textvram_set_cursor(
    UINTN x,
    UINTN y);

void textvram_get_cursor(
    UINTN *x,
    UINTN *y);

void textvram_show_cursor(
    BOOLEAN visible);


/* ==========================================================
 * Terminal output
 * ========================================================== */

void textvram_putc(
    UINT8 character);

void textvram_print(
    const char *text);


/* ==========================================================
 * Direct string output
 *
 * Cursor positionは変更しない。
 * ========================================================== */

void textvram_put_string(
    UINTN x,
    UINTN y,
    const char *text,
    UINT32 foreground,
    UINT32 background);


/* ==========================================================
 * Colors used by textvram_putc / textvram_print
 * ========================================================== */

void textvram_set_color(
    UINT32 foreground,
    UINT32 background);


/* ==========================================================
 * Render
 * ========================================================== */

void textvram_render(
    INTN origin_x,
    INTN origin_y);


#endif

