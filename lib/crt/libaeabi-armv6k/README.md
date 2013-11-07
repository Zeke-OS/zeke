ARM Run-Time ABI for the Cortex-M0 processor
============================================

This library implements the Run-Time ABI for the ARM architecture as defined
in document ARM IHI 0043C (http://infocenter.arm.com) for the Thumb-2 ISA
subset of the Cortex-M0.


So Far Implemented
------------------

~~~~
__aeabi_lmul()
__aeabi_ldivmod()
__aeabi_uldivmod()
__aeabi_llsl()
__aeabi_llsr()
__aeabi_lasr()
__aeabi_idiv()
__aeabi_uidiv()
__aeabi_idivmod()
__aeabi_uidivmod()
__aeabi_memset8()
__aeabi_memset4()
__aeabi_memset()
__aeabi_memclr8()
__aeabi_memclr4()
__aeabi_memclr()
~~~~


Additional libgcc wrapper functions
-----------------------------------
Ironically they are not needed for gcc, which uses the aeabi functions, but for
LLVM.

~~~~
__muldi3()
__moddi3()
__divdi3()
__umoddi3()
__udivdi3()
__ashldi3()
__lshrdi3()
__ashrdi3()
__modsi3()
__divsi3()
__umodsi3()
__udivsi3()
~~~~


Building a compiler that uses the runtime library
-------------------------------------------------

### binutils

Install binutils to the directory $TARGETDIR. The sourcecode can be downloaded
from ftp://ftp.gnu.org/gnu/binutils/

~~~~
$ tar xf binutils-2.23.1.tar.bz2
$ mkdir btmp
$ cd btmp
$ ../binutils-2.23.1/configure --target=arm-none-eabi --prefix=$TARGETDIR \
    --disable-interwork --disable-multibib --disable-nls --disable-libssp
$ make
$ sudo make install
$ export PATH=$PATH:$TARGETDIR/bin
~~~~

### GNU C

The GNU C compiler can be installed to the same directory $TARGETDIR. The
sourcecode can be downloaded from ftp://ftp.gnu.org/gnu/gcc/

~~~~
$ tar xf gcc-4.7.2.tar.bz2
$ mkdir gtmp
$ cd gtmp
$ ../gcc-4.7.2/configure --target=arm-none-eabi --prefix=$TARGETDIR \
    --disable-interwork --disable-multibib --disable-nls --disable-libssp \
    --enable-languages="c" --with-float="soft"
$ make
$ sudo make install
~~~~

### clang / LLVM


License
-------
Licensed under the ISC licence (similar to the MIT/Expat license).
