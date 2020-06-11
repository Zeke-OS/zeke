irq: Interrupt Handling
=======================

[irq.c](/kern/hal/irq.c) provides a generic HW-independent interface for
managing interrupts. The external API is defined in
[hal/irq.h](/kern/include/hal/irq.h).

```
+-----+                          +-------+
| irq |-----------------------+--| sched |
+-----+                       |  +-------+
   |                          |
+-------------------+ +----------------+
| bcm2835_interrupt | | bcm2835_timers |
+-------------------+ +----------------+
```

Figure: IRQ subsystem and its connections

A new interrupt handler can be registered at a bootup or at any point during
runtime by calling `irq_register()` and deregistered by calling
`irq_deregister()`.

```c
static struct irq_handler bcm2835_timer_irq_handler = {
    .name = "ARM Timer",
    .ack = arm_timer_ack,
    .handle = arm_timer_handle,
};

...
	irq_register(irq_num, &bcm2835_timer_irq_handler);
```

The `irq_handler` structure passed as an argument must be available in heap
during the whole time the IRQ handler remains registered. The `ack()` function
determines the IRQ system behavior, whether an incoming interrupt needs to be
handled immediately by calling the handler, or whether the `ack()` function
took care of the interrupt, or whether the IRQ handler thread should call
the handler once its scheduled. The last one is called deferred handling and
its typically preferred for interrupts that have no realtime requirements
and can wait for fair scheduling.

```c
/**
 * IRQ Acknowledge Types.
 */
enum irq_ack {
    IRQ_HANDLED = 0,    /*!< IRQ handled and no further processing is needed. */
    IRQ_NEEDS_HANDLING, /*!< Call handler immediately. */
    IRQ_WAKE_THREAD     /*!< Handle in an IRQ handler thread. */
};
```

The `irq_handler` structure also has an optional flag `allow_multiple` for
allowing the kernel to receive the same interrupt multiple times. This is
only meaningful for the threaded interrupt handling. If `allow_multiple` is
not set then specific IRQ is never disabled unless the `ack()` function or
the `handle()` function does so.

Relation to the Scheduler
-------------------------

The IRQ subsystem is directly responsible for taking care that the scheduler
is executed periodically. This is practically achieved by implementing a
timer interrupt handler that configures the timer to interrupt on every
`configSCHED_HZ`.

A timer scheduler *MUST* be registered in a `HW_POSTINIT` [constructor](boot.md)
to make the scheduler work. (See the `bcm_interrupt_postinit()` function in
[bcm2835_timers.c](/kern/hal/bcm2835/bcm2835_timers.c)). The timer interrupt
handler should decide when the scheduler should run by returning
`IRQ_NEEDS_HANDLING` from the `ack()` function. The scheduler timer must never
return `IRQ_WAKE_THREAD` from the `ack()` function. The `handler()` function of
the timer interrupt handler must call at least the following functions:

- `sched_handler()`
- `hw_timers_run()`

procfs
------

The irq subsystem provides a debugging interface in `procfs`.

```
# cat /proc/irq
0: 1512 ARM Timer
```

The file format has the following columns:

- IRQ number (0-64),
- Interrupt counter i.e. the number of times the interrupt has been triggered,
- Name of the interrupt handler.
