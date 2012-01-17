# Edit the path to oyour CM repo and the path you want your modules installed to.
#
# Let's install our modules to our modules directory
make ARCH=arm CROSS_COMPILE=~/Android/CMR/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi- INSTALL_MOD_PATH=~/Android/Kernels/MS910-CM9-modules modules modules_install -j4
#
echo "modules are now installed to your modules directory!"
