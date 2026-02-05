#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
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

# Change to the output directory where all build artifacts will be stored
cd "$OUTDIR"

# Clone the Linux stable kernel repository only if it does not already exist
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    # Inform the user that the kernel repository is being cloned
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
    # Clone the specified kernel version with minimal history for efficiency
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi

# Check if the kernel Image has already been built
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    # Enter the Linux kernel source directory
    cd linux-stable

    # Ensure the correct kernel version is checked out
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # Configure the kernel with default settings for the target architecture
	make -j$(nproc) ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig

    # Build the kernel Image and device tree blobs
	make -j$(nproc) ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} Image dtbs
fi

# Notify that the kernel Image is being copied to the output directory
echo "Adding the Image in outdir"
# Copy the built kernel Image to the output directory
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/Image

# Indicate creation of the root filesystem staging directory
echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"

# Remove any existing root filesystem to ensure a clean build
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# Create the root filesystem directory
mkdir -p ${OUTDIR}/rootfs
cd ${OUTDIR}/rootfs

# Create the standard Linux directory structure for the root filesystem
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr/bin usr/sbin var

# Return to the output directory
cd "$OUTDIR"

# Clone BusyBox only if it does not already exist
if [ ! -d "${OUTDIR}/busybox" ]
then
    # Clone the BusyBox repository
    git clone git://busybox.net/busybox.git
    # Enter the BusyBox directory
    cd busybox
    # Checkout the specified BusyBox version
    git checkout ${BUSYBOX_VERSION}
else
    # Enter the existing BusyBox directory
    cd busybox
fi

# Clean any previous BusyBox build artifacts
make distclean
# Configure BusyBox with default settings
make defconfig
# Build BusyBox for the target architecture
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
# Install BusyBox into the root filesystem
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} CONFIG_PREFIX=${OUTDIR}/rootfs install

# Display shared library and interpreter dependencies of BusyBox
echo "Library dependencies"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "Shared library"

# Determine the sysroot used by the cross-compiler
SYSROOT=$(${CROSS_COMPILE}gcc --print-sysroot)

# Copy the dynamic loader and required libraries into the root filesystem
cp ${SYSROOT}/lib/ld-linux-aarch64.so.1 ${OUTDIR}/rootfs/lib/
cp ${SYSROOT}/lib/ld-linux-aarch64.so.1 ${OUTDIR}/rootfs/lib64/
cp -rf ${SYSROOT}/lib64/lib* ${OUTDIR}/rootfs/lib64/

# Create required device nodes for the root filesystem
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/null c 1 3
sudo mknod -m 600 ${OUTDIR}/rootfs/dev/console c 5 1

# Change to the Finder application directory
cd ${FINDER_APP_DIR}
# Clean previous builds and build the Finder application
make clean build CROSS_COMPILE=${CROSS_COMPILE}

# Create the home directory in the root filesystem
mkdir -p ${OUTDIR}/rootfs/home/

# Copy application scripts and binaries into the root filesystem
cp ${FINDER_APP_DIR}/autorun-qemu.sh ${OUTDIR}/rootfs/home/
cp writer ${OUTDIR}/rootfs/home/
cp finder.sh ${OUTDIR}/rootfs/home/
# Update the shebang to use /bin/sh instead of /bin/bash
sed -i '1s|^#!/bin/bash|#!/bin/sh|' ${OUTDIR}/rootfs/home/finder.sh
cp finder-test.sh ${OUTDIR}/rootfs/home/
# Fix the configuration file path in the test script
sed -i 's|\.\./conf/assignment.txt|conf/assignment.txt|g' ${OUTDIR}/rootfs/home/finder-test.sh

# Create and populate the configuration directory
mkdir -p ${OUTDIR}/rootfs/home/conf
cp -r ${FINDER_APP_DIR}/conf/* ${OUTDIR}/rootfs/home/conf/

# Ensure all files in the root filesystem are owned by root
sudo chown -R root:root ${OUTDIR}/rootfs

# Package the root filesystem into an initramfs archive
cd ${OUTDIR}/rootfs
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
# Compress the initramfs archive
gzip -f ${OUTDIR}/initramfs.cpio

