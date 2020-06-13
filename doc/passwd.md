passwd: User Management
=======================

Currently Zeke implements a very traditional Unix-like user management system.

There are no hard-coded meanings for UIDs or GIDs in Zeke at kernel level nor
in the userland. However, if the fatfs driver is used then the number of
practically available IDs is severely limited as FAT can only store 8bits
of an ID. Therefore, a mapping is being made for convenience and to provide a
some sort of compatibility with the traditional use of UIDs and GIDs. The
following tables shows the recommended layout and limits of each ID group.

| UID           | Description           |
|---------------|-----------------------|
| 0             | Root user             |
| 1-63          | System users          |
| 100-163       | Admins                |
| 500-563       | Regular users         |
| 1000-1063     | External users        |

| GID           | Description           |
|---------------|-----------------------|
| 0             | Wheel group (admins)  |
| 1-63          | System users          |
| 100-163       | Admins                |
| 500-563       | Regular users         |
| 1000-1063     | External users        |

Runtime Files
-------------

The following files are available in a running system.

### /etc/passwd

The file contains newline separated records, each line containing colon (`:`)
separated fields. The fields are as follows:
1. Username: Used as a login username. 1 to `MAXLOGNAME` characters in length,
             shouldn't start with dash (`-`).
2. Password: A number value indicates that encrypted password is stored in
             /etc/shadow file with an offset indicated by the number.
3. UID:      User ID. Zero (`0`) is reserved for root.
4. GID:      The primary group. (Stored in `/etc/group`)
5. GECOS:    Full name.
6. Home dir: User's home directory.
7. Shell:    The command that's executed when the user logs in.

See the [default file](/etc/passwd).

### /etc/shadow

Contains the encrypted or hashed passwords.

### /etc/group

Lists the groups available in the system.

See the [default file](/etc/group).


Libc
----

### /include/pwd.h

The header file `/include/pwd.h` defines a standard Unix-like interface for
reading the password database.
