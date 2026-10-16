#include "font8x16.h"
#include "font8x8_basic.h"


static UINT8 glyph16[FONT8X16_HEIGHT];


const UINT8 *font8x16_get(
    UINT8 code)
{
    UINTN y;
    UINT8 row;


    /*
     * font8x8_basic contains U+0000 - U+007F.
     */

    if (code >= 0x80)
    {
        code = ' ';
    }


    /*
     * Convert 8x8 to 8x16.
     *
     * Each 8 pixel row is repeated twice.
     *
     * font8x8_basic uses LSB for the left pixel.
     * textvram renderer currently expects MSB for
     * the left pixel, so reverse the bit order.
     */

    for (y = 0; y < 8; y++)
    {
        UINT8 value;
        UINT8 reversed;
        UINTN bit;


        value =
            (UINT8)font8x8_basic[code][y];

        reversed = 0;


        for (bit = 0; bit < 8; bit++)
        {
            if (value & (1U << bit))
            {
                reversed |=
                    (UINT8)(
                        1U << (7 - bit)
                    );
            }
        }


        row =
            reversed;


        glyph16[y * 2] =
            row;

        glyph16[y * 2 + 1] =
            row;
    }


    return glyph16;
}

