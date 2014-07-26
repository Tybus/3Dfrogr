#DEVKITPRO=/home/yourname/devkitPro
# enter the path to your devkitpro above, this is just an example

DEVKITARM=$(DEVKITPRO)/devkitARM
export Path:=$(DEVKITPRO);$(DEVKITARM);$(Path)

PROJ = $(shell pwd)
CC = $(DEVKITARM)/bin/arm-none-eabi-gcc
LINK = $(DEVKITARM)/bin/arm-none-eabi-ld
OBJCOPY = $(DEVKITARM)/bin/arm-none-eabi-objcopy

CTRULIB = $(PROJ)/libctru
CFLAGS += -Wall -std=c99 -march=armv6 -O3 -I"$(CTRULIB)/include" -I$(DEVKITPRO)/libnds/include
LDFLAGS += --script=$(PROJ)/resources/linker.ld -L"$(DEVKITARM)/arm-none-eabi/lib" -L"$(CTRULIB)/lib"

CFILES = $(wildcard source/*.c)
OFILES = $(CFILES:source/%.c=build/%.o)
DFILES = $(CFILES:source/%.c=build/%.d)
SFILES = $(wildcard source/*.s)
OFILES += $(SFILES:source/%.s=build/%.o)
PROJECTNAME = ${shell basename "$(CURDIR)"}

.PHONY:=all dir

all: dir $(PROJECTNAME).bin

dir:
	@mkdir -p build

$(PROJECTNAME).bin: $(PROJECTNAME).elf
	$(OBJCOPY) -O binary $< $@

$(PROJECTNAME).elf: $(OFILES)
	$(LINK) $(LDFLAGS) -o $(PROJECTNAME).elf $(filter-out build/crt0.o, $(OFILES)) -lctru -lc

clean:
	@rm -f build/*.o build/*.d
	@rm -f $(PROJECTNAME).elf $(PROJECTNAME).bin
	@echo "all cleaned up !"

-include $(DFILES)

build/%.o: source/%.c
	$(CC) $(CFLAGS) -c $< -o $@
	@$(CC) -MM $< > build/$*.d

build/%.o: source/%.s
	$(CC) $(CFLAGS) -c $< -o $@
	@$(CC) -MM $< > build/$*.d
