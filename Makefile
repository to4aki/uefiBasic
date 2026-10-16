TARGET = BOOTX64.EFI

CC = gcc

Z80EX_DIR = z80/z80ex

Z80EX_OBJS = \
        z80ex_core.o

OBJS = \
	main.o \
	graphics.o \
	input.o \
	textvram.o \
	font8x16.o \
	hardware.o \
	fileio.o \
	z80emu.o \
	$(Z80EX_OBJS) \
	system.o

Z80EX_DEFS = \
        -DWORDS_LITTLE_ENDIAN \
        -DZ80EX_API_REVISION=1 \
        -DZ80EX_VERSION_MAJOR=1 \
        -DZ80EX_VERSION_MINOR=21 \
        -DZ80EX_RELEASE_TYPE=\"\" \
        -DZ80EX_VERSION_STR=\"1.21\"


# ============================================================
# GNU-EFI
# ============================================================

EFI_INC = \
        -I/usr/include/efi \
        -I/usr/include/efi/x86_64 \
        -Iz80/z80ex/include

CFLAGS = \
	$(EFI_INC) \
	-fpic \
	-fshort-wchar \
	-ffreestanding \
	-fno-stack-protector \
	-mno-red-zone

LDFLAGS = \
	-nostdlib \
	-Wl,-T,/usr/lib/elf_x86_64_efi.lds \
	-Wl,-shared \
	-Wl,-Bsymbolic \
	-Wl,-znocombreloc


# ============================================================
# OVMF
# ============================================================

OVMF_BUILD = ../edk2/Build/OvmfX64/DEBUG_GCC/FV

OVMF_CODE = $(OVMF_BUILD)/OVMF_CODE.fd
OVMF_VARS = $(OVMF_BUILD)/OVMF_VARS.fd

DEBUG_VARS = OVMF_VARS_DEBUG.fd


# ============================================================
# Directories
# ============================================================

ESP_DIR = esp

DIST_DIR = dist
KVM_DIR = $(DIST_DIR)/kvm
USB_DIR = $(DIST_DIR)/usb


# ============================================================
# Disk images
# ============================================================

KVM_RAW_IMAGE = $(KVM_DIR)/system.raw
QCOW2_IMAGE = $(KVM_DIR)/system.qcow2

USB_IMAGE = $(USB_DIR)/system.img

IMAGE_SIZE_MB = 64

ESP_OFFSET_SECTORS = 2048
ESP_OFFSET_BYTES = 1048576


# ============================================================
# Build
# ============================================================

all: $(TARGET)


%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


z80ex_core.o: $(Z80EX_DIR)/z80ex.c
	$(CC) $(CFLAGS) $(Z80EX_DEFS) \
		-I$(Z80EX_DIR) \
		-I$(Z80EX_DIR)/include \
		-c $< \
		-o $@


boot.so: $(OBJS)
	$(CC) \
		$(LDFLAGS) \
		/usr/lib/crt0-efi-x86_64.o \
		$(OBJS) \
		-L/usr/lib \
		-lefi \
		-lgnuefi \
		-o $@

$(TARGET): boot.so
	objcopy \
		-j .text \
		-j .sdata \
		-j .data \
		-j .dynamic \
		-j .dynsym \
		-j .rel \
		-j .rela \
		-j .reloc \
		--target=efi-app-x86_64 \
		boot.so \
		$(TARGET)


# ============================================================
# Development ESP
# ============================================================

install: $(TARGET)
	mkdir -p $(ESP_DIR)/EFI/BOOT
	cp $(TARGET) $(ESP_DIR)/EFI/BOOT/BOOTX64.EFI


# ============================================================
# Debug
# ============================================================

$(DEBUG_VARS):
	cp "$(OVMF_VARS)" $(DEBUG_VARS)


debug: install $(DEBUG_VARS)
	qemu-system-x86_64 \
		-device virtio-tablet-pci \
		-drive if=pflash,format=raw,readonly=on,file="$(OVMF_CODE)" \
		-drive if=pflash,format=raw,file=$(DEBUG_VARS) \
		-drive format=raw,file=fat:rw:$(ESP_DIR)


run: debug


# ============================================================
# KVM package
# ============================================================

kvm: $(TARGET)
	rm -rf $(KVM_DIR)
	mkdir -p $(KVM_DIR)/esp/EFI/BOOT

	cp $(TARGET) \
		$(KVM_DIR)/esp/EFI/BOOT/BOOTX64.EFI

	cp "$(OVMF_CODE)" \
		$(KVM_DIR)/OVMF_CODE.fd

	cp "$(OVMF_VARS)" \
		$(KVM_DIR)/OVMF_VARS.fd

	@echo
	@echo "KVM package created:"
	@find $(KVM_DIR) -type f -print


