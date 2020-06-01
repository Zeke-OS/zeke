MIPS Build
==========

**Compile binutils**

```
mkdir /usr/src/binutils
cd /usr/src/binutils
chown
wget http://ftp.gnu.org/gnu/binutils/binutils-2.24.tar.gz
```

make & make install script:

```bash
    #!/bin/bash
    export WDIR=$(pwd)
    export TARGET=mipsel-sde-elf
    export PREFIX=/opt/cross
    export PATH="${PATH}":${PREFIX}/bin

    mkdir -p ${TARGET}-toolchain && cd ${TARGET}-toolchain
    tar xf /usr/src/binutils/binutils-2.24.tar.gz
    mkdir -p build-binutils && cd build-binutils
    ../binutils-2.24/configure --target=$TARGET --prefix=$PREFIX
    make -j9
    sudo make install
```

Update `PATH`.

**Select target in Zeke Kconfig**
