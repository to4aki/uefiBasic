#include <efi.h>
#include <efilib.h>

#include "z80emu.h"
#include "textvram.h"
#include "fileio.h"

#include "z80ex.h"


#define Z80_MEMORY_SIZE 65536

/*
 * T-state debt carried over from the previous run.
 *
 * z80ex_step() always executes a complete instruction,
 * so execution may slightly exceed the requested number
 * of T-states.
 */

static int cycle_debt =
    0;


/*
 * ==========================================================
 * Z80 emulator context
 * ==========================================================
 *
 * Android version:
 *
 *     Z80Context
 *       +-- memory
 *       +-- io
 *
 * Current UEFI version:
 *
 *     Z80EMU_CONTEXT
 *       +-- memory
 *
 * I/O bus will be added later.
 */

typedef struct
{
    UINT8 *memory;

} Z80EMU_CONTEXT;


/*
 * ==========================================================
 * Global state
 * ==========================================================
 */

static Z80EX_CONTEXT *cpu =
    NULL;


static UINT8 memory[
    Z80_MEMORY_SIZE
];


static Z80EMU_CONTEXT context;


/*
 * ==========================================================
 * Memory read callback
 * ==========================================================
 */

static Z80EX_BYTE cpu_read(
    Z80EX_CONTEXT *cpu_context,
    Z80EX_WORD addr,
    int m1_state,
    void *user_data)
{
    Z80EMU_CONTEXT *ctx;


    (void)cpu_context;
    (void)m1_state;


    ctx =
        (Z80EMU_CONTEXT *)user_data;


    return
        ctx->memory[addr];
}


/*
 * ==========================================================
 * Memory write callback
 * ==========================================================
 */

static void cpu_write(
    Z80EX_CONTEXT *cpu_context,
    Z80EX_WORD addr,
    Z80EX_BYTE value,
    void *user_data)
{
    Z80EMU_CONTEXT *ctx;


    (void)cpu_context;


    ctx =
        (Z80EMU_CONTEXT *)user_data;


    ctx->memory[addr] =
        value;
}


/*
 * ==========================================================
 * Port input callback
 * ==========================================================
 */

static Z80EX_BYTE cpu_in(
    Z80EX_CONTEXT *cpu_context,
    Z80EX_WORD port,
    void *user_data)
{
    Z80EMU_CONTEXT *ctx;


    (void)cpu_context;
    (void)port;


    ctx =
        (Z80EMU_CONTEXT *)user_data;


    (void)ctx;


    /*
     * No input device yet.
     */

    return 0xFF;
}


/*
 * ==========================================================
 * Port output callback
 * ==========================================================
 */

static void cpu_out(
    Z80EX_CONTEXT *cpu_context,
    Z80EX_WORD port,
    Z80EX_BYTE value,
    void *user_data)
{
    Z80EMU_CONTEXT *ctx;

    UINT8 port_low;


    (void)cpu_context;


    ctx =
        (Z80EMU_CONTEXT *)user_data;


    (void)ctx;


    /*
     * For now, only decode the lower 8 bits
     * of the Z80 I/O address.
     */

    port_low =
        (UINT8)(
            port &
            0x00FF
        );


    /*
     * Port 00h
     *
     * Console output.
     */

    if (port_low == 0x00)
    {
        textvram_putc(
            (char)value
        );
    }
}


/*
 * ==========================================================
 * Interrupt vector read callback
 * ==========================================================
 */

static Z80EX_BYTE cpu_intread(
    Z80EX_CONTEXT *cpu_context,
    void *user_data)
{
    Z80EMU_CONTEXT *ctx;


    (void)cpu_context;


    ctx =
        (Z80EMU_CONTEXT *)user_data;


    (void)ctx;


    return 0x00;
}


/*
 * ==========================================================
 * Initialize
 * ==========================================================
 */

EFI_STATUS z80emu_init(void)
{
    if (cpu != NULL)
    {
        return EFI_SUCCESS;
    }


    /*
     * Clear complete Z80 memory.
     */

    SetMem(
        memory,
        sizeof(memory),
        0
    );


    /*
     * Initialize emulator context.
     */

    context.memory =
        memory;


    /*
     * Clear cycle correction.
     */

    cycle_debt =
        0;


    /*
     * Create Z80Ex CPU.
     */

    cpu =
        z80ex_create(
            cpu_read,
            &context,

            cpu_write,
            &context,

            cpu_in,
            &context,

            cpu_out,
            &context,

            cpu_intread,
            &context
        );


    if (cpu == NULL)
    {
        return EFI_OUT_OF_RESOURCES;
    }


    /*
     * Reset CPU.
     */

    z80ex_reset(
        cpu
    );


    return EFI_SUCCESS;
}


