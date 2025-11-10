
all:  bootloader kernel userland image

bootloader:
	cd Bootloader; make all

kernel:
	cd Kernel; make all

userland:
	cd Userland; make all

image: kernel bootloader userland
	cd Image; make all

clean:
	cd Bootloader; make clean
	cd Image; make clean
	cd Kernel; make clean
	cd Userland; make clean

format:
	@command -v clang-format >/dev/null 2>&1 || { echo "Error: clang-format no está instalado. Por favor instálalo primero."; exit 1; }
	find Kernel Userland Bootloader Toolchain -type f \( -name "*.c" -o -name "*.h" \) -exec clang-format -style=file --sort-includes --Werror -i {} +

.PHONY: bootloader image collections kernel userland all clean format
