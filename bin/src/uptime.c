/**
 *******************************************************************************
 * @file    uptime.c
 * @author  Olli Vanhoja
 * @brief   uptime command
 * @section LICENSE
 * Copyright (c) 2020 Olli Vanhoja <olli.vanhoja@alumni.helsinki.fi>
 * Copyright (c) 2014, 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *******************************************************************************
 */

#include <stdio.h>
#include <time.h>
#include <sys/resource.h>

static void fmt_lavg(char * dst, double load)
{
    const int x = (int)(load);
    const int y = (int)(load * 100.0) % 100;

    sprintf(dst, "%d.%.2d", x, y);
}

int main(void)
{
    struct timespec ts = {0, 0};
    double loads[3];
    char l1[5];
    char l2[5];
    char l3[5];

    if (clock_gettime(CLOCK_UPTIME, &ts))
        return 1;

    getloadavg(loads, 3);
    fmt_lavg(l1, loads[0]);
    fmt_lavg(l2, loads[1]);
    fmt_lavg(l3, loads[2]);

    printf("%u.%u %s, %s, %s\n",
           (unsigned)ts.tv_sec, (unsigned)ts.tv_nsec, l1, l2, l3);

    return 0;
}
