DEVICE  = atmega168
F_CPU   = 24000000	# in Hz
FUSE_L  = # see below for fuse values for particular devices
FUSE_H  = 
FUSE_E	= 0xe2

CFLAGS  = -I . -D DEBUG=0
OBJECTS = main.o usart.o camera.o sccb.o cmdline.o cmd.o crc16.o
LDFLAGS = -Wl,-Map=mapfile,-cref

COMPILE = avr-gcc -Wall -Os -DF_CPU=$(F_CPU) $(CFLAGS) -mmcu=$(DEVICE) $(LDFLAGS)

test:
	avrProg -act=read -obj=fuse -addr=low

hex: main.hex

all: main.elf
	rm -f main.bin
	avr-objcopy -j .text -j .data -O binary main.elf main.bin
	avr-size main.elf
	
program:
	avrProg -act=erase -obj=flash
	avrProg -act=write -obj=flash -addr=0 -input=main.bin
	
erase:
	avrProg -act=erase -obj=flash

read:
	avrProg -act=read -obj=flash -addr=0 -npages=all

# rule for deleting dependent files (those which can be built by Make):
clean:
	rm -f *.hex *.lst *.obj *.cof *.list *.map *.eep.hex *.elf *.o mapfile

# Generic rule for compiling C files:
.c.o:
	$(COMPILE) -c $< -o $@

# Generic rule for assembling Assembler source files:
.S.o:
	$(COMPILE) -x assembler-with-cpp -c $< -o $@
# "-x assembler-with-cpp" should not be necessary since this is the default
# file type for the .S (with capital S) extension. However, upper case
# characters are not always preserved on Windows. To ensure WinAVR
# compatibility define the file type manually.

# Generic rule for compiling C to assembler, used for debugging only.
.c.s:
	$(COMPILE) -S $< -o $@


main.elf: $(OBJECTS)
	$(COMPILE) -o main.elf $(OBJECTS) $(LDFLAGS)

main.hex: main.elf
	rm -f main.hex main.eep.hex
	avr-objcopy -j .text -j .data -O ihex main.elf main.hex
	avr-size main.hex
