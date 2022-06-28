# Dhrystone for MC1

This folder contains a bootable Dhrystone application for the MC1 computer.

## System requirements

* VRAM: ? KB
* SD card reader

## Build & Run

Run `make` to build the application, which will produce `out/mc1dhry.elf`. Then
copy the file to a FAT formatted SD card (be sure to call the target file
`MC1BOOT.EXE`), e.g:

```bash
make
cp out/mc1dhry.elf /path/to/sdcard/MC1BOOT.EXE
```

Insert the SD card into the MC1 computer and boot the computer.

