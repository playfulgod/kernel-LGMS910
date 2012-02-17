#!/bin/bash
# Scripted by Dewayne Jones @PlayfulGod
#Let's make sure the environment is clean and ready to compile the kernel

ver="v.35"
dev="PlayfulGod"
mdir="~/Android/Kernels/Modules/modules-esteem"
defconfig="plague_defconfig"
kdir="kernel-esteem"
kernel="Plague Esteem Kernel"
toolchain="~/Android/CM7.2/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi-"
title="Kernel Build Script $ver by $dev for $kernel"
menu1="Build Kernel"
menu2="Build Kernel Modules"
menu3="Install Kernel Modules"
menu4="Exit"

function menu {
clear
MENU_CHOICE=""
echo "================================================================="
echo "  $title "
echo "================================================================="
echo
echo "MENU:";
echo " 1) $menu1";
echo " 2) $menu2";
echo " 3) $menu3";
echo " 4) $menu4";
echo
read -n1 -s -p "Please Type a Number [1-4]: " MENU_CHOICE
echo
echo
case $MENU_CHOICE in
"1") header "Building kernel";buildkernel;;
"2") header "Build Modules";buildmodules;;
"3") header "Install Modules to $mdir";installmodules;;
"4") header "Exit";bye;;
*) menu;;
esac
}


function buildkernel {
header "Building kernel"
echo "Cleaning house!!"
echo
make distclean
echo
echo "House cleaned, lets build a kernel!!!"
echo

# Lets set the kernel defconfig
echo
echo "Setting defconfig!!!"
echo
make ARCH=arm $defconfig

# Let's build a kernel
echo
echo "Now compiling kernel, go get a soda! ;)"
echo

ARCH=arm CROSS_COMPILE=~/Android/CM7.2/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi- make zImage -j4

if [ -f arch/arm/boot/zImage ]; then
      echo
      echo "Plague has been compiled!!! You can find it in $kdir/arch/arm/boot/zImage"
      echo
else
      echo
      echo "Kernel did not compile, please check for errors!!"
      echo
fi

end
}

function buildmodules {
header "Building Modules"
ARCH=arm CROSS_COMPILE=~/Android/CM7.2/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi- make modules -j4
echo
echo "Modules are compiled and ready to be installed to $mdir"
echo

end
}

function end {
echo
echo All Done!
echo
read -n1 -s -p "Press any key to exit, or M for Main Menu :" CHOOSE
case $CHOOSE in
[mM]) echo; menu;;
esac
echo; echo
exit
}

function installmodules {
header "Installing Modules to $mdir"
make ARCH=arm CROSS_COMPILE=~/Android/CM7.2/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi- INSTALL_MOD_PATH=$mdir zImage modules modules_install -j4

echo
echo "modules are now installed to $mdir!"
echo
end
}

function main {
chooser
buildkernel
buildmodules
installmodules
}

function header {
clear
echo ===============================================================================
echo $1
echo ===============================================================================
echo $2
echo
}

function chooser {
echo
read -n1 -s -p "Press any key to continue, or M for Main Menu :" CHOOSE
case $CHOOSE in
[mM]) echo; menu;;
esac
echo
}

function bye {
header "$menu4" 
exit
}

menu