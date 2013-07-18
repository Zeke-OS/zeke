# Kernel config

# Supported MCU models
MCU_MODEL_STM32F0 = 1
MCU_MODEL_STR912F = 2

# Supported load average calculation periods
SCHED_LAVGPERIOD_5SEC  = 5
SCHED_LAVGPERIOD_11SEC = 11

# Kernel configuration
configMCU_MODEL = MCU_MODEL_STM32F0

# Scheduler
configSCHED_MAX_THREADS = 10      # Maximum number of threads
configFAST_FORK = 0               # Use a queue to find the next free threadId.
configSCHED_FREQ = 100u           # Scheduler frequency in Hz
configSCHED_LAVG_PER = SCHED_LAVGPERIOD_11SEC
configHEAP_BOUNDS_CHECK = 0       # Enable or disable heap bounds check
configTIMERS_MAX = 4              # Maximum number of timers available

# APP Main
configAPP_MAIN_SSIZE = 200u    # Stack size for app_main thread
configAPP_MAIN_PRI = osPriorityNormal # Priority of app_main thread

# Kernel Services
configDEVSUBSYS = 1       # Enable device driver subsystem
configPTTK91_VM = 0
