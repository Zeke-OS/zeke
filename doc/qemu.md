Running Zeke in QEMU
====================

Building and Running QEMU
-------------------------

QEMU from rpi branch of `git@github.com:Torlus/qemu.git` repository seems to work
best for BCM2835/Raspberry Pi version of Zeke. It's downloaded and built from our
own mirror in the [Zekedock Dockerfile](/docker/Dockerfile). Currently no other
machine targets are supported with Qemu.

```
$ ../src/configure --prefix=[PATH TO BIN] --target-list=arm-softmmu,arm-linux-user,armeb-linux-user --enable-sdl
$ make -j4 && sudo make install
```

Once you have QEMU installed you can run Zeke in Qemu by using the following
build target.

```bash
$ make qemu
```

This command also works inside Zekedock but without SDL.

See [Debugging with GDB](gdb.md) if you are interested in debugging Zeke in GDB.

Building Zeke for QEMU
----------------------

```
$ KBUILD_DEFCONFIG=qemu_rpi_headless_testing.defconfig ./configure defconfig
$ ./configure menuconfig
$ make all opttest -j4
```

`KBUILD_DEFCONFIG` can be set as any of the defconfig targets found under
the `defconfigs` directory but to run QEMU you'll need one of the files
prefixed with `qemu_`.

Finally you can build the rootfs image by running:

```
$ make rootfs
```
