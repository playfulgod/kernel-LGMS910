#CONFIG_MACH_LGE_BRYCE	sungmin.shin	10.06.22
#create new file.

EXTRA_CFLAGS += -Iarch/arm/mach-msm
# sangwoo.kang 2010-09-01 E
# END: 0009214 sehyuny.kim@lge.com 2010-09-03
#obj-$(CONFIG_MACH_LGE_BRYCE) += board-bryce.o
#obj-y += board-bryce.o

#CONFIG_MACH_LGE_BRYCE      ey.cho  2010.06.25 - START
#for touch keypad in EVB2
obj-$(CONFIG_MACH_LGE_BRYCE) += board-bryce-keypad.o

# BEGIN: 0009214 sehyuny.kim@lge.com 2010-09-03
# MOD 0009214: [DIAG] LG Diag feature added in side of android
# sangwoo.kang 2010-09-01 S
#obj-$(CONFIG_LGE_HEADSET_2GPIO) += lge_gpio_h2w.o
#obj-$(CONFIG_LGE_SWITCH_GPIO) += lge_gpio_switch.o
#obj-$(CONFIG_LGE_PROC_COMM) += lge_proc_comm.o
# sangwoo.kang 2010.09.03 debugging for build error
obj-$(CONFIG_LGE_SUPPORT_RAPI) += LG_rapi_client.o 
#obj-$(CONFIG_LGE_MTD_DIRECT_ACCESS) += lge_mtd_direct_access.o
obj-$(CONFIG_LGE_DIAGTEST) += lg_diag_testmode.o
# // BEGIN 0011665: eundeok.bae@lge.com FTM MODE FOR ONLY KERNEL BOOTING
# // [KERNEL] Added source code For Sleep Mode Test, Test Mode V8.3 [250-42] 
obj-$(CONFIG_LGE_DIAGTEST) += lg_diag_testmode_sysfs.o
# // END 0011665: eundeok.bae@lge.com FTM MODE FOR ONLY KERNEL BOOTING
obj-$(CONFIG_LGE_DIAGTEST) += lg_fw_diag_class.o
obj-$(CONFIG_LGE_DIAGTEST) += lg_fw_diag_communication.o
# add lg_diag_event_log.c for key logging in diag testmode test 2010-07-10
obj-$(CONFIG_LGE_DIAGTEST) += lg_diag_event_log.o
# add lg_diag_keypress.c for UTS test by younchan.kim 2010-06-11
obj-$(CONFIG_LGE_DIAGTEST) += lg_diag_keypress.o
# add lg_diag_screen_shot.c for UTS test by minjong.gong 2010-06-13
obj-$(CONFIG_LGE_DIAGTEST) += lg_diag_screen_shot.o
# LGE_MERGE_S : for SMS UTS Test by sunmyoung.lee 2010-07-16
obj-$(CONFIG_LGE_DIAGTEST) += lg_diag_udm.o
# LGE_MERGE_E
obj-$(CONFIG_LGE_DIAGTEST) += lg_fw_diag_mtc.o
obj-$(CONFIG_LGE_DIAGTEST) += lg_diag_wifi.o
# sangwoo.kang 2010-09-01 E
# END: 0009214 sehyuny.kim@lge.com 2010-09-03
#obj-$(CONFIG_MACH_LGE_BRYCE) += board-bryce.o

#CONFIG_MACH_LGE_BRYCE      ey.cho  2010.06.25 - START
#for touch keypad in EVB2
#obj-$(CONFIG_MACH_LGE_BRYCE) += board-bryce-keypad.o

#CONFIG_MACH_LGE_BRYCE	sungwoo.cho	10.04.20
#For Vibrator Setting
obj-$(CONFIG_MACH_LGE_BRYCE) += board-bryce-misc.o

#CONFIG_MACH_LGE_BRYCE	ehgrace.kim	10.04.21
#For Audio

#MS910_LGE_CHANGE_AUDIO [jy0127.jang@lge.com] 2011-01-14 [START]
obj-$(CONFIG_MACH_LGE_BRYCE) += board-bryce-headset.o
#obj-$(CONFIG_MACH_LGE_BRYCE) += lge_gpio_h2w.o
#obj-$(CONFIG_MACH_MSM7X30_SURF) += lge_qpio_h2w.o
#MS910_LGE_CHANGE_AUDIO [jy0127.jang@lge.com] 2011-01-14 [END]

