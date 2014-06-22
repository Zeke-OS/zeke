# Kernel config

# Supported load average calculation periods
SCHED_LAVGPERIOD_5SEC  = 5#
SCHED_LAVGPERIOD_11SEC = 11#

# User configurable part begins ################################################

configATAG = 0# Support bootloader ATAGs
configMP = 0# Enable MP support
configMMU = 1# MMU support
configUART = 1# UART support
configUART_MAX_PORTS = 2# Maximum number of UART ports
configFB = 0# Enable Frame Buffer support
configFB_FONT_LATIN = 0# Latin extension
configFB_FONT_GREEK = 0# Greek extension
configFB_FONT_HIRAGANA = 0# Hiragana extension
#define HW
configBCM_MB = 0# BCM2835 mailboxes
configBCM_JTAG = 0# BCM2835 JTAG support
configRPI_LEDS = 0# Raspberry PI leds
configRPI_EMMC = 0# Raspberry PI EMMC

# Kernel logging options
configKLOGGER = 1# Enable KERROR as kernel logger
configKERROR_BUF_SIZE = 2048# Lastlog buf size
configKERROR_MAXLEN = 400# Max kerror line length
configKERROR_UART = 1# Option to output kernel logs directly to UART0
configKERROR_FB = 0
# 0 = No logger
# 1 = Kerror buffer
# 2 = UART0
# 3 = FB
configDEF_KLOGGER = 2# Select default logger (options above)

# loglevel
# Additional log messages and asserts.
# These are one to one with KERROR message levels, although meanings are bit
# different when considered the meaning of the message. E.g. most of level 3
# messages are printed anyway.
# 4 - All debug messages and asserts.
# 3 . Additional info messages.
# 2 - Additional warnings.
# 1 - Additional error messages and asserts for serious errors that should not
#     occur in normal operation.
# 0 - Normal build
configDEBUG = '4'#Additional debug messages and asserts.
# end of logging options

# Processes
configMAXPROC = 5# maxproc, default maximum number of processes.

# Scheduler Selection
configSCHED_TINY = 1#
#
# SCHED_TINY Config
configSCHED_MAX_THREADS = 10# Maximum number of threads.
configSCHED_HZ = 100u# Scheduler frequency in Hz.
configSCHED_LAVG_PER = SCHED_LAVGPERIOD_11SEC#

configTIMERS_MAX = 4# Maximum number of timers available

# String functions
configSTRING_OPT_SIZE = 0# Optimize string functions for speed or size

# usrinit
configUSRINIT_SSIZE = 8192# Stack size for usrinit thread
configUSRINIT_PRI = osPriorityNormal# Priority of usrinit thread

# Virtual file system and file systems
configRAMFS = 1# Compile ramfs with Zeke base

# In-Kernel Unit Tests
configKUNIT = 1# Compile KUnit and tests
configKUNIT_REPORT_ORIENTED = 0#
configKUNIT_GENERIC = 0# Tests for generic data structures
configKUNIT_KSTRING = 0# Tests for kstring library
configKUNIT_FS = 1# Tests for vfs and filesystems

# User space
# TODO This should be possibly somewhere else
configTISH = 1# Debug console in init
