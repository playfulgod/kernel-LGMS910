#!/bin/sh
#Let's make sure the environment is clean and ready to compile the kernel
echo "Cleaning house!!"
make mrproper
echo "House cleaned, lets build a kernel!!!"
#
# Lets set the kernel defconfig
echo "defconfig = plague_defconfig"
make ARCH=arm plague_defconfig
#
# Let's build a kernel
echo "Now compiling kernel, go get a soda! ;)"
ARCH=arm CROSS_COMPILE=~/Android/CMR/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi- make zImage -j4
#
if [ -f arch/arm/boot/zImage ]; then
      echo "Plague has been compiled!!! You can find it in arch/arm/boot/zImage"
else
      echo "Kernel did not compile, please check for errors!!"
fi


#arg=$1

#target=`find arch/arm/boot -name zImage`

#case "target" in
#      "arch/arm/boot/zImage")
#   echo "Plague has been compiled! You can find it in: arch/arm/boot/zImage"
#	;;

#esac

#exit 0
