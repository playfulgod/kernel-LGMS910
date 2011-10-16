#ARCH=arm CROSS_COMPILE=~/android/src/c8600/prebuilt/linux-x86/toolchain/arm-eabi-4.4.0/bin/arm-eabi- make V=1 zImage -j2
export TARGET_PRODUCT=lge_bryce
echo "target set to lge_bryce"
export BUILD_LG_HW_MS910_REV=5
export TARGET_BUILD_VARIANT=user
export TARGET_LTE_IMAGE=persist
make ARCH=arm plague_defconfig
ARCH=arm CROSS_COMPILE=~/Android/CMR/prebuilt/linux-x86/toolchain/arm-eabi-4.4.0/bin/arm-eabi- make zImage -j4
