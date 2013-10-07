BCM2835 Physical Memory Map
===========================

Following table explains the physical memory layout of BCM2835 as well as its
usage on Zeke.

    Address           | Zeke Specific                  Description
    --------------------------------------------------------------
    Interrupt Vectors:|
    0x0000 -          |
    Priv Stacks:      |
    0x2000 - 0x2400   | x               Supervisor (SWI/SVC) stack
    0x2401 - 0x2800   | x               Abort stack
    0x2801 - 0x2c00   | x               IRQ stack
    0x2c01 - 0x3c00   | x               System stack
    -                 |
    0x3c01 - 0x7fff   |                 Free space
    0x8000 -          |                 Kernel (boot address)
    -                 |
    0x20000000 -      |
    0x20FFFFFF        |                 Peripherals

