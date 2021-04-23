# MC1 application examples

This folder contains bootable application examples for the MC1 computer.

Run `make` to build the example applications, which will be stored as `*.img`
files in the `out/` folder. An image can be installed on an SD card using
[`dd`](https://linux.die.net/man/1/dd), e.g:

```bash
dd if=out/demo1.img of=/dev/sdX
```

Replace `/dev/sdX` with the SD card device.

**IMPORTANT:** *Using the wrong device may corrupt the system disk on your host computer*.

## Credits

The cyberpunk car pictures ([Retrowave car](retrawave-car.png),
[Is that a supra?](is-that-a-supra.png) and [Phantom](phantom.png)) were created by
[Fernando Correa](https://www.artstation.com/fernandocorrea).

The [32x32 font](ming-charset-32x32.png) was created by "Ming" in 1988.
