obj-$(CONFIG_LTE) += lte.o
#obj-$(CONFIG_LGE_L2K_ETH) += l2k_eth.o

lte-objs = lte_sdio_module.o \
	lte_sdio.o \
	lte_him.o \
	lte_boot.o \
	lte_tty_lldm.o
	
SRCROOT = drivers/net/lte

EXTRA_CFLAGS = \
	-DLTE_DEBUG \
	-I$(SRCROOT) \

# BEGIN : daeok.kim@lge.com 20110206
# ADD 0015440: [LTE] LTE image (singed_app.bin) partition is changed to system from persist
ifeq ($(TARGET_BUILD_VARIANT), eng)
	EXTRA_CFLAGS += -DENG
endif
ifeq ($(TARGET_BUILD_VARIANT), userdebug)
	EXTRA_CFLAGS += -DUSERDEBUG
endif
ifeq ($(TARGET_BUILD_VARIANT), user)
	EXTRA_CFLAGS += -DUSER
endif
# END : daeok.kim@lge.com 20110206
#	-DIOCTL_RESP_TIMEOUT=1000 \
#	-DSDIO_VENDOR_ID_L2000=0x2d0 \
#	-DSDIO_DEVICE_ID_L2000=0x4c6 \
#	-DANDROID_SPECIFIC \

