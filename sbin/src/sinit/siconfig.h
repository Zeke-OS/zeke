/* See LICENSE file for copyright and license details. */

static char *const rcinitcmd[] = { "/bin/sh", "/sbin/rc.init", NULL };
static char *const rcrebootcmd[] = { "/bin/sh", "/bin/rc.shutdown",
                                     "reboot", NULL };
static char *const rcpoweroffcmd[] = { "/bin/sh", "/bin/rc.shutdown",
                                       "poweroff", NULL };
