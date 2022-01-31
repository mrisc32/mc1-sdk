# MC1 boot sequence

The MC1 first boots from ROM, which does minimal system initialization and tries to load and run an MRISC32 ELF32 executable from a FAT volume on an SD card.

## Initialization.

The ROM initialization code sets up a small (512 bytes) stack at the top of VRAM. This stack is used by all ROM routines, but once control is handed over to the boot code the stack is expected to be redefined.

## Boot loader

The ROM code will:

* Initialize an SD card device (retry until a card is inserted).
* Mount any FAT volumes on the SD card.
* Load `MC1BOOT.EXE` from the root folder of the first bootable FAT volume.
* Call the entry point in the boot executable.

Note: If none of the FAT volumes have the `boot` flag set, the first found FAT volume will be used as the boot volume.

## MC1BOOT.EXE

The boot executable must be an MRISC32 ELF32 binary file.

### VRAM executable

If the boot program is to be loaded into VRAM it needs to linked such that it does *not* overwrite the ROM BSS area (see the [memory map](memory_map.md)). This is achieved by using the `app-vram.ld` linker script.

Note that the size of the VRAM is limited, so only small programs can be loaded into VRAM.

### XRAM executable

If the boot program is to be loaded into XRAM it needs to linked such that it loads into XRAM. This is achieved by using the `app-xram.ld` linker script.

Note that XRAM may not be available, in which case the boot will fail.
