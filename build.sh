#!/bin/bash

set -e  # Exit on any error

# ========================
# Configuration
# ========================
CC="gcc"
LD="ld"
AS="nasm"
OBJCOPY="objcopy"

SRC_DIR="src"
BUILD_DIR="build"
BOOT_DIR="${SRC_DIR}/boot"
LINKER_SCRIPT="linker-scripts/x86_64.ld"

# Outputs
BOOTLOADER="${BUILD_DIR}/boot.bin"
KERNEL_ELF="${BUILD_DIR}/kernel.elf"
KERNEL_BIN="${BUILD_DIR}/kernel.bin"
OS_IMAGE="${BUILD_DIR}/os.bin"
IDE_DRIVE="${BUILD_DIR}/ide_drive.img"

CFLAGS="
-ffreestanding -nostdlib -Wall -Wextra -pedantic -O2 -m32
-fno-pic -fno-pie -fno-stack-protector
-I ./includes
"

LDFLAGS="-T ${LINKER_SCRIPT} -m elf_i386"

# ========================
# Colors
# ========================
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${BLUE}Building binbowsDOS...${NC}"
mkdir -p "${BUILD_DIR}"

# ========================
# Step 0: Find bootloader
# ========================
BOOT_ASM=""
if [ -f "${BOOT_DIR}/boot.asm" ]; then
    BOOT_ASM="${BOOT_DIR}/boot.asm"
else
    first_boot_asm=$(find "${BOOT_DIR}" -maxdepth 1 -type f -name "*.asm" 2>/dev/null | head -n 1 || true)
    if [ -n "${first_boot_asm}" ]; then
        BOOT_ASM="${first_boot_asm}"
    fi
fi

# ========================
# Step 1: Assemble bootloader
# ========================
if [ -n "${BOOT_ASM}" ]; then
    echo -e "${GREEN}[1/6] Assembling bootloader ${BOOT_ASM}...${NC}"
    boot_basename=$(basename "${BOOT_ASM}" .asm)
    ${AS} -f bin "${BOOT_ASM}" -o "${BUILD_DIR}/${boot_basename}.bin"
    BOOTLOADER="${BUILD_DIR}/${boot_basename}.bin"
    
    # Verify bootloader is exactly 512 bytes
    BOOT_SIZE=$(stat -c%s "${BOOTLOADER}")
    if [ "${BOOT_SIZE}" -ne 512 ]; then
        echo -e "${RED}Error: Bootloader must be exactly 512 bytes, got ${BOOT_SIZE}${NC}"
        exit 1
    fi
else
    echo -e "${YELLOW}Warning: No bootloader found in ${BOOT_DIR}${NC}"
fi

# ========================
# Step 2: Assemble kernel .asm files
# ========================
echo -e "${GREEN}[2/6] Assembling kernel assembly files...${NC}"

ASM_FILES=$(find "${SRC_DIR}" -type d -name "boot" -prune -o -type f -name "*.asm" -print)
ASM_OBJECTS=""

for asm_file in ${ASM_FILES}; do
    rel_path=$(realpath --relative-to="${SRC_DIR}" "${asm_file}")
    safe_name=$(echo "${rel_path}" | sed 's/[\/\.]/_/g')
    out_file="${BUILD_DIR}/${safe_name}_asm.o"

    echo "  ${asm_file} -> ${out_file}"
    ${AS} -f elf32 "${asm_file}" -o "${out_file}"
    ASM_OBJECTS="${ASM_OBJECTS} ${out_file}"
done

if [ -z "${ASM_OBJECTS}" ]; then
    echo "  No assembly files found"
fi

# ========================
# Step 3: Compile C files
# ========================
echo -e "${GREEN}[3/6] Compiling C files...${NC}"

O_FILES=""

# Compile main.c first if it exists
if [ -f "${SRC_DIR}/main.c" ]; then
    echo "  ${SRC_DIR}/main.c -> ${BUILD_DIR}/main.o"
    ${CC} ${CFLAGS} -c "${SRC_DIR}/main.c" -o "${BUILD_DIR}/main.o"
    O_FILES="${BUILD_DIR}/main.o"
