all: _bits16_ _bits32_ _bits64_

_bits16_:
	clear
	make -C bits16

_bits32_:
	make -C bits32

_bits64_:
	make -C bits64

clean:
	clear
	make -C bits16 clean
	make -C bits32 clean
	make -C bits64 clean