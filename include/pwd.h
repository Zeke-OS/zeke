/**
 *******************************************************************************
 * @file    pwd.h
 * @author  Olli Vanhoja
 * @brief   Password structure.
 * @section LICENSE
 * Copyright (c) 2020 Olli Vanhoja <olli.vanhoja@alumni.helsinki.fi>
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

/**
 * @addtogroup pwd
 * @{
 */

/**
 * The passwd struct.
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

/**
 * Get password file entry.
 * The first time getpwent() is called, it returns the first entry;
 * thereafter, it returns successive entries.
 */
struct passwd * getpwent(void);

/**
 * Get password file entry.
 * Returns the entry that matches to the username nam.
 */
struct passwd * getpwnam(char * nam);

/**
 * Get password file entry.
 * Return the entry that matches to the user ID uid.
 */
struct passwd * getpwuid(uid_t uid);

/**
 * Rewind to the begining of the password database.
 */
int setpwent(void);

/**
 * Rewind to the begining of the password database.
 * If stayopen is non-zero the file descriptors are left open on subsequent
 * calls to getpwnam() and getpwuid(), other pwd functions are not affected.
 *
 * Long running processes should not keep the file descriptors open for long
 * periods of time as the database might be updated during the runtime of the
 * process.
 */
int setpassent(int stayopen);

/**
 * Close open files related to pwd.
 */
void endpwent(void);

__END_DECLS
#endif /* !KERNEL_INTERNAL */

/**
 * @}
 */

#endif /* PWD_H */

/**
 * @}
 */
