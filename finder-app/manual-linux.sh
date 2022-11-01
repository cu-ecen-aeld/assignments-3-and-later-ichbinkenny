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

echo "Path being used is: $PATH"

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
		make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
		# Build modules
		# make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
		# Build device tree
		make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
mkdir ${OUTDIR}/rootfs
cd ${OUTDIR}/rootfs
mkdir -p bin dev etc home lib lib64 proc sys sbin tmp usr usr/bin usr/lib usr/sbin var var/log
# Change owner to root
sudo chown -R root:root *

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    make ARCH=${ARCH} distclean
    make ARCH=${ARCH} defconfig
    make ARCH=${ARCH}
else
    cd busybox
fi

# TODO: Make and install busybox
echo "Trying busybox again..."
sudo env "PATH=$PATH" make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} CONFIG_PREFIX=${OUTDIR}/rootfs/ install



# TODO: Add library dependencies to rootfs
SYSROOT=$(${CROSS_COMPILE}gcc --print-sysroot)
cd ${OUTDIR}/rootfs

echo "Library dependencies"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "Shared library"

# Seems like -L is needed for this to run...
sudo cp -L ${SYSROOT}/lib/ld-linux-aarch64.* ${OUTDIR}/rootfs/lib
sudo cp -L ${SYSROOT}/lib64/libm.so.* ${OUTDIR}/rootfs/lib64
sudo cp -L ${SYSROOT}/lib64/libresolv.so.* ${OUTDIR}/rootfs/lib64
sudo cp -L ${SYSROOT}/lib64/libc.so.* ${OUTDIR}/rootfs/lib64

# TODO: Make device nodes
# Make /dev/null entry
sudo mknod -m 666 dev/null c 1 3
# Make console entry
sudo mknod -m 600 dev/console c 5 1
# TODO: Clean and build the writer utility
cd ${FINDER_APP_DIR}/
make clean
make CROSS_COMPILE=${CROSS_COMPILE}

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
sudo cp ${FINDER_APP_DIR}/writer ${OUTDIR}/rootfs/home/
sudo cp ${FINDER_APP_DIR}/finder-test.sh ${OUTDIR}/rootfs/home/
sudo cp ${FINDER_APP_DIR}/finder.sh ${OUTDIR}/rootfs/home/
sudo cp -r ${FINDER_APP_DIR}/../conf ${OUTDIR}/rootfs/home/ 
sudo cp ${FINDER_APP_DIR}/autorun-qemu.sh ${OUTDIR}/rootfs/home/

# Having permission issues, try marking as exec.

# TODO: Chown the root directory
cd ${OUTDIR}/rootfs
sudo chown -R root:root *
# Create .cpio file
echo "Creating CPIO file..."
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
# GZip file
# TODO: Create initramfs.cpio.gz
cd ${OUTDIR}
echo "Zipping CPIO file..."
rm -f initramfs.cpio.gz
gzip initramfs.cpio

