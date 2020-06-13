Debugging with GDB
==================

Setup
-----

For the best possible debugging experience it's a good idea to compile GDB
specifically for Zeke. The following configure flags should work the best with
Zeke and its GDB Python scripts.

```
./configure --with-python=python3.4 --host=x86_64-linux-gnu --target=arm-none-eabi
```

If you wish to use [Zekedock](/docker), which you should as Zeke only works with
very specific versions of build and debug tools, it already includes GDB 7.10.

While running Zeke in QEMU gdb can be started by executing `rm-none-eabi-gdb`
which will automatically attach to the Zeke in QEMU, thanks to
[.gdbinit](/.gdbinit). It might be helpful to run these in `screen` when
using the Docker image. `screen` is preinstalled in the Docker image.

Debugging
---------

To get debugging symbols for the kernel (and user space executables) add the
`-g` flag to `CCFLAGS` in Kconfig: Build Config.

Debugging the kernel is probably easiest when Zeke is running in QEMU. If you
have QEMU listening as a GDB server already (`-s` option), it's enough to just
execute GDB with the repository root as a working directory. This way GDB will
read `.gdbinit`, and thus connecting to the server and loading all Zeke specific
debugging utils.

Debugging user space executables is currently not supported but core dump files
written by the kernel are mostly GDB compatible. Most of the tools capable of
reading Linux and/or freeBSD elf core files can also parse core files written
by Zeke.

### Get current PID and TID

```gdb
print current_process_id
print current_thread.id
```

### Set PC and SP if kernel crashed during DAB

```gdb
set $pc = 0xYYY
set $sp = 0xXXX
```

### Get thread stack before panic in DAB

```gdb
print/x current_thread.kstack_region.b_data
$2 = 0x403000
set $sp = 0x403f28 # PC should have been printed to the console
set $pc = 0x0002311c
```
