Contributing
============

Feature Requests, Pull Requests, Bugs
-------------------------------------

Cool, just file it and let's discuss about it.

Setting Up the Build and Test Envrionment
-----------------------------------------

The easiest way to run Zeke is by using the ready built
[Docker image](https://hub.docker.com/r/olliv/zekedock/) and
[dmk](https://github.com/Zeke-OS/dmk).

```bash
$ dmk
you@abc:/build$ KBUILD_DEFCONFIG=qemu_rpi_headless_testing.defconfig ./configure defconfig
you@abc:/build$ make all -j5
you@abc:/build$ make qemu
```

After making the defconfig you can change the configuration knobs by running
`./configure menuconfig`. More information regarding `configure` script can be
found [here](https://github.com/Zeke-OS/zeke/wiki/configure-script).

The userland unit tests must be built by specifying a special target `opttest`
as `all` doesn't include tests. The tests can be found under `opt/test` when
running Zeke, which means the tests must be run inside Zeke as thebinaries are
compiled for Zeke using its libc.

The unit tests for the kernel (kunit) can be executed by writing the name of
each test to `/proc/kunit` and available tests can be listed by reading the
same file.

The Docker image comes with a GCC version specially built for debugging the
Zeke kernel.
[Here](https://github.com/Zeke-OS/zeke/wiki/Debugging-with-GDB) is some
documentation about the best practises and provided macros.

Creation and building new user space programs is documented
[here](https://github.com/Zeke-OS/zeke/wiki/Creating-a-new-user-space-program).

Roadmap
-------

Let's make a realtimish, securish, clean, and minimal operating system for
cheap processors having MMU, especially ARMs.

Contacting the Author(s)
------------------------

Please don't use email.
