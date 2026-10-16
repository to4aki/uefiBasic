#include "hardware.h"


static HARDWARE_INFO info;


/* ==========================================================
 * CPUID
 * ========================================================== */

static void cpuid(
    UINT32 leaf,
    UINT32 subleaf,
    UINT32 *eax,
    UINT32 *ebx,
    UINT32 *ecx,
    UINT32 *edx)
{
    UINT32 a;
    UINT32 b;
    UINT32 c;
    UINT32 d;


    __asm__ __volatile__(
        "cpuid"
        : "=a"(a),
          "=b"(b),
          "=c"(c),
          "=d"(d)
        : "a"(leaf),
          "c"(subleaf)
    );


    if (eax != NULL)
        *eax = a;

    if (ebx != NULL)
        *ebx = b;

    if (ecx != NULL)
        *ecx = c;

    if (edx != NULL)
        *edx = d;
}


/* ==========================================================
 * Detect CPU vendor
 * ========================================================== */

static void detect_cpu_vendor(void)
{
    UINT32 eax;
    UINT32 ebx;
    UINT32 ecx;
    UINT32 edx;

    UINT32 *vendor;


    cpuid(
        0,
        0,
        &eax,
        &ebx,
        &ecx,
        &edx
    );


    vendor =
        (UINT32 *)info.cpu_vendor;


    vendor[0] = ebx;
    vendor[1] = edx;
    vendor[2] = ecx;


    info.cpu_vendor[12] =
        '\0';
}


/* ==========================================================
 * Detect CPU family/model/stepping
 * ========================================================== */

static void detect_cpu_model(void)
{
    UINT32 eax;
    UINT32 ebx;
    UINT32 ecx;
    UINT32 edx;

    UINT32 base_family;
    UINT32 base_model;

    UINT32 ext_family;
    UINT32 ext_model;


    cpuid(
        1,
        0,
        &eax,
        &ebx,
        &ecx,
        &edx
    );


    info.cpu_stepping =
        eax & 0x0F;


    base_model =
        (eax >> 4) & 0x0F;


    base_family =
        (eax >> 8) & 0x0F;


    ext_model =
        (eax >> 16) & 0x0F;


    ext_family =
        (eax >> 20) & 0xFF;


    if (base_family == 0x0F)
    {
        info.cpu_family =
            base_family +
            ext_family;
    }
    else
    {
        info.cpu_family =
            base_family;
    }


    if (base_family == 0x06 ||
        base_family == 0x0F)
    {
        info.cpu_model =
            base_model +
            (ext_model << 4);
    }
    else
    {
        info.cpu_model =
            base_model;
    }
}


/* ==========================================================
 * Detect CPU brand
 * ========================================================== */

static void detect_cpu_brand(void)
{
    UINT32 eax;
    UINT32 ebx;
    UINT32 ecx;
    UINT32 edx;

    UINT32 max_extended;

    UINT32 leaf;

    UINT32 *brand;


    cpuid(
        0x80000000U,
        0,
        &eax,
        &ebx,
        &ecx,
        &edx
    );


    max_extended =
        eax;


    info.cpu_brand[0] =
        '\0';


    if (max_extended < 0x80000004U)
        return;


    brand =
        (UINT32 *)info.cpu_brand;


    for (leaf = 0;
         leaf < 3;
         leaf++)
    {
        cpuid(
            0x80000002U + leaf,
            0,
            &eax,
            &ebx,
            &ecx,
            &edx
        );


        brand[(leaf * 4) + 0] =
            eax;

        brand[(leaf * 4) + 1] =
            ebx;

        brand[(leaf * 4) + 2] =
            ecx;

        brand[(leaf * 4) + 3] =
            edx;
    }


    info.cpu_brand[48] =
        '\0';
}


/* ==========================================================
 * Detect memory
 * ========================================================== */

static EFI_STATUS detect_memory(void)
{
    EFI_STATUS status;

    EFI_MEMORY_DESCRIPTOR *memory_map;
    EFI_MEMORY_DESCRIPTOR *descriptor;

    UINTN memory_map_size;
    UINTN map_key;
    UINTN descriptor_size;

    UINT32 descriptor_version;

    UINTN offset;

    UINTN pages_total;
    UINTN pages_available;


    memory_map =
        NULL;

    memory_map_size =
        0;


    status =
        uefi_call_wrapper(
            BS->GetMemoryMap,
            5,
            &memory_map_size,
            NULL,
            &map_key,
            &descriptor_size,
            &descriptor_version
        );


    if (status != EFI_BUFFER_TOO_SMALL)
        return status;


    /*
     * Firmware may add descriptors while allocating
     * the memory map itself. Reserve some extra space.
     */

    memory_map_size +=
        descriptor_size * 8;


    memory_map =
        AllocatePool(
            memory_map_size
        );


    if (memory_map == NULL)
        return EFI_OUT_OF_RESOURCES;


    status =
        uefi_call_wrapper(
            BS->GetMemoryMap,
            5,
            &memory_map_size,
            memory_map,
            &map_key,
            &descriptor_size,
            &descriptor_version
        );


    if (EFI_ERROR(status))
    {
        FreePool(
            memory_map
        );

        return status;
    }


    pages_total =
        0;

    pages_available =
        0;


    for (offset = 0;
         offset < memory_map_size;
         offset += descriptor_size)
    {
        descriptor =
            (EFI_MEMORY_DESCRIPTOR *)
            ((UINT8 *)memory_map + offset);


        /*
         * Count RAM-like UEFI memory.
         *
         * MMIO and reserved address ranges are not
         * included in physical RAM total.
         */

        switch (descriptor->Type)
        {
            case EfiLoaderCode:
            case EfiLoaderData:

            case EfiBootServicesCode:
            case EfiBootServicesData:

            case EfiRuntimeServicesCode:
            case EfiRuntimeServicesData:

            case EfiConventionalMemory:

            case EfiACPIReclaimMemory:
            case EfiACPIMemoryNVS:

                pages_total +=
                    descriptor->NumberOfPages;

                break;

            default:
                break;
        }


        /*
         * Currently free memory available to
         * Boot Services allocations.
         */

        if (descriptor->Type ==
            EfiConventionalMemory)
        {
            pages_available +=
                descriptor->NumberOfPages;
        }
    }


    info.memory_total_bytes =
        (UINT64)pages_total *
        4096ULL;


    info.memory_available_bytes =
        (UINT64)pages_available *
        4096ULL;


    FreePool(
        memory_map
    );


    return EFI_SUCCESS;
}


/* ==========================================================
 * Detect hardware
 * ========================================================== */

EFI_STATUS hardware_detect(void)
{
    EFI_STATUS status;


    SetMem(
        &info,
        sizeof(info),
        0
    );


    detect_cpu_vendor();

    detect_cpu_model();

    detect_cpu_brand();


    status =
        detect_memory();


    return status;
}


/* ==========================================================
 * Get information
 * ========================================================== */

const HARDWARE_INFO *hardware_info(void)
{
    return &info;
}

