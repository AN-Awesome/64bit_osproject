all: clean _kernel_ _tools_ Disk.img test

_kernel_:
	make -C kernel

_tools_:
	make -C tools

Disk.img: build/boot.bin build/kernel32.bin build/kernel64.bin
	./tools/ImageMaker/ImageMaker $^
	sudo chmod 755 Disk.img

test:
	sudo qemu-system-x86_64 -m 64 -fda ./Disk.img -rtc base=localtime -M pc

clean:
	clear

	make -C tools clean

	rm -f build/*.*
	rm -f Disk.img
	rm -f ImageMaker
	rm -f buildcounter

	rm -f kernel/bits32/data/*.o
	rm -f kernel/bits32/data/*.elf
	rm -f kernel/bits32/data/*.dep
	rm -f kernel/bits32/data/*.bin

	rm -f kernel/bits64/data/*.o
	rm -f kernel/bits64/data/*.elf
	rm -f kernel/bits64/data/*.dep
	rm -f kernel/bits64/data/*.bin