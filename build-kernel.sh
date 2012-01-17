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
ARCH=arm CROSS_COMPILE=~/Android/CM9/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi- make zImage -j4
#
echo "Plague has been compiled! You can find it in: arch/arm/boot/zImage"
