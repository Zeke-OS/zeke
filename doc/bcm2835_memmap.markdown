BCM2835 Physical Memory Map
===========================

Following table explains the physical memory layout of BCM2835 as well as its
usage on Zeke. Some of these mappings are common to all ARM11 implementations.

    Address             |   | Description
    ----------------------------------------------------------------------------
    Interrupt Vectors:  |   |
    0x0 - 0xff          |   | Not used by Zeke
    0x100  - 0x4000     | L | Typical placement of ATAGs
    Priv Stacks:        |   |
    0x1000 - 0x2fff     | Z | Supervisor (SWI/SVC) stack
    0x3000 - 0x4fff     | Z | Abort stack
    0x5000 - 0x5fff     | Z | IRQ stack
    0x6000 - 0x6fff     | Z | Undef stack
    0x7000 - 0x7fff     | Z | System stack
    0x8000 - 0xfffff    | Z | Kernel area (boot address)
    0x00100000-         | Z | Page Table
    0x002FFFFF          |   | Area
    0x00300000          | Z | Dynmem
    0x00FFFFFF          |   | Area
    -                   |   |
    Peripherals:        |   |
    0x20000000 -        |   |
     Interrupts         |   |
    0x2000b200          | B | IRQ basic pending
    0x2000b204          | B | IRQ pending 1
    0x2000b20c          | B | IRQ pending 2
    0x2000b210          | B | Enable IRQs 1
    0x2000b214          | B | Enable IRQs 2
    0x2000b218          | B | Enable Basic IRQs
    0x2000b21c          | B | Disable IRQs 1
    0x2000b220          | B | Disable IRQs 2
    0x2000b224          | B | Disable Basic IRQs
    - 0x20FFFFFF        | B | Peripherals

Z = Zeke specific
L = Linux bootloader specific
B = BCM2835 firmware specific mappings
