/**
 *******************************************************************************
 * @file    startup_bcm2835.
 * @brief   Handle .{pre_init,init,fini}_array sections.
 *
 * Copyright (C) 2013 Olli Vanhoja
 * Copyright (C) 2004 CodeSourcery, LLC
 *
 * Permission to use, copy, modify, and distribute this file
 * for any purpose is hereby granted without fee, provided that
 * the above copyright notice and this notice appears in all
 * copies.
 *
 * This file is distributed WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *******************************************************************************
 */

/* These magic symbols are provided by the linker.  */
extern void (*__init_array_start []) (void) __attribute__((weak));
extern void (*__init_array_end []) (void) __attribute__((weak));
extern void (*__fini_array_start []) (void) __attribute__((weak));
extern void (*__fini_array_end []) (void) __attribute__((weak));

/**
 * Iterate over all the init routines.
 */
void __libc_init_array(void)
{
  int count;
  int i;

  count = __init_array_end - __init_array_start;
  for (i = 0; i < count; i++)
    __init_array_start[i] ();
}

/**
 *  Run all the cleanup routines.
 */
void __libc_fini_array(void)
{
  int count;
  int i;

  count = __fini_array_end - __fini_array_start;
  for (i = 0; i < count; i++) {
    __fini_array_start[i] ();
  }
}
