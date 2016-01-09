ifeq ($(configBOOTLOADER_NONE),y)
	MKROOTFS_BOOTFILES := boot/none.files
endif
ifeq ($(configBOOTLOADER_RPI),y)
	MKROOTFS_BOOTFILES := boot/rpi.files
endif
ifeq ($(configBOOTLOADER_RPI0),y)
	MKROOTFS_BOOTFILES := boot/rpi0.files
endif
