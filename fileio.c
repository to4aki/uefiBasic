#include "fileio.h"


#define FILEIO_READ_BLOCK_SIZE 4096


struct FILEIO_HANDLE
{
    EFI_FILE_PROTOCOL *file;
    UINTN mode;
};


static EFI_FILE_PROTOCOL *root = NULL;


/* ==========================================================
 * Initialize
 * ========================================================== */

EFI_STATUS fileio_init(
    EFI_HANDLE image_handle)
{
    EFI_STATUS status;

    EFI_GUID loaded_image_guid =
        EFI_LOADED_IMAGE_PROTOCOL_GUID;

    EFI_GUID filesystem_guid =
        EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;

    EFI_LOADED_IMAGE *loaded_image;

    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *filesystem;


    loaded_image = NULL;
    filesystem = NULL;
    root = NULL;


    status =
        uefi_call_wrapper(
            BS->HandleProtocol,
            3,
            image_handle,
            &loaded_image_guid,
            (VOID **)&loaded_image
        );


    if (EFI_ERROR(status))
        return status;


    status =
        uefi_call_wrapper(
            BS->HandleProtocol,
            3,
            loaded_image->DeviceHandle,
            &filesystem_guid,
            (VOID **)&filesystem
        );


    if (EFI_ERROR(status))
        return status;


    status =
        uefi_call_wrapper(
            filesystem->OpenVolume,
            2,
            filesystem,
            &root
        );


    if (EFI_ERROR(status))
    {
        root = NULL;

        return status;
    }


    return EFI_SUCCESS;
}


/* ==========================================================
 * Shutdown
 * ========================================================== */

void fileio_shutdown(void)
{
    if (root != NULL)
    {
        uefi_call_wrapper(
            root->Close,
            1,
            root
        );

        root = NULL;
    }
}


/* ==========================================================
 * Open
 * ========================================================== */

EFI_STATUS fileio_open(
    const CHAR16 *path,
    UINTN mode,
    FILEIO_HANDLE **handle)
{
    EFI_STATUS status;

    FILEIO_HANDLE *new_handle;

    EFI_FILE_PROTOCOL *file;

    UINT64 efi_mode;


    if (path == NULL)
        return EFI_INVALID_PARAMETER;

    if (handle == NULL)
        return EFI_INVALID_PARAMETER;

    if (root == NULL)
        return EFI_NOT_READY;


    *handle = NULL;


    if ((mode &
         (FILEIO_MODE_READ |
          FILEIO_MODE_WRITE)) == 0)
    {
        return EFI_INVALID_PARAMETER;
    }


    /*
     * UEFI requires READ when CREATE is used.
     */

    efi_mode = 0;


    if (mode & FILEIO_MODE_READ)
    {
        efi_mode |=
            EFI_FILE_MODE_READ;
    }


    if (mode & FILEIO_MODE_WRITE)
    {
        efi_mode |=
            EFI_FILE_MODE_WRITE;
    }


    if (mode & FILEIO_MODE_CREATE)
    {
        efi_mode |=
            EFI_FILE_MODE_READ |
            EFI_FILE_MODE_WRITE |
            EFI_FILE_MODE_CREATE;
    }


    file = NULL;


    status =
        uefi_call_wrapper(
            root->Open,
            5,
            root,
            &file,
            (CHAR16 *)path,
            efi_mode,
            0
        );


    if (EFI_ERROR(status))
        return status;


    new_handle =
        AllocatePool(
            sizeof(FILEIO_HANDLE)
        );


    if (new_handle == NULL)
    {
        uefi_call_wrapper(
            file->Close,
            1,
            file
        );

        return EFI_OUT_OF_RESOURCES;
    }


    new_handle->file =
        file;

    new_handle->mode =
        mode;


    *handle =
        new_handle;


    return EFI_SUCCESS;
}


/* ==========================================================
 * Close
 * ========================================================== */

EFI_STATUS fileio_close(
    FILEIO_HANDLE *handle)
{
    EFI_STATUS status;


    if (handle == NULL)
        return EFI_INVALID_PARAMETER;


    if (handle->file == NULL)
    {
        FreePool(
            handle
        );

        return EFI_INVALID_PARAMETER;
    }


    status =
        uefi_call_wrapper(
            handle->file->Close,
            1,
            handle->file
        );


    handle->file =
        NULL;


    FreePool(
        handle
    );


    return status;
}


