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

# User configurable part begins ################################################

# Kernel configuration
configMCU_MODEL = MCU_MODEL_BCM2835
#configARM_PROFILE_A = 0# Not supported
#configARM_PROFILE_R = 0# Not supported
configARM_PROFILE_M = 0#
configARCH = __ARM6K__# Architecture of the core
