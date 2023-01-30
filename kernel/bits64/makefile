all: _entry_ _c_dep_ _bin_link_ ../../build/kernel64.bin

_entry_: src/Entry.s
	nasm -f elf64 -o data/entry.o $<

_c_dep_:
	gcc -c -m64 -ffreestanding -MM $(wildcard ./src/*.c) > data/Dependency.dep

_bin_link_:
	make -C data

../../build/kernel64.bin:
	objcopy -j .text -j .data -j .rodata -j .bss -S -O binary data/kernel64.elf $@