# ============================================================
# KVM QCOW2 image
# ============================================================

kvm-qcow2: $(TARGET)
	rm -rf $(KVM_DIR)
	mkdir -p $(KVM_DIR)

	cp "$(OVMF_CODE)" \
		$(KVM_DIR)/OVMF_CODE.fd

	cp "$(OVMF_VARS)" \
		$(KVM_DIR)/OVMF_VARS.fd

	# 64MB RAW disk
	dd \
		if=/dev/zero \
		of=$(KVM_RAW_IMAGE) \
		bs=1M \
		count=$(IMAGE_SIZE_MB)

	# GPT
	parted -s $(KVM_RAW_IMAGE) \
		mklabel gpt

	# EFI System Partition
	parted -s $(KVM_RAW_IMAGE) \
		mkpart ESP fat32 1MiB 100%

	parted -s $(KVM_RAW_IMAGE) \
		set 1 esp on

	# FAT32
	mkfs.fat \
		-F 32 \
		--offset=$(ESP_OFFSET_SECTORS) \
		$(KVM_RAW_IMAGE)

	# EFI directory
	mmd \
		-i $(KVM_RAW_IMAGE)@@$(ESP_OFFSET_BYTES) \
		::/EFI

	mmd \
		-i $(KVM_RAW_IMAGE)@@$(ESP_OFFSET_BYTES) \
		::/EFI/BOOT

	# BOOTX64.EFI
	mcopy \
		-i $(KVM_RAW_IMAGE)@@$(ESP_OFFSET_BYTES) \
		$(TARGET) \
		::/EFI/BOOT/BOOTX64.EFI

	# RAW -> QCOW2
	qemu-img convert \
		-f raw \
		-O qcow2 \
		$(KVM_RAW_IMAGE) \
		$(QCOW2_IMAGE)

	rm -f $(KVM_RAW_IMAGE)

	@echo
	@echo "QCOW2 package created:"
	@ls -lh \
		$(QCOW2_IMAGE) \
		$(KVM_DIR)/OVMF_CODE.fd \
		$(KVM_DIR)/OVMF_VARS.fd


# ============================================================
# USB boot image
#
# Rufus等でUSBメモリへ書き込むRAWディスクイメージ
#
# system.img
#   GPT
#   EFI System Partition
#     FAT32
#       EFI/
#         BOOT/
#           BOOTX64.EFI
# ============================================================

usb: $(TARGET)
	rm -rf $(USB_DIR)
	mkdir -p $(USB_DIR)

	# 64MB RAW USB image
	dd \
		if=/dev/zero \
		of=$(USB_IMAGE) \
		bs=1M \
		count=$(IMAGE_SIZE_MB)

	# GPT
	parted -s $(USB_IMAGE) \
		mklabel gpt

	# EFI System Partition
	parted -s $(USB_IMAGE) \
		mkpart ESP fat32 1MiB 100%

	parted -s $(USB_IMAGE) \
		set 1 esp on

	# FAT32
	mkfs.fat \
		-F 32 \
		--offset=$(ESP_OFFSET_SECTORS) \
		$(USB_IMAGE)

	# EFI directory
	mmd \
		-i $(USB_IMAGE)@@$(ESP_OFFSET_BYTES) \
		::/EFI

	mmd \
		-i $(USB_IMAGE)@@$(ESP_OFFSET_BYTES) \
		::/EFI/BOOT

	# BOOTX64.EFI
	mcopy \
		-i $(USB_IMAGE)@@$(ESP_OFFSET_BYTES) \
		$(TARGET) \
		::/EFI/BOOT/BOOTX64.EFI

	@echo
	@echo "USB boot image created:"
	@ls -lh $(USB_IMAGE)


# ============================================================
# Clean
# ============================================================

clean:
	rm -rf \
		*.o \
		*.so \
		*.EFI \
		$(DEBUG_VARS) \
		$(ESP_DIR) \
		$(DIST_DIR)


# ============================================================
# Rebuild
# ============================================================

rebuild: clean all


# ============================================================
# Rebuild + Debug
# ============================================================

rr: rebuild
	$(MAKE) debug


# ============================================================
# Rebuild + KVM package
# ============================================================

rk: rebuild
	$(MAKE) kvm


# ============================================================
# Rebuild + KVM QCOW2
# ============================================================

rkq: rebuild
	$(MAKE) kvm-qcow2


# ============================================================
# Rebuild + USB image
# ============================================================

ru: rebuild
	$(MAKE) usb


.PHONY: \
	all \
	install \
	debug \
	run \
	kvm \
	kvm-qcow2 \
	usb \
	clean \
	rebuild \
	rr \
	rk \
	rkq \
	ru


