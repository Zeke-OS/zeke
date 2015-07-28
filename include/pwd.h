/**
 *******************************************************************************
 * @file    pwd.h
 * @author  Olli Vanhoja
 * @brief   Password structure.
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

#ifndef PWD_H
#define PWD_H

#include <sys/types/_gid_t.h>
#include <sys/types/_size_t.h>
#include <sys/types/_uid_t.h>

/*
 * /etc/passwd file format
 *
 * The file contains newline separated records, each line containing
 * colon (`:') separated fields. The fields are as follows:
 *
 * 1. Username: Used as a login username. 1 to MAXLOGNAME characters in length,
 *              shouldn't start with dash (`-').
 * 2. Password: A number value indicates that encrypted password is stored in
 *              /etc/shadow file with an offset indicated by the number.
 * 3. UID:      User ID. Zero (0) is reserved for root.
 * 4. GID:      The primary group. (Stored in /etc/group)
 * 5. GECOS:    Full name.
 * 6. Home dir: User's home directory.
 * 7. Shell:    The command that's executed when the user logs in.
 */

struct passwd {
    char    *pw_name;   /*!< User's login name. */
    char    *pw_passwd; /*!< Encrypted password. */
    uid_t    pw_uid;    /*!< Numerical user ID. */
    gid_t    pw_gid;    /*!< Numerical group ID. */
    char    *pw_gecos;  /*!< Real name. */
    char    *pw_dir;    /*!< Initial working directory. */
    char    *pw_shell;  /*!< Program to use as shell. */
};

#ifndef KERNEL_INTERNAL
__BEGIN_DECLS

struct passwd * getpwent(void);
struct passwd * getpwnam(char * nam);
struct passwd * getpwuid(uid_t uid);
int setpwent(void);
int setpassent(int stayopen);
void endpwent(void);

__END_DECLS
#endif /* !KERNEL_INTERNAL */

#endif /* PWD_H */

/**
 * @}
 */
