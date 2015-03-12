include $(ROOT_DIR)/genconfig/buildconf.mk
include $(ROOT_DIR)/genconfig/dist.mk

# Add here one or more include directories.
IDIR = $(ROOT_DIR)/include

# Set this to 1 if you will use asserts that compares floats or doubles;
# or 0 disable
PU_LMATH = 0

# Enable Unix Unit (incl. stdin_writer)
PU_UU = 0

# Report oriented mode prints more additional information like test module and
# test case descriptions. 0 = disabled; 1 = enabled.
PU_REPORT_ORIENTED = 1

# Location of punit makefile
# Do not touch
include ../punit/Makefile.punit
