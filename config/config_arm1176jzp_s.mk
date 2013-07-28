# Kernel config

# Supported MCU models
MCU_MODEL_STM32F0 = 1
MCU_MODEL_STR912F = 2
MCU_MODEL_ARM1176JZF_S = 3

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
# MCU_MODEL_ARM1176JZF_S = __ARM6__

# Supported load average calculation periods
SCHED_LAVGPERIOD_5SEC  = 5
SCHED_LAVGPERIOD_11SEC = 11

# User configurable part begins ################################################

# Kernel configuration
configMCU_MODEL = MCU_MODEL_ARM1176JZF_S
#configARM_PROFILE_A = 0#Not supported
#configARM_PROFILE_R = 0#Not supported
configARM_PROFILE_M = 0
# Architecture of the core
configARCH = __ARM6K__

# Scheduler Selection
configSCHED_TINY = 1
#
# Tiny Scheduler
# Maximum number of threads
configSCHED_MAX_THREADS = 10
# Use a queue to find the next free threadId.
configFAST_FORK = 0
# Scheduler frequency in Hz
configSCHED_FREQ = 100u
configSCHED_LAVG_PER = SCHED_LAVGPERIOD_11SEC

# Enable or disable heap bounds check
configHEAP_BOUNDS_CHECK = 0

# Maximum number of timers available
configTIMERS_MAX = 4

# String functions
# Optimize string functions for speed
configSTRING_OPT_SIZE = 1

# APP Main
# Stack size for app_main thread
configAPP_MAIN_SSIZE = 200u
# Priority of app_main thread
configAPP_MAIN_PRI = osPriorityNormal

# Kernel Services
# Enable device driver subsystem
configDEVSUBSYS = 1
configPTTK91_VM = 0
