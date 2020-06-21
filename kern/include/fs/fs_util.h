/**
 *******************************************************************************
 * @file    fs_util.h
 * @author  Olli Vanhoja
 * @brief   Virtual file system utils.
 * @section LICENSE
 * Copyright (c) 2020 Olli Vanhoja <olli.vanhoja@alumni.helsinki.fi>
 * Copyright (c) 2013 - 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#pragma once
#ifndef FS_UTIL_H
#define FS_UTIL_H

struct fs;
struct fs_superblock;
struct vnode;
struct vnode_ops;

/**
 * Init superblock.
 * Updates:
 * - fs set properly
 * - root pointer set to NULL
 * @param[out] sbn is the target superblock node.
 * @param[in] fs is the file system.
 */
void fs_init_superblock(struct fs_superblock * sb, struct fs * fs);

/**
 * Insert suberblock to the list of super blocks in fs struct.
 */
void fs_insert_superblock(struct fs * fs, struct fs_superblock * new_sb);

/**
 * Remove a given sb from the sb mount list.
 */
void fs_remove_superblock(struct fs * fs, struct fs_superblock * sb);

/**
 * Iterate over all superblocks mounted on the file system.
 */
struct fs_superblock *
fs_iterate_superblocks(fs_t * fs, struct fs_superblock * sb);

/**
 * Create a new pseudo file system root.
 * Create a new pseudo fs root by inheriting ramfs implementation
 * as a basis. The created fs is a good basis for ram based file
 * systems like procfs but probably not the best basis for a
 * disk backed file system.
 */
struct vnode * fs_create_pseudofs_root(struct fs * newfs, int majornum);

/**
 * Inherit unset vnops from another file system.
 * @param dest_vnops is a vnode_ops struct containing pointers to vnops that
 *                   are actually implemented in the target file system
 *                   and other functions pointers shall be set to NULL.
 * @param base_vnops is the source used for vnops that are unimplemented in the
 *                   target file system.
 */
void fs_inherit_vnops(struct vnode_ops * dest_vnops,
                      const struct vnode_ops * base_vnops);

/**
 * Initialize a vnode.
 * This is the prefferred method to initialize a vnode because vnode struct
 * internals may change and this function is (usually) kept up-to-date.
 */
void fs_vnode_init(struct vnode * vnode, ino_t vn_num,
                   struct fs_superblock * sb,
                   const struct vnode_ops * const vnops);

/**
 * Cleanup some vnode data.
 * File system is responsible to call this function before deleting a vnode.
 * This handles following cleanup tasks:
 * - Release and write out buffers
 */
void fs_vnode_cleanup(struct vnode * vnode);


/**
 * **Usage:**
 *
 * ```
 * struct my_parm {
 *     char *val;
 *     char *bool1;
 *     char *bool2;
 * };
 * ...
 * char parm[] = "val=text;bool1";
 * struct my_parm my_parm;
 * fs_parse_parm(parm, (const char*[]){ "val", "bool1", "bool2" },
 *               &parsed, sizeof(parsed));
 * ```
 *
 * `my_parm.val` will no contain a pointer to `"text"` of `parm` while `;` has
 * been replaced with `'\0'`. Since `bool1` doesn't have a value `my_parm.bool1`
 * now points to a const string `"y"` and `my_parm.bool2` points to `NULL`.
 *
 * @param parm  is a pointer to a writable copy of a null-terminated mount
 *              parameter string. The expected parameter format:
 *              `parm=value;boolp;parm2=val`
 * @param names a NULL-terminated array of pointers to strings of expected
 *              parameter names (argv-, envp-like).
 * @param parsed is a pointer to a struct which has pointers to strings in
 *              the same order as `names` has them.
 * @param parse_size is the size of `parsed` in bytes.
 */
void fs_parse_parm(char * parm, const char * names[], void * parsed, size_t parsed_size);

#endif /* FS_UTIL_H */
