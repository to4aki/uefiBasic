#ifndef FILEIO_H
#define FILEIO_H

#include <efi.h>
#include <efilib.h>
#include <efiprot.h>


#define FILEIO_MODE_READ    0x01
#define FILEIO_MODE_WRITE   0x02
#define FILEIO_MODE_CREATE  0x04


typedef struct FILEIO_HANDLE FILEIO_HANDLE;


/* ==========================================================
 * File system
 * ========================================================== */

EFI_STATUS fileio_init(
    EFI_HANDLE image_handle
);


void fileio_shutdown(void);


/* ==========================================================
 * Stream API
 * ========================================================== */

EFI_STATUS fileio_open(
    const CHAR16 *path,
    UINTN mode,
    FILEIO_HANDLE **handle
);


EFI_STATUS fileio_close(
    FILEIO_HANDLE *handle
);


EFI_STATUS fileio_read(
    FILEIO_HANDLE *handle,
    VOID *buffer,
    UINTN size,
    UINTN *read_size
);


EFI_STATUS fileio_write(
    FILEIO_HANDLE *handle,
    const VOID *buffer,
    UINTN size,
    UINTN *written_size
);


EFI_STATUS fileio_seek(
    FILEIO_HANDLE *handle,
    UINT64 position
);


EFI_STATUS fileio_tell(
    FILEIO_HANDLE *handle,
    UINT64 *position
);


EFI_STATUS fileio_size(
    FILEIO_HANDLE *handle,
    UINT64 *size
);


EFI_STATUS fileio_flush(
    FILEIO_HANDLE *handle
);


/* ==========================================================
 * Utility API
 * ========================================================== */

BOOLEAN fileio_exists(
    const CHAR16 *path
);


EFI_STATUS fileio_load(
    const CHAR16 *path,
    VOID **buffer,
    UINTN *size
);


EFI_STATUS fileio_save(
    const CHAR16 *path,
    const VOID *buffer,
    UINTN size
);

/* ==========================================================
 * High level API
 * ========================================================== */

char *fileio_read_text(
    const CHAR16 *path,
    UINTN *size
);


EFI_STATUS fileio_write_text(
    const CHAR16 *path,
    const char *text
);


EFI_STATUS fileio_append_text(
    const CHAR16 *path,
    const char *text
);


VOID *fileio_read_binary(
    const CHAR16 *path,
    UINTN *size
);


EFI_STATUS fileio_write_binary(
    const CHAR16 *path,
    const VOID *data,
    UINTN size
);


void fileio_free(
    VOID *buffer
);

#endif