/*
 * ==========================================================
 * Shutdown
 * ==========================================================
 */

void z80emu_shutdown(void)
{
    if (cpu == NULL)
    {
        return;
    }


    z80ex_destroy(
        cpu
    );


    cpu =
        NULL;
}


/*
 * ==========================================================
 * Reset
 * ==========================================================
 */

void z80emu_reset(void)
{
    if (cpu == NULL)
    {
        return;
    }


    cycle_debt =
        0;


    z80ex_reset(
        cpu
    );
}


/*
 * ==========================================================
 * Run specified number of Z80 T-states
 * ==========================================================
 */

void z80emu_run_cycles(
    int cycles)
{
    int target;
    int executed;


    if (cpu == NULL)
    {
        return;
    }


    if (cycles <= 0)
    {
        return;
    }


    /*
     * Compensate for cycles executed beyond the requested
     * amount during the previous call.
     */

    target =
        cycles -
        cycle_debt;


    if (target <= 0)
    {
        cycle_debt =
            -target;

        return;
    }


    executed =
        0;


    while (executed < target)
    {
        executed +=
            z80ex_step(
                cpu
            );
    }


    /*
     * z80ex_step() executes one complete instruction.
     *
     * Therefore executed can exceed target.
     * Carry that excess into the next frame.
     */

    cycle_debt =
        executed -
        target;
}


/*
 * ==========================================================
 * Interrupt
 * ==========================================================
 */

void z80emu_interrupt(void)
{
    if (cpu == NULL)
    {
        return;
    }


    z80ex_int(
        cpu
    );
}


/*
 * ==========================================================
 * Get Z80 memory
 * ==========================================================
 */

UINT8 *z80emu_memory(void)
{
    return memory;
}


/*
 * ==========================================================
 * Minimal Z80 execution test
 * ==========================================================
 *
 * 0000  3E 41      LD A,41h
 * 0002  D3 00      OUT (00h),A
 * 0004  76         HALT
 *
 * Result:
 *
 *     Z80 TEST: A
 *
 * ==========================================================
 */

void z80emu_test(void)
{
    static const UINT8 program[] =
    {
        0x3E, 0x41,
        0xD3, 0x00,
        0x76
    };

    UINTN i;


    if (cpu == NULL)
    {
        textvram_print(
            "Z80 NOT INITIALIZED\n"
        );

        return;
    }


    /*
     * Clear Z80 memory.
     */

    SetMem(
        memory,
        sizeof(memory),
        0
    );


    /*
     * Load test program at 0000h.
     */

    for (i = 0;
         i < sizeof(program);
         i++)
    {
        memory[i] =
            program[i];
    }


    /*
     * Reset Z80.
     *
     * PC starts at 0000h.
     */

    z80ex_reset(
        cpu
    );


    textvram_print(
        "Z80 TEST: "
    );


    /*
     * LD A,'A'
     */

    z80ex_step(
        cpu
    );


    /*
     * OUT (00h),A
     *
     * cpu_out() should receive value 41h
     * and output 'A' to textvram.
     */

    z80ex_step(
        cpu
    );


    /*
     * HALT
     */

    z80ex_step(
        cpu
    );


    textvram_putc(
        '\n'
    );
}

EFI_STATUS z80emu_load(
    const CHAR16 *path,
    UINT16 address)
{
    VOID *data;

    UINTN size;

    UINTN available;


    if (path == NULL)
    {
        return EFI_INVALID_PARAMETER;
    }


    data =
        NULL;

    size =
        0;


    /*
     * Load complete binary using High Level fileio.
     */

    data =
        fileio_read_binary(
            path,
            &size
        );


    if (data == NULL)
    {
        return EFI_NOT_FOUND;
    }


    /*
     * Number of bytes available from the requested
     * address to the end of the Z80 address space.
     */

    available =
        Z80_MEMORY_SIZE -
        (UINTN)address;


    /*
     * Do not allow the binary to overflow 64KB RAM.
     */

    if (size > available)
    {
        fileio_free(
            data
        );

        return EFI_BAD_BUFFER_SIZE;
    }


    /*
     * Copy binary into Z80 memory.
     */

    CopyMem(
        &memory[address],
        data,
        size
    );


    fileio_free(
        data
    );


    /*
     * Reset timing correction.
     *
     * CPU reset itself is deliberately NOT done here.
     * Loading memory and resetting CPU remain separate
     * operations.
     */

    cycle_debt =
        0;


    return EFI_SUCCESS;
}