/* ==========================================================
 * Read
 * ========================================================== */

EFI_STATUS fileio_read(
    FILEIO_HANDLE *handle,
    VOID *buffer,
    UINTN size,
    UINTN *read_size)
{
    EFI_STATUS status;

    UINTN actual;


    if (read_size != NULL)
        *read_size = 0;


    if (handle == NULL)
        return EFI_INVALID_PARAMETER;

    if (handle->file == NULL)
        return EFI_INVALID_PARAMETER;

    if (buffer == NULL &&
        size != 0)
    {
        return EFI_INVALID_PARAMETER;
    }


    if ((handle->mode &
         FILEIO_MODE_READ) == 0)
    {
        return EFI_ACCESS_DENIED;
    }


    actual =
        size;


    status =
        uefi_call_wrapper(
            handle->file->Read,
            3,
            handle->file,
            &actual,
            buffer
        );


    if (read_size != NULL)
    {
        *read_size =
            actual;
    }


    return status;
}


/* ==========================================================
 * Write
 * ========================================================== */

EFI_STATUS fileio_write(
    FILEIO_HANDLE *handle,
    const VOID *buffer,
    UINTN size,
    UINTN *written_size)
{
    EFI_STATUS status;

    UINTN actual;


    if (written_size != NULL)
        *written_size = 0;


    if (handle == NULL)
        return EFI_INVALID_PARAMETER;

    if (handle->file == NULL)
        return EFI_INVALID_PARAMETER;

    if (buffer == NULL &&
        size != 0)
    {
        return EFI_INVALID_PARAMETER;
    }


    if ((handle->mode &
         FILEIO_MODE_WRITE) == 0)
    {
        return EFI_ACCESS_DENIED;
    }


    actual =
        size;


    status =
        uefi_call_wrapper(
            handle->file->Write,
            3,
            handle->file,
            &actual,
            (VOID *)buffer
        );


    if (written_size != NULL)
    {
        *written_size =
            actual;
    }


    return status;
}


/* ==========================================================
 * Seek
 * ========================================================== */

EFI_STATUS fileio_seek(
    FILEIO_HANDLE *handle,
    UINT64 position)
{
    if (handle == NULL)
        return EFI_INVALID_PARAMETER;

    if (handle->file == NULL)
        return EFI_INVALID_PARAMETER;


    return
        uefi_call_wrapper(
            handle->file->SetPosition,
            2,
            handle->file,
            position
        );
}


/* ==========================================================
 * Tell
 * ========================================================== */

EFI_STATUS fileio_tell(
    FILEIO_HANDLE *handle,
    UINT64 *position)
{
    if (handle == NULL)
        return EFI_INVALID_PARAMETER;

    if (handle->file == NULL)
        return EFI_INVALID_PARAMETER;

    if (position == NULL)
        return EFI_INVALID_PARAMETER;


    return
        uefi_call_wrapper(
            handle->file->GetPosition,
            2,
            handle->file,
            position
        );
}


/* ==========================================================
 * Size
 * ========================================================== */

EFI_STATUS fileio_size(
    FILEIO_HANDLE *handle,
    UINT64 *size)
{
    EFI_STATUS status;

    UINT64 original_position;
    UINT64 end_position;


    if (handle == NULL)
        return EFI_INVALID_PARAMETER;

    if (handle->file == NULL)
        return EFI_INVALID_PARAMETER;

    if (size == NULL)
        return EFI_INVALID_PARAMETER;


    *size = 0;


    status =
        fileio_tell(
            handle,
            &original_position
        );


    if (EFI_ERROR(status))
        return status;


    /*
     * UEFI defines all-bits-one as "seek to end"
     * for regular files.
     */

    status =
        fileio_seek(
            handle,
            0xFFFFFFFFFFFFFFFFULL
        );


    if (EFI_ERROR(status))
        return status;


    status =
        fileio_tell(
            handle,
            &end_position
        );


    /*
     * Restore the previous position even if
     * GetPosition failed.
     */

    fileio_seek(
        handle,
        original_position
    );


    if (EFI_ERROR(status))
        return status;


    *size =
        end_position;


    return EFI_SUCCESS;
}


/* ==========================================================
 * Flush
 * ========================================================== */

