CC=sh3eb-elf-gcc
CPP=sh3eb-elf-g++
OBJCOPY=sh3eb-elf-objcopy
MKG3A=mkg3a
RM=rm
CFLAGS=-m4a-nofpu -mb -fgcse-sm -fgcse-las -fgcse-after-reload -Isrc -O3 -fmerge-all-constants -mhitachi -flto -fuse-linker-plugin -Wall -Wextra -I../../../../include -lgcc -L../../../../lib -I./ -I../../cgdoom
LDFLAGS=$(CFLAGS) -nostartfiles -T../../../../toolchain/prizm.x -Wl,-static -Wl,-gc-sections -lgcc
CSOURCES=$(wildcard ../../cgdoom/*.c)
OBJECTS=$(CSOURCES:.c=.o)
PROJ_NAME=CG_Doom
BIN=$(PROJ_NAME).bin
ELF=$(PROJ_NAME).elf
ADDIN=$(PROJ_NAME).g3a
 
all: $(ADDIN)
 
 
$(ADDIN): $(BIN)
	$(MKG3A) -n :Doom -i uns:CGDOOM_UNSELECTED.BMP -i sel:CGDOOM_SELECTED.BMP $< $@
 
.s.o:
	$(CC) -c $(CFLAGS) $< -o $@
 
.c.o:
	$(CC) -c $(CFLAGS) $< -o $@
	
.cpp.o:
	$(CC) -c $(CFLAGS) $< -o $@
	
.cc.o:
	$(CC) -c $(CFLAGS) $< -o $@

$(ELF): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
$(BIN): $(ELF)
	$(OBJCOPY) -O binary $(PROJ_NAME).elf $(BIN)

clean:
	rm -f $(OBJECTS) $(PROJ_NAME).bin $(PROJ_NAME).elf $(ADDIN)