static int hex_digit(
    char ch)
{
    if (ch >= '0' &&
        ch <= '9')
    {
        return
            ch - '0';
    }


    if (ch >= 'A' &&
        ch <= 'F')
    {
        return
            ch - 'A' + 10;
    }


    if (ch >= 'a' &&
        ch <= 'f')
    {
        return
            ch - 'a' + 10;
    }


    return -1;
}

static BOOLEAN hex_byte(
    const char *text,
    UINT8 *value)
{
    int high;
    int low;


    high =
        hex_digit(
            text[0]
        );


    low =
        hex_digit(
            text[1]
        );


    if (high < 0 ||
        low < 0)
    {
        return FALSE;
    }


    *value =
        (UINT8)(
            (high << 4) |
            low
        );


    return TRUE;
}

EFI_STATUS z80emu_load_hex(
    const CHAR16 *path)
{
    char *text;

    UINTN file_size;
    UINTN pos;

    UINT32 base_address;

    BOOLEAN eof_found;


    if (path == NULL)
    {
        return EFI_INVALID_PARAMETER;
    }


    text =
        fileio_read_text(
            path,
            &file_size
        );


    if (text == NULL)
    {
        return EFI_NOT_FOUND;
    }


    /*
     * Clear complete Z80 address space before loading ROM.
     */

    SetMem(
        memory,
        sizeof(memory),
        0
    );


    pos =
        0;


    base_address =
        0;


    eof_found =
        FALSE;


    while (pos < file_size)
    {
        UINT8 length;

        UINT8 address_high;
        UINT8 address_low;

        UINT8 record_type;

        UINT8 checksum;

        UINT8 calculated_checksum;

        UINT16 record_address;

        UINT32 absolute_address;

        UINTN data_pos;
        UINTN i;


        /*
         * Skip CR/LF and empty lines.
         */

        while (pos < file_size &&
               (text[pos] == '\r' ||
                text[pos] == '\n'))
        {
            pos++;
        }


        if (pos >= file_size)
        {
            break;
        }


        /*
         * Every Intel HEX record starts with ':'.
         */

        if (text[pos] != ':')
        {
            fileio_free(
                text
            );

            return EFI_LOAD_ERROR;
        }


        pos++;


        /*
         * Minimum fields:
         *
         * LL AAAA TT CC
         *
         * represented by 10 hexadecimal characters.
         */

        if ((file_size - pos) < 10)
        {
            fileio_free(
                text
            );

            return EFI_LOAD_ERROR;
        }


        /*
         * Record length.
         */

        if (!hex_byte(
                &text[pos],
                &length))
        {
            fileio_free(
                text
            );

            return EFI_LOAD_ERROR;
        }


        pos +=
            2;


        /*
         * Address high byte.
         */

        if (!hex_byte(
                &text[pos],
                &address_high))
        {
            fileio_free(
                text
            );

            return EFI_LOAD_ERROR;
        }


        pos +=
            2;


        /*
         * Address low byte.
         */

        if (!hex_byte(
                &text[pos],
                &address_low))
        {
            fileio_free(
                text
            );

            return EFI_LOAD_ERROR;
        }


        pos +=
            2;


        record_address =
            ((UINT16)address_high << 8) |
            address_low;


        /*
         * Record type.
         */

        if (!hex_byte(
                &text[pos],
                &record_type))
        {
            fileio_free(
                text
            );

            return EFI_LOAD_ERROR;
        }


        pos +=
            2;


        /*
         * Verify that data + checksum remain in file.
         */

        if ((file_size - pos) <
            ((UINTN)length * 2 + 2))
        {
            fileio_free(
                text
            );

            return EFI_LOAD_ERROR;
        }


        calculated_checksum =
            length;


        calculated_checksum +=
            address_high;


        calculated_checksum +=
            address_low;


        calculated_checksum +=
            record_type;


        data_pos =
            pos;


        /*
         * Include all data bytes in checksum.
         */

        for (i = 0;
             i < length;
             i++)
        {
            UINT8 value;


            if (!hex_byte(
                    &text[data_pos + (i * 2)],
                    &value))
            {
                fileio_free(
                    text
                );

                return EFI_LOAD_ERROR;
            }


            calculated_checksum +=
                value;
        }


        /*
         * Read stored checksum.
         */

        if (!hex_byte(
                &text[
                    data_pos +
                    ((UINTN)length * 2)
                ],
                &checksum))
        {
            fileio_free(
                text
            );

            return EFI_LOAD_ERROR;
        }


        calculated_checksum +=
            checksum;


        /*
         * Intel HEX checksum is valid when
         * the low 8 bits of the complete sum are zero.
         */

        if (calculated_checksum != 0)
        {
            fileio_free(
                text
            );

            return EFI_CRC_ERROR;
        }


        /*
         * Process record.
         */

        switch (record_type)
        {
            /*
             * Data record
             */

            case 0x00:
            {
                absolute_address =
                    base_address +
                    record_address;


                /*
                 * Z80 address space is only 64KB.
                 */

                if (absolute_address >=
                    Z80_MEMORY_SIZE)
                {
                    fileio_free(
                        text
                    );

                    return EFI_BAD_BUFFER_SIZE;
                }


                if ((absolute_address +
                     length) >
                    Z80_MEMORY_SIZE)
                {
                    fileio_free(
                        text
                    );

                    return EFI_BAD_BUFFER_SIZE;
                }


                /*
                 * Decode directly into Z80 RAM.
                 */

                for (i = 0;
                     i < length;
                     i++)
                {
                    UINT8 value;


                    if (!hex_byte(
                            &text[
                                data_pos +
                                (i * 2)
                            ],
                            &value))
                    {
                        fileio_free(
                            text
                        );

                        return EFI_LOAD_ERROR;
                    }


                    memory[
                        absolute_address + i
                    ] =
                        value;
                }


                break;
            }


            /*
             * End Of File
             */

            case 0x01:
            {
                eof_found =
                    TRUE;

                break;
            }


            /*
             * Extended Segment Address
             */

            case 0x02:
            {
                UINT8 high;
                UINT8 low;

                UINT16 segment;


                if (length != 2)
                {
                    fileio_free(
                        text
                    );

                    return EFI_LOAD_ERROR;
                }


                if (!hex_byte(
                        &text[data_pos],
                        &high) ||
                    !hex_byte(
                        &text[data_pos + 2],
                        &low))
                {
                    fileio_free(
                        text
                    );

                    return EFI_LOAD_ERROR;
                }


                segment =
                    ((UINT16)high << 8) |
                    low;


                base_address =
                    ((UINT32)segment) << 4;


                break;
            }


            /*
             * Extended Linear Address
             */

            case 0x04:
            {
                UINT8 high;
                UINT8 low;

                UINT16 upper;


                if (length != 2)
                {
                    fileio_free(
                        text
                    );

                    return EFI_LOAD_ERROR;
                }


                if (!hex_byte(
                        &text[data_pos],
                        &high) ||
                    !hex_byte(
                        &text[data_pos + 2],
                        &low))
                {
                    fileio_free(
                        text
                    );

                    return EFI_LOAD_ERROR;
                }


                upper =
                    ((UINT16)high << 8) |
                    low;


                base_address =
                    ((UINT32)upper) << 16;


                break;
            }


            /*
             * Start address records are not needed
             * for the current Z80 environment.
             */

            case 0x03:
            case 0x05:
            {
                break;
            }


            default:
            {
                fileio_free(
                    text
                );

                return EFI_UNSUPPORTED;
            }
        }


        /*
         * Move beyond:
         *
         * data
         * checksum
         */

        pos =
            data_pos +
            ((UINTN)length * 2) +
            2;


        /*
         * EOF record terminates parsing.
         */

        if (eof_found)
        {
            break;
        }


        /*
         * Next loop skips CR/LF.
         */
    }


    fileio_free(
        text
    );


    if (!eof_found)
    {
        return EFI_LOAD_ERROR;
    }


    cycle_debt =
        0;


    return EFI_SUCCESS;
}


