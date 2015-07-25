/**
 *******************************************************************************
 * @file    grp.h
 * @author  Olli Vanhoja
 * @brief   Group database.
 * @section LICENSE
 * Copyright (c) 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

/**
 * @addtogroup LIBC
 * @{
 */

#ifndef GRP_H
#define GRP_H

#include <sys/types/_gid_t.h>
#include <sys/types/_size_t.h>

struct group {
    char   *gr_name;    /*!< The name of the group. */
    gid_t   gr_gid;     /*!< Numerical group ID. */
    char  **gr_mem;     /*!< Pointer to a null-terminated array of character
                         *   pointers to member names. */
};

#ifndef KERNEL_INTERNAL
__BEGIN_DECLS

struct group * getgrent(void);
struct group * getgrgid(gid_t gid);
struct group * getgrnam(const char * name);
int getgrouplist(char * uname, gid_t agroup, gid_t * groups, int * grpcnt);
void endgrent(void);
void setgrent(void);

__END_DECLS
#endif /* !KERNEL_INTERNAL */

#endif /* GRP_H */

/**
 * @}
 */
