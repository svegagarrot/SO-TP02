
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

# PVS-Studio static analysis targets
pvs-trace:
	pvs-studio-analyzer trace -- make -j

pvs-analyze:
	pvs-studio-analyzer analyze -o pvs.log

pvs-report:
	plog-converter -a GA:1,2 -t fullhtml -o pvs-report pvs.log

pvs:
	./scripts/pvs.sh

pvs-clean:
	rm -f pvs.log strace_out
	rm -rf pvs-report

.PHONY: bootloader image collections kernel userland all clean
.PHONY: pvs-trace pvs-analyze pvs-report pvs pvs-clean
