INSTALLDIR= /home/bob/src/Emulators/exec09

all:		6309.bin

6309.bin:	6309.srec
		m2rom <6309.srec
		mv rom.bin 6309.bin

6309.srec:	6309.asm
		lwasm --format=srec --output=6309.srec --list=6309.lst --map=6309.map 6309.asm

clean:
		rm -f *~ *.hex *.srec *.lst *.bin a.out

