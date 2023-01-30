%.o: ../src/%.c
	gcc -c -m64 -ffreestanding -c $<

%.o: ../src/%.asm
	nasm -f elf64 -o $@ $<

kernel64.elf: entry.o $(notdir $(patsubst %.c, %.o, $(wildcard ../src/*.c))) $(notdir $(patsubst %.asm, %.o, $(wildcard ../src/*.asm)))
	ld -melf_x86_64 -T ../elf_x86_64.x -nostdlib -e Main -Ttext 0x200000 -o $@ $^

ifeq (Dependency.dep, $(Dependency.dep))
include Dependency.dep
endif