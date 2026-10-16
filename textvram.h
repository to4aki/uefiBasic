#ifndef TEXTVRAM_H
#define TEXTVRAM_H

#include <efi.h>


#define TEXT_COLS 80
#define TEXT_ROWS 25

#define TEXT_FONT_W 8
#define TEXT_FONT_H 16


typedef struct
{
    UINT8 character;

    UINT32 foreground;
    UINT32 background;

} TEXT_CELL;


/* 初期化 */
void textvram_init(void);


/* 全画面クリア */
void textvram_clear(
    UINT32 foreground,
    UINT32 background);


/* 1文字書き込み */
void textvram_put_char(
    UINTN x,
    UINTN y,
    UINT8 character,
    UINT32 foreground,
    UINT32 background);


/* 文字列書き込み */
void textvram_put_string(
    UINTN x,
    UINTN y,
    const char *text,
    UINT32 foreground,
    UINT32 background);


/* 1セル取得 */
const TEXT_CELL *textvram_get_cell(
    UINTN x,
    UINTN y);


/* Graphics Backbufferへ描画 */
void textvram_render(void);


#endif

