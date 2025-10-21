#!/bin/bash

set -e  # Exit on any error

# Configuration
CC="gcc"
LD="ld"
AS="nasm"
OBJCOPY="objcopy"

# Directories
SRC_DIR="src"
BUILD_DIR="build"
BOOT_DIR="${SRC_DIR}/boot"
LINKER_SCRIPT="linker-scripts/x86_64.ld"

# Output files
BOOTLOADER="${BUILD_DIR}/boot.bin"
KERNEL_ELF="${BUILD_DIR}/kernel.elf"
KERNEL_BIN="${BUILD_DIR}/kernel.bin"
OS_IMAGE="${BUILD_DIR}/os.bin"

# Compiler flags
CFLAGS="
-ffreestanding -nostdlib -Wall -Wextra -pedantic -O2 -m32
-fno-pic -fno-pie -fno-stack-protector
-I ./includes
"

LDFLAGS="-T ${LINKER_SCRIPT} -m elf_i386"

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}Building binbowsDOS...${NC}"

# Create build directory
mkdir -p "${BUILD_DIR}"

# Assemble bootloader
echo -e "${GREEN}[1/5] Assembling bootloader...${NC}"
${AS} -f bin "${BOOT_DIR}/boot.asm" -o "${BOOTLOADER}"

# Find and compile all C files
echo -e "${GREEN}[2/5] Compiling C files...${NC}"

# Compile main.c FIRST
O_FILES=""
if [ -f "${SRC_DIR}/main.c" ]; then
    echo "  Compiling ${SRC_DIR}/main.c..."
    ${CC} ${CFLAGS} -c "${SRC_DIR}/main.c" -o "${BUILD_DIR}/main.o"
    O_FILES="${BUILD_DIR}/main.o"
fi

# Then compile everything else (excluding main.c)
C_FILES=$(find "${SRC_DIR}" -name "*.c" ! -name "main.c")

for c_file in ${C_FILES}; do
    # Get filename without path and extension
    basename=$(basename "${c_file}" .c)
    o_file="${BUILD_DIR}/${basename}.o"
    
    echo "  Compiling ${c_file}..."
    ${CC} ${CFLAGS} -c "${c_file}" -o "${o_file}"
    
    O_FILES="${O_FILES} ${o_file}"
done

# Link kernel
echo -e "${GREEN}[3/5] Linking kernel...${NC}"
${LD} ${LDFLAGS} ${O_FILES} -o "${KERNEL_ELF}"

# Extract binary from ELF
echo -e "${GREEN}[4/5] Extracting kernel binary...${NC}"
${OBJCOPY} -O binary "${KERNEL_ELF}" "${KERNEL_BIN}"

# Combine bootloader and kernel
echo -e "${GREEN}[5/5] Creating OS image...${NC}"
cat "${BOOTLOADER}" "${KERNEL_BIN}" > "${OS_IMAGE}"

# Display success message and file info
echo -e "${BLUE}Build complete!${NC}"
echo "  Bootloader: ${BOOTLOADER} ($(stat -c%s ${BOOTLOADER}) bytes)"
echo "  Kernel ELF: ${KERNEL_ELF} ($(stat -c%s ${KERNEL_ELF}) bytes)"
echo "  Kernel BIN: ${KERNEL_BIN} ($(stat -c%s ${KERNEL_BIN}) bytes)"
echo "  OS Image:   ${OS_IMAGE} ($(stat -c%s ${OS_IMAGE}) bytes)"
echo ""
echo "Running qemu-system-i386 -drive format=raw,file=${OS_IMAGE}"
qemu-system-i386 -drive format=raw,file=${OS_IMAGE}