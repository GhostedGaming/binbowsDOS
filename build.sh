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

# Outputs (will be set/overwritten as build runs)
BOOTLOADER="${BUILD_DIR}/boot.bin"
KERNEL_ELF="${BUILD_DIR}/kernel.elf"
KERNEL_BIN="${BUILD_DIR}/kernel.bin"
OS_IMAGE="${BUILD_DIR}/os.bin"

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
NC='\033[0m' # No Color

echo -e "${BLUE}Building binbowsDOS...${NC}"
mkdir -p "${BUILD_DIR}"

# ========================
# Step 0: Choose boot asm (preferred: boot.asm)
# ========================
BOOT_ASM=""
if [ -f "${BOOT_DIR}/boot.asm" ]; then
    BOOT_ASM="${BOOT_DIR}/boot.asm"
else
    # pick first asm file in boot dir if present
    first_boot_asm=$(find "${BOOT_DIR}" -maxdepth 1 -type f -name "*.asm" 2>/dev/null | head -n 1 || true)
    if [ -n "${first_boot_asm}" ]; then
        BOOT_ASM="${first_boot_asm}"
    fi
fi

# ========================
# Step 1: Assemble all .asm files (exclude boot dir, search recursively)
# ========================
echo -e "${GREEN}[1/5] Assembling assembly files (excluding boot dir)...${NC}"

# Find asm files recursively but exclude the boot directory
ASM_FILES=$(find "${SRC_DIR}" -type d -name "boot" -prune -o -type f -name "*.asm" -print)
ASM_OBJECTS=""

for asm_file in ${ASM_FILES}; do
    base_name=$(basename "${asm_file}" .asm)
    # Use a unique name based on the relative path to avoid collisions
    rel_path=$(realpath --relative-to="${SRC_DIR}" "${asm_file}")
    safe_name=$(echo "${rel_path}" | sed 's/[\/\.]/_/g')
    out_file="${BUILD_DIR}/${safe_name}_asm.o"

    echo "  Assembling ${asm_file} -> ${out_file} ..."
    ${AS} -f elf32 "${asm_file}" -o "${out_file}"
    ASM_OBJECTS="${ASM_OBJECTS} ${out_file}"
done

# Assemble boot ASM (if any) to raw bin
if [ -n "${BOOT_ASM}" ]; then
    echo -e "${GREEN}  Assembling bootloader ${BOOT_ASM}...${NC}"
    boot_basename=$(basename "${BOOT_ASM}" .asm)
    ${AS} -f bin "${BOOT_ASM}" -o "${BUILD_DIR}/${boot_basename}.bin"
    BOOTLOADER="${BUILD_DIR}/${boot_basename}.bin"
else
    echo -e "${GREEN}  No boot asm found in ${BOOT_DIR}, using existing BOOTLOADER value if set.${NC}"
fi

# ========================
# Step 2: Compile all .c files (exclude boot dir, search recursively)
# ========================
echo -e "${GREEN}[2/5] Compiling C files (excluding boot dir)...${NC}"

O_FILES=""

# Compile main.c first if it exists (only if it's not inside boot)
if [ -f "${SRC_DIR}/main.c" ]; then
    echo "  Compiling ${SRC_DIR}/main.c..."
    ${CC} ${CFLAGS} -c "${SRC_DIR}/main.c" -o "${BUILD_DIR}/main.o"
    O_FILES="${BUILD_DIR}/main.o"
fi

# Find other C files recursively but exclude boot dir
C_FILES=$(find "${SRC_DIR}" -type d -name "boot" -prune -o -type f -name "*.c" -print | grep -v "${SRC_DIR}/main.c" || true)
for c_file in ${C_FILES}; do
    base_name=$(basename "${c_file}" .c)
    # Use a unique name based on the relative path to avoid collisions
    rel_path=$(realpath --relative-to="${SRC_DIR}" "${c_file}")
    safe_name=$(echo "${rel_path}" | sed 's/[\/\.]/_/g')
    o_file="${BUILD_DIR}/${safe_name}.o"
    
    echo "  Compiling ${c_file} -> ${o_file} ..."
    ${CC} ${CFLAGS} -c "${c_file}" -o "${o_file}"
    O_FILES="${O_FILES} ${o_file}"
done

# Add ASM object files to the list
O_FILES="${O_FILES} ${ASM_OBJECTS}"

# ========================
# Step 3: Link kernel ELF
# ========================
echo -e "${GREEN}[3/5] Linking kernel...${NC}"
${LD} ${LDFLAGS} ${O_FILES} -o "${KERNEL_ELF}"

# ========================
# Step 4: Extract kernel binary
# ========================
echo -e "${GREEN}[4/5] Extracting kernel binary...${NC}"
${OBJCOPY} -O binary "${KERNEL_ELF}" "${KERNEL_BIN}"

# ========================
# Step 5: Combine bootloader + kernel
# ========================
echo -e "${GREEN}[5/5] Creating OS image...${NC}"

if [ -f "${BOOTLOADER}" ]; then
    cat "${BOOTLOADER}" "${KERNEL_BIN}" > "${OS_IMAGE}"
else
    echo "Warning: Bootloader binary not found (${BOOTLOADER}). Creating OS image with kernel only."
    cp "${KERNEL_BIN}" "${OS_IMAGE}"
fi

# ========================
# Done!
# ========================
echo -e "${BLUE}Build complete!${NC}"
if [ -f "${BOOTLOADER}" ]; then
    echo "  Bootloader: ${BOOTLOADER} ($(stat -c%s ${BOOTLOADER}) bytes)"
else
    echo "  Bootloader: (none)"
fi
echo "  Kernel ELF: ${KERNEL_ELF} ($(stat -c%s ${KERNEL_ELF}) bytes)"
echo "  Kernel BIN: ${KERNEL_BIN} ($(stat -c%s ${KERNEL_BIN}) bytes)"
echo "  OS Image:   ${OS_IMAGE} ($(stat -c%s ${OS_IMAGE}) bytes)"
echo ""
echo "Running qemu-system-i386 -drive format=raw,file=${OS_IMAGE}"
qemu-system-i386 -drive format=raw,file=${OS_IMAGE}