# MC1 application examples

This folder contains bootable application examples for the MC1 computer.

Run `make` to build the example applications, which will be stored as `*.img`
files in the `out/` folder. An image can be installed on an SD card using
[`dd`](https://linux.die.net/man/1/dd), e.g:

```bash
dd if=out/boot-minimal.img of=/dev/sdX
```

Replace `/dev/sdX` with the SD card device.

**IMPORTANT:** *Using the wrong device may corrupt the system disk on your host computer*.

