#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

# Fail if could not create directory.
if [ $? -ne 0 ]
then
	echo "Could not create supplied output directory."
	return 1
fi

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
		# Clean all old configs.
		make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
		# Defconfig to setup VIRT target for QEMU
		make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig 
		# Build target
		make -j8 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
		# Build modules
		make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
		# Build device tree
		make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
fi

echo "Adding the Image in outdir"

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
cd "${OUTDIR}/rootfs"
mkdir bin dev etc home lib proc sys sbin tmp usr usr/bin usr/lib usr/sbin root var
# Change owner to root
sudo chown -R root:root *

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    make distclean
    make defconfig
else
    cd busybox
fi

# TODO: Make and install busybox
sudo make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install CONFIG_PREFIX=${OUTDIR}/rootfs/


echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs

# TODO: Make device nodes
# Make /dev/null entry
sudo mknod -m 666 dev/null c 1 3
# Make console entry
sudo mknod -m 666 dev/console c 5 1
# TODO: Clean and build the writer utility
cd ${FINDER_APP_DIR}
make clean
make CROSS_COMPILE=${CROSS_COMPILE}

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp ${FINDER_APP_DIR}/writer ${OUTDIR}/rootfs/home/
cp ${FINDER_APP_DIR}/finder-test.sh ${OUTDIR}/rootfs/home/
cp ${FINDER_APP_DIR}/finder.sh ${OUTDIR}/rootfs/home/
cp -R ${FINDER_APP_DIR}/../conf ${OUTDIR}/rootfs/home/ 

# TODO: Chown the root directory
sudo chown -R root:root ${OUTDIR}/rootfs/root
# TODO: Create initramfs.cpio.gz

# Create .cpio file

