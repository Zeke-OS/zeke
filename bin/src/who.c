/**
 *******************************************************************************
 * @file    vmmap.c
 * @author  Olli Vanhoja
 * @brief   Show who is logged on.
 * @section LICENSE
 * Copyright (c) 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <stdlib.h>
#include <sys/sysctl.h>
#include <sys/proc.h>
#include <sysexits.h>

static size_t get_sessions(struct kinfo_session ** sessions)
{
    int mib[3];

    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_SESSION;

    for (int i = 0; i < 3; i++) {
        size_t size = 0;
        struct kinfo_session * s;

        if (sysctl(mib, num_elem(mib), NULL, &size, 0, 0)) {
            return 0;
        }

        s = malloc(size);
        if (!s)
            return 0;

        if (sysctl(mib, num_elem(mib), s, &size, 0, 0)) {
            free(s);
            continue;
        }
        *sessions = s;
        return size / sizeof(struct kinfo_session);
    }
    return 0;
}

int main(int argc, char * argv[], char * envp[])
{
    struct kinfo_session * sessions;
    struct kinfo_session * entry;
    size_t n;

    n = get_sessions(&sessions);
    if (n == 0) {
        perror("Failed to get a list of all sessions, try again later");
        return EX_NOINPUT;
    }

    entry = sessions;
    for (size_t i = 0; i < n; i++) {
        if (entry->s_login[0] != '\0') {
            printf("%s %d %d\n",
                   entry->s_login,
                   entry->s_leader,
                   entry->s_ctty_fd);
        }
        entry++;
    }

    free(sessions);
    return EX_OK;
}
