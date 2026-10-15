TARGET = BOOTX64.EFI

CC = gcc


# ============================================================
# GNU-EFI
# ============================================================

EFI_INC = \
	-I/usr/include/efi \
	-I/usr/include/efi/x86_64

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

RAW_IMAGE = $(KVM_DIR)/system.raw
QCOW2_IMAGE = $(KVM_DIR)/system.qcow2

IMAGE_SIZE_MB = 64

ESP_OFFSET_SECTORS = 2048
SECTOR_SIZE = 512
ESP_OFFSET_BYTES = 1048576


# ============================================================
# Build
# ============================================================

all: $(TARGET)


main.o: main.c
	$(CC) $(CFLAGS) -c $< -o $@


boot.so: main.o
	$(CC) \
		$(LDFLAGS) \
		/usr/lib/crt0-efi-x86_64.o \
		main.o \
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
#
# EFIファイル + OVMF
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
#
# system.raw
#   GPT
#   EFI System Partition
#   FAT32
#   EFI/BOOT/BOOTX64.EFI
#
# ↓
#
# system.qcow2
# ============================================================

kvm-qcow2: $(TARGET)
	rm -rf $(KVM_DIR)
	mkdir -p $(KVM_DIR)

	cp "$(OVMF_CODE)" \
		$(KVM_DIR)/OVMF_CODE.fd

	cp "$(OVMF_VARS)" \
		$(KVM_DIR)/OVMF_VARS.fd

	# --------------------------------------------------------
	# 64MB RAW disk
	# --------------------------------------------------------

	dd \
		if=/dev/zero \
		of=$(RAW_IMAGE) \
		bs=1M \
		count=$(IMAGE_SIZE_MB)

	# --------------------------------------------------------
	# GPT + EFI System Partition
	# --------------------------------------------------------

	parted -s $(RAW_IMAGE) \
		mklabel gpt

	parted -s $(RAW_IMAGE) \
		mkpart ESP fat32 1MiB 100%

	parted -s $(RAW_IMAGE) \
		set 1 esp on

	# --------------------------------------------------------
	# FAT32 filesystem
	#
	# Partition begins at 1MiB
	# --------------------------------------------------------

	mkfs.fat \
		-F 32 \
		--offset=$(ESP_OFFSET_SECTORS) \
		$(RAW_IMAGE)

	# --------------------------------------------------------
	# Copy BOOTX64.EFI using mtools
	# --------------------------------------------------------

	mmd \
		-i $(RAW_IMAGE)@@$(ESP_OFFSET_BYTES) \
		::/EFI

	mmd \
		-i $(RAW_IMAGE)@@$(ESP_OFFSET_BYTES) \
		::/EFI/BOOT

	mcopy \
		-i $(RAW_IMAGE)@@$(ESP_OFFSET_BYTES) \
		$(TARGET) \
		::/EFI/BOOT/BOOTX64.EFI

	# --------------------------------------------------------
	# RAW -> QCOW2
	# --------------------------------------------------------

	qemu-img convert \
		-f raw \
		-O qcow2 \
		$(RAW_IMAGE) \
		$(QCOW2_IMAGE)

	# RAWは配布不要
	rm -f $(RAW_IMAGE)

	@echo
	@echo "QCOW2 package created:"
	@echo
	@ls -lh \
		$(QCOW2_IMAGE) \
		$(KVM_DIR)/OVMF_CODE.fd \
		$(KVM_DIR)/OVMF_VARS.fd


# ============================================================
# USB files
# ============================================================

usb: $(TARGET)
	rm -rf $(USB_DIR)

	mkdir -p $(USB_DIR)/EFI/BOOT

	cp $(TARGET) \
		$(USB_DIR)/EFI/BOOT/BOOTX64.EFI

	@echo
	@echo "USB package created:"
	@find $(USB_DIR) -type f -print


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
# Rebuild + USB
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


