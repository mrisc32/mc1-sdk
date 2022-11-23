# MC1 example demo 2

This folder contains a bootable demo application for the MC1 computer.

## System requirements

* VRAM: 256 KB
* SD card reader

## Build & Run

Run `make` to build the demo, which will produce `out/demo2.elf`. Then copy the
file to a FAT formatted SD card (be sure to call the target file `MC1BOOT.EXE`),
e.g:

```bash
make
cp out/demo2.elf /path/to/sdcard/MC1BOOT.EXE
```

Insert the SD card into the MC1 computer and boot the computer.

## Credits

The cyberpunk car pictures ([Retrawave car](retrawave-car.png),
[Is that a supra?](is-that-a-supra.png) and [Phantom](phantom.png)) were created by
[Fernando Correa](https://www.artstation.com/fernandocorrea).

The [32x32 font](ming-charset-32x32.png) was created by "Ming" in 1988.
