# MC1 boot sequence

The MC1 first boots from ROM, which does minimal system initialization and tries to load and run an MRISC32 ELF32 executable from a FAT volume on an SD card.

## Initialization

The ROM initialization code sets up a small (512 bytes) stack at the top of VRAM. This stack is used by all ROM routines, but once control is handed over to the boot code the stack is expected to be redefined.

## Boot loader

The ROM code will:

* Initialize an SD card device (retry until a card is inserted).
* Mount any FAT volumes on the SD card.
* Load `MC1BOOT.EXE` from the root folder of the first bootable FAT volume.
* Call the entry point in the boot executable.

Note: If none of the FAT volumes have the `boot` flag set, the first found FAT volume will be used as the boot volume.

## Boot program (MC1BOOT.EXE)

The boot program is typically a small shell or launcher program that resides in the lower part of VRAM (see the [memory map](memory_map.md)).

The boot program must be an MRISC32 ELF32 binary file.

It needs to linked such that it does *not* overwrite the ROM BSS area. This is achieved by using the `boot-vram.ld` linker script.

## Application programs

Application programs can be loaded and executed from a boot program (e.g. a shell), and may also return control to the boot program (provided that they do not clobber the lower VRAM area).

Application programs must be MRISC32 ELF32 binary files.

### VRAM application program

If the application program is to be loaded into VRAM it needs to linked such that it does *not* overwrite the lower VRAM area that is reserved for the boot program (see the [memory map](memory_map.md)). This is achieved by using the `app-vram.ld` linker script.

Note that the size of the VRAM is limited, so only small programs can be loaded into VRAM.

### XRAM application program

If the boot program is to be loaded into XRAM it needs to linked such that it loads into XRAM. This is achieved by using the `app-xram.ld` linker script.

Note that XRAM may not be available, in which case the boot will fail.