#CONFIG_MACH_LGE_BRYCE	chanhee.park	10.04.21
#For Camera
#jjy0127_temp obj-$(CONFIG_MACH_LGE_BRYCE) += board-bryce-camera.o

#CONFIG_MACH_LGE_BRYCE chanha.park@lge.com    10.08.26
#For Bluetooth
obj-$(CONFIG_MACH_LGE_BRYCE) += board-bryce-bt.o

#obj-$(CONFIG_MACH_LGE_BRYCE) += board-bryce-headset.o

#CONFIG_MACH_LGE_BRYCE	chanhee.park	10.04.21
#For Camera
obj-$(CONFIG_MACH_LGE_BRYCE) += board-bryce-camera.o

#CONFIG_MACH_LGE_BRYCE chanha.park@lge.com    10.08.26
#For Bluetooth
#obj-$(CONFIG_MACH_LGE_BRYCE) += board-bryce-bt.o

# BEGIN: 0009214 sehyuny.kim@lge.com 2010-09-03
# MOD 0009214: [DIAG] LG Diag feature added in side of android
# sangwoo.kang 2010-09-03 LG DIAG ATS MERGE
#obj-$(CONFIG_MACH_LGE) += devices_lge.o


obj-$(CONFIG_LGE_EMMC_SUPPORT) += lge_emmc_direct_access.o


obj-$(CONFIG_LGE_SUPPORT_AT_CMD) += lge_ats.o
obj-$(CONFIG_LGE_SUPPORT_AT_CMD) += lge_ats_cmd.o
obj-$(CONFIG_LGE_SUPPORT_AT_CMD) += lge_ats_eta.o
obj-$(CONFIG_LGE_SUPPORT_AT_CMD) += lge_ats_mmc.o
obj-$(CONFIG_LGE_SUPPORT_AT_CMD) += lge_ats_event_log.o
obj-$(CONFIG_LGE_ATS_UEVENT_DEVICE) += lge_ats_uevent.o
obj-$(CONFIG_LGE_ATS_INPUT_DEVICE) += lge_ats_input.o # ATS_INPUT_DEVICE
obj-$(CONFIG_LGE_DIAG_WMC) += lg_diag_wmc.o
# END: 0009214 sehyuny.kim@lge.com 2010-09-03

#Baikal ID 0009963 CONFIG_MACH_LGE_BRYCE	kiran.kanneganti@lge.com 13-oct-2010
#For Audio
obj-$(CONFIG_MACH_LGE_BRYCE) += lge_audio_misc_ctrl.o

# kwangdo.yi for ERS handler 10.10.26 S
# 0010357: add ram dump for ERS in AP side
# kwangdo.yi@lge.com 10.11.29 S
# 0011521: [kernel] ERS enable/disable feature added

#obj-$(CONFIG_ANDROID_RAM_CONSOLE) += lge_errorhandler.o

# kwangdo.yi@lge.com  10.11.04
# 0010459: file name change from board-swift-ers.c to lge-bryce-ers.c
# kwangdo.yi@lge.com 10.11.29 S
# 0011521: [kernel] ERS enable/disable feature added
#obj-$(CONFIG_ANDROID_RAM_CONSOLE) += lge-bryce-ers.o

# kwangdo.yi E

# // BEGIN 0011665: eundeok.bae@lge.com FTM MODE FOR ONLY KERNEL BOOTING
# // [KERNEL] Added source code For Sleep Mode Test, Test Mode V8.3 [250-42]
# // END 0011665: eundeok.bae@lge.com FTM MODE FOR ONLY KERNEL BOOTING
obj-y += lg_ftmmode.o
# // END 0011665: eundeok.bae@lge.com FTM MODE FOR ONLY KERNEL BOOTING
# 0014609 : neo.kang@lge.com to save the autoall test result
#obj-y += lge_bryce_temp.o

obj-$(CONFIG_LGE_ERS) += lge_handle_panic.o
obj-$(CONFIG_LGE_ERS) += lge_ers.o