fi

# Find and compile other C files
C_FILES=$(find "${SRC_DIR}" -type d -name "boot" -prune -o -type f -name "*.c" -print | grep -v "${SRC_DIR}/main.c" || true)
for c_file in ${C_FILES}; do
    rel_path=$(realpath --relative-to="${SRC_DIR}" "${c_file}")
    safe_name=$(echo "${rel_path}" | sed 's/[\/\.]/_/g')
    o_file="${BUILD_DIR}/${safe_name}.o"
    
    echo "  ${c_file} -> ${o_file}"
    ${CC} ${CFLAGS} -c "${c_file}" -o "${o_file}"
    O_FILES="${O_FILES} ${o_file}"
done

# Add ASM objects
O_FILES="${O_FILES} ${ASM_OBJECTS}"

if [ -z "${O_FILES}" ]; then
    echo -e "${RED}Error: No source files found to compile!${NC}"
    exit 1
fi

# ========================
# Step 4: Link kernel ELF
# ========================
echo -e "${GREEN}[4/6] Linking kernel ELF...${NC}"
${LD} ${LDFLAGS} ${O_FILES} -o "${KERNEL_ELF}"

# ========================
# Step 5: Extract kernel binary
# ========================
echo -e "${GREEN}[5/6] Extracting kernel binary...${NC}"
${OBJCOPY} -O binary "${KERNEL_ELF}" "${KERNEL_BIN}"

# ========================
# Step 6: Create OS image
# ========================
echo -e "${GREEN}[6/6] Creating OS image...${NC}"

if [ -f "${BOOTLOADER}" ]; then
    cat "${BOOTLOADER}" "${KERNEL_BIN}" > "${OS_IMAGE}"
    
    # Pad to minimum disk size (useful for testing)
    # OS_SIZE=$(stat -c%s "${OS_IMAGE}")
    # if [ ${OS_SIZE} -lt 1474560 ]; then  # 1.44MB floppy size
    #     truncate -s 1474560 "${OS_IMAGE}"
    # fi
else
    echo -e "${YELLOW}Warning: No bootloader, creating kernel-only image${NC}"
    cp "${KERNEL_BIN}" "${OS_IMAGE}"
fi

# ========================
# Create IDE Drive
# ========================
echo -e "${GREEN}Creating IDE drive...${NC}"
if [ ! -f "${IDE_DRIVE}" ]; then
    # Create a 100MB IDE drive image
    dd if=/dev/zero of="${IDE_DRIVE}" bs=1M count=100 status=none
    echo "  Created new IDE drive: ${IDE_DRIVE} (100MB)"
else
    echo "  Using existing IDE drive: ${IDE_DRIVE}"
fi

# ========================
# Build Summary
# ========================
echo -e "${BLUE}════════════════════════════════════${NC}"
echo -e "${BLUE}Build complete!${NC}"
echo -e "${BLUE}════════════════════════════════════${NC}"

if [ -f "${BOOTLOADER}" ]; then
    echo -e "  Bootloader: ${BOOTLOADER} (${GREEN}$(stat -c%s ${BOOTLOADER}) bytes${NC})"
else
    echo -e "  Bootloader: ${YELLOW}(none)${NC}"
fi

echo "  Kernel ELF: ${KERNEL_ELF} ($(stat -c%s ${KERNEL_ELF}) bytes)"
echo "  Kernel BIN: ${KERNEL_BIN} ($(stat -c%s ${KERNEL_BIN}) bytes)"
echo "  OS Image:   ${OS_IMAGE} ($(stat -c%s ${OS_IMAGE}) bytes)"
echo "  IDE Drive:  ${IDE_DRIVE} ($(stat -c%s ${IDE_DRIVE}) bytes)"
echo ""

# ========================
# Run in QEMU
# ========================
echo -e "${BLUE}Launching QEMU...${NC}"
qemu-system-i386 \
  -drive format=raw,file=${OS_IMAGE} \
  -drive format=raw,file=${IDE_DRIVE},if=ide \
  -m 1G -display curses -monitor none