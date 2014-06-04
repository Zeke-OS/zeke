# Kernel config

# Supported MCU models
MCU_MODEL_BCM2835 = 1

# Supported architectures
# -----------------------
# Even though v is missing between ARM and number these are not processor models
# but architecture versions. To mix it even further for some historical reasons
# we will detect processor core model mainly from its architecture version.
# Also profile identification is used for newer models and complete model number
# for some very vendor/model specific stuff like interupts.
__ARM4T__   = 1
#__ARM4TM__ = 2
#__ARM5__   = 3
#__ARM5E__  = 4
__ARM6__    = 5
__ARM6K__   = 6
__ARM6M__   = 7
#__ARM6SM__ = 8
#__ARM7M__  = 9
#__ARM7EM__ = 10
#__ARM7A__  = 11
#__ARM7R__  = 12
# Notes:
# MCU_MODEL_STM32F0 = __ARM6M__
# MCU_MODEL_STR912F = __ARM4T__
# MCU_MODEL_BCM2835 = __ARM6__

# Supported load average calculation periods
SCHED_LAVGPERIOD_5SEC  = 5#
SCHED_LAVGPERIOD_11SEC = 11#

# User configurable part begins ################################################

# Kernel configuration
configMCU_MODEL = MCU_MODEL_BCM2835
#configARM_PROFILE_A = 0# Not supported
#configARM_PROFILE_R = 0# Not supported
configARM_PROFILE_M = 0#
configARCH = __ARM6K__# Architecture of the core
configMP = 0# Enable MP support
configATAG = 0# Support bootloader ATAGs
configMMU = 1# MMU support
configUART = 1# UART support
configUART_MAX_PORTS = 2# Maximum number of UART ports
configFB = 1# Enable Frame Buffer support
configFB_FONT_LATIN = 0# Latin extension
configFB_FONT_GREEK = 0# Greek extension
configFB_FONT_HIRAGANA = 0# Hiragana extension
configBCM_JTAG = 0# BCM2835 JTAG support

# Kernel logging options
configKLOGGER = 1# Enable KERROR as kernel logger
configKERROR_BUF_SIZE = 2048# Lastlog buf size
configKERROR_UART = 1# Option to output kernel logs directly to UART0
configKERROR_FB = 1
# 0 = No logger
# 1 = Kerror buffer
# 2 = UART0
# 3 = FB
configDEF_KLOGGER = 3# Select default logger (options above)

# loglevel
# Additional log messages and asserts.
# These are one to one with KERROR message levels.
# 4 - All debug messages and asserts.
# 3 . Additional info messages.
# 2 - Additional warnings.
# 1 - Additional error messages and asserts for serious errors that should not
#     occur in normal operation.
# 0 - Normal build
configDEBUG = '4'#Additional debug messages and asserts.
configKERROR_MAXLEN = 400# Max kerror line length
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
configRAMFS = 0# Compile ramfs with Zeke base

# In-Kernel Unit Tests
configKUNIT = 0# Compile KUnit and tests
KU_REPORT_ORIENTED = 0#
configKUNIT_GENERIC = 0# Tests for generic data structures
configKUNIT_KSTRING = 0# Tests for kstring library
configKUNIT_FS = 0# Tests for vfs and filesystems

# User space
# TODO This should be possibly somewhere else
configTISH = 0# Debug console in init