EFI_STATUS fileio_flush(
    FILEIO_HANDLE *handle)
{
    if (handle == NULL)
        return EFI_INVALID_PARAMETER;

    if (handle->file == NULL)
        return EFI_INVALID_PARAMETER;


    return
        uefi_call_wrapper(
            handle->file->Flush,
            1,
            handle->file
        );
}


/* ==========================================================
 * Exists
 * ========================================================== */

BOOLEAN fileio_exists(
    const CHAR16 *path)
{
    EFI_STATUS status;

    FILEIO_HANDLE *handle;


    handle =
        NULL;


    status =
        fileio_open(
            path,
            FILEIO_MODE_READ,
            &handle
        );


    if (EFI_ERROR(status))
        return FALSE;


    fileio_close(
        handle
    );


    return TRUE;
}


/* ==========================================================
 * Load complete file
 * ========================================================== */

EFI_STATUS fileio_load(
    const CHAR16 *path,
    VOID **buffer,
    UINTN *size)
{
    EFI_STATUS status;

    FILEIO_HANDLE *handle;

    UINT64 file_size;

    UINT8 *data;

    UINTN actual;


    if (buffer == NULL)
        return EFI_INVALID_PARAMETER;

    if (size == NULL)
        return EFI_INVALID_PARAMETER;


    *buffer =
        NULL;

    *size =
        0;


    handle =
        NULL;


    status =
        fileio_open(
            path,
            FILEIO_MODE_READ,
            &handle
        );


    if (EFI_ERROR(status))
        return status;


    status =
        fileio_size(
            handle,
            &file_size
        );


    if (EFI_ERROR(status))
    {
        fileio_close(
            handle
        );

        return status;
    }


    /*
     * UINTN is 64-bit in the current x86_64 build,
     * but explicitly reject impossible conversions.
     */

    if (file_size >
        (UINT64)((UINTN)-1) - 1)
    {
        fileio_close(
            handle
        );

        return EFI_OUT_OF_RESOURCES;
    }


    data =
        AllocatePool(
            (UINTN)file_size + 1
        );


    if (data == NULL)
    {
        fileio_close(
            handle
        );

        return EFI_OUT_OF_RESOURCES;
    }


    status =
        fileio_seek(
            handle,
            0
        );


    if (EFI_ERROR(status))
    {
        FreePool(
            data
        );

        fileio_close(
            handle
        );

        return status;
    }


    actual =
        0;


    status =
        fileio_read(
            handle,
            data,
            (UINTN)file_size,
            &actual
        );


    if (EFI_ERROR(status))
    {
        FreePool(
            data
        );

        fileio_close(
            handle
        );

        return status;
    }


    if (actual !=
        (UINTN)file_size)
    {
        FreePool(
            data
        );

        fileio_close(
            handle
        );

        return EFI_DEVICE_ERROR;
    }


    /*
     * Convenience terminator for ASCII files.
     * It is not included in returned size.
     */

    data[actual] =
        0;


    fileio_close(
        handle
    );


    *buffer =
        data;

    *size =
        actual;


    return EFI_SUCCESS;
}


/* ==========================================================
 * Save complete file
 * ========================================================== */

EFI_STATUS fileio_save(
    const CHAR16 *path,
    const VOID *buffer,
    UINTN size)
{
    EFI_STATUS status;

    FILEIO_HANDLE *handle;

    UINTN actual;


    if (path == NULL)
        return EFI_INVALID_PARAMETER;

    if (buffer == NULL &&
        size != 0)
    {
        return EFI_INVALID_PARAMETER;
    }


    handle =
        NULL;


    status =
        fileio_open(
            path,
            FILEIO_MODE_READ |
            FILEIO_MODE_WRITE |
            FILEIO_MODE_CREATE,
            &handle
        );


    if (EFI_ERROR(status))
        return status;


    status =
        fileio_seek(
            handle,
            0
        );


    if (EFI_ERROR(status))
    {
        fileio_close(
            handle
        );

        return status;
    }


    actual =
        0;


    status =
        fileio_write(
            handle,
            buffer,
            size,
            &actual
        );


    if (EFI_ERROR(status))
    {
        fileio_close(
            handle
        );

        return status;
    }


    if (actual != size)
    {
        fileio_close(
            handle
        );

        return EFI_DEVICE_ERROR;
    }


    status =
        fileio_flush(
            handle
        );


    fileio_close(
        handle
    );


    return status;
}

