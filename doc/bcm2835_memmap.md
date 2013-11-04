BCM2835 Physical Memory Map
===========================

Following table explains the physical memory layout of BCM2835 as well as its
usage on Zeke. Some of these mappings are common to all ARM11 implementations.

    Address             | Z | Description
    ----------------------------------------------------------------------------
    Interrupt Vectors:  |   |
    0x0000 -            |   | Default, Not used by Zeke
    Priv Stacks:        |   |
    0x2000 - 0x2400     | x | Supervisor (SWI/SVC) stack
    0x2401 - 0x2800     | x | Abort stack
    0x2801 - 0x2c00     | x | IRQ stack
    0x2c01 - 0x3c00     | x | System stack
    -                   |   |
    0x3c01 - 0x7fff     |   | Free space
    0x8000 - 0xfffff    | x | Kernel area (boot address)
    0x00100000-         | x | Page Aable
    0x002FFFFF          |   | Area
    0x00300000          | x | Dynmem
    0x00FFFFFF          |   | Area
    -                   |   |
    Peripherals:        |   |
    0x20000000 -        |   |
     Interrupts         |   |
    0x2000b200          |   | IRQ basic pending
    0x2000b204          |   | IRQ pending 1
    0x2000b20c          |   | IRQ pending 2
    0x2000b210          |   | Enable IRQs 1
    0x2000b214          |   | Enable IRQs 2
    0x2000b218          |   | Enable Basic IRQs
    0x2000b21c          |   | Disable IRQs 1
    0x2000b220          |   | Disable IRQs 2
    0x2000b224          |   | Disable Basic IRQs
    - 0x20FFFFFF        |   | Peripherals

Z = Zeke specific
