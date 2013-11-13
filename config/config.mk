# Kernel config

# Supported MCU models
MCU_MODEL_STM32F0 = 1
MCU_MODEL_STR912F = 2
MCU_MODEL_BCM2835 = 3

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
SCHED_LAVGPERIOD_5SEC  = 5
SCHED_LAVGPERIOD_11SEC = 11

# User configurable part begins ################################################

# Kernel configuration
configMCU_MODEL = MCU_MODEL_BCM2835
#configARM_PROFILE_A = 0# Not supported
#configARM_PROFILE_R = 0# Not supported
configARM_PROFILE_M = 0#
configARCH = __ARM6K__# Architecture of the core
configMMU = 1# MMU support
configUART = 1#
configATAG = 1# Support bootloader ATAGs

# Kernel logging methods (Select one or none)
configKERROR_LAST = 0# Store last n log messages in buffer
configKERROR_TTYS = 1#

# Scheduler Selection
configSCHED_TINY = 1#
#
# SCHED_TINY Config
configSCHED_MAX_THREADS = 10# Maximum number of threads
configFAST_FORK = 0# Use a queue to find the next free threadId.
configSCHED_FREQ = 100u# Scheduler frequency in Hz
configSCHED_LAVG_PER = SCHED_LAVGPERIOD_11SEC#

# Enable or disable heap bounds check
configHEAP_BOUNDS_CHECK = 0#

# Maximum number of timers available
configTIMERS_MAX = 4#

# String functions
configSTRING_OPT_SIZE = 1# Optimize string functions for speed or size

# usrinit
configUSRINIT_SSIZE = 200u# Stack size for usrinit thread
configUSRINIT_PRI = osPriorityNormal# Priority of usrinit thread

# Virtual file system config
configFS_MAX = 5# Maximum number of registered file systems

# Kernel Services
configPTTK91_VM = 0#

# Dev driver modules
# These can be also commented out
configDEV_NULL = 1#
configDEV_LCD = 0#
configDEV_TTYS = 1#