/* ==========================================================
 * String length
 * ========================================================== */

static UINTN fileio_strlen(
    const char *text)
{
    UINTN length;


    if (text == NULL)
        return 0;


    length = 0;


    while (text[length] != '\0')
    {
        length++;
    }


    return length;
}


/* ==========================================================
 * Read complete text file
 *
 * Returned buffer is always zero terminated.
 * Caller must release it with fileio_free().
 * ========================================================== */

char *fileio_read_text(
    const CHAR16 *path,
    UINTN *size)
{
    EFI_STATUS status;

    VOID *buffer;

    UINTN file_size;


    buffer = NULL;

    file_size = 0;


    status =
        fileio_load(
            path,
            &buffer,
            &file_size
        );


    if (EFI_ERROR(status))
    {
        if (size != NULL)
            *size = 0;

        return NULL;
    }


    if (size != NULL)
    {
        *size =
            file_size;
    }


    return
        (char *)buffer;
}


/* ==========================================================
 * Write complete text file
 * ========================================================== */

EFI_STATUS fileio_write_text(
    const CHAR16 *path,
    const char *text)
{
    UINTN length;


    if (path == NULL)
        return EFI_INVALID_PARAMETER;

    if (text == NULL)
        return EFI_INVALID_PARAMETER;


    length =
        fileio_strlen(
            text
        );


    return
        fileio_save(
            path,
            text,
            length
        );
}


/* ==========================================================
 * Append text
 * ========================================================== */

EFI_STATUS fileio_append_text(
    const CHAR16 *path,
    const char *text)
{
    EFI_STATUS status;

    FILEIO_HANDLE *file;

    UINTN length;
    UINTN written;


    if (path == NULL)
        return EFI_INVALID_PARAMETER;

    if (text == NULL)
        return EFI_INVALID_PARAMETER;


    file = NULL;


    /*
     * Try existing file first.
     */

    status =
        fileio_open(
            path,
            FILEIO_MODE_READ |
            FILEIO_MODE_WRITE,
            &file
        );


    /*
     * Create it when it does not exist.
     */

    if (EFI_ERROR(status))
    {
        status =
            fileio_open(
                path,
                FILEIO_MODE_READ |
                FILEIO_MODE_WRITE |
                FILEIO_MODE_CREATE,
                &file
            );


        if (EFI_ERROR(status))
            return status;
    }


    /*
     * UEFI EOF position.
     */

    status =
        fileio_seek(
            file,
            0xFFFFFFFFFFFFFFFFULL
        );


    if (EFI_ERROR(status))
    {
        fileio_close(
            file
        );

        return status;
    }


    length =
        fileio_strlen(
            text
        );


    written = 0;


    status =
        fileio_write(
            file,
            text,
            length,
            &written
        );


    if (!EFI_ERROR(status) &&
        written != length)
    {
        status =
            EFI_DEVICE_ERROR;
    }


    if (!EFI_ERROR(status))
    {
        status =
            fileio_flush(
                file
        );
    }


    fileio_close(
        file
    );


    return status;
}


/* ==========================================================
 * Read complete binary file
 *
 * Caller must release returned buffer with fileio_free().
 * ========================================================== */

VOID *fileio_read_binary(
    const CHAR16 *path,
    UINTN *size)
{
    EFI_STATUS status;

    VOID *buffer;

    UINTN file_size;


    if (size == NULL)
        return NULL;


    buffer = NULL;

    file_size = 0;


    status =
        fileio_load(
            path,
            &buffer,
            &file_size
        );


    if (EFI_ERROR(status))
    {
        *size = 0;

        return NULL;
    }


    *size =
        file_size;


    return buffer;
}


/* ==========================================================
 * Write complete binary file
 * ========================================================== */

EFI_STATUS fileio_write_binary(
    const CHAR16 *path,
    const VOID *data,
    UINTN size)
{
    return
        fileio_save(
            path,
            data,
            size
        );
}


/* ==========================================================
 * Free buffer returned by high level API
 * ========================================================== */

void fileio_free(
    VOID *buffer)
{
    if (buffer != NULL)
    {
        FreePool(
            buffer
        );
    }
}


