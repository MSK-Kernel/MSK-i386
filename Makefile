CC=gcc
LD=ld
ASM=nasm

CFLAGS=-ffreestanding -m32 -c -fno-stack-protector -fno-builtin -fno-pic -fno-pie
LDFLAGS=-m elf_i386 -T linker/linker.ld -nostdlib

# Get all .c files in cmd/ EXCEPT init.c
CMD_SRC = $(filter-out cmd/init.c, $(wildcard cmd/*.c))
CMD_OBJ = $(CMD_SRC:.c=.o)
CMD_NAMES = $(notdir $(basename $(CMD_SRC)))

all: output/image/msk-i386

output/build:
	mkdir -p output/build

output/image:
	mkdir -p output/image

# Generate init.c from command files (not including itself)
cmd/init.c: $(CMD_SRC)
	@echo "// Auto-generated command registry" > $@
	@echo 'extern void register_cmd(const char* name, void (*func)(char*));' >> $@
	@echo "" >> $@
	@echo "void init_cmds() {" >> $@
	@for f in $(CMD_NAMES); do \
		echo "    extern void $${f}_cmd(char* args);" >> $@; \
		echo "    register_cmd(\"$$f\", $${f}_cmd);" >> $@; \
	done
	@echo "}" >> $@

cmd/%.o: cmd/%.c
	$(CC) $(CFLAGS) -o $@ $<

output/build/boot.o: boot/boot.asm | output/build
	$(ASM) -f elf32 boot/boot.asm -o output/build/boot.o

output/build/kernel.o: kernel/kernel.c | output/build
	$(CC) $(CFLAGS) kernel/kernel.c -o output/build/kernel.o

output/image/msk-i386: output/build/boot.o output/build/kernel.o cmd/init.o $(CMD_OBJ) | output/image
	@echo "Linking with $(words $(CMD_NAMES)) commands: $(CMD_NAMES)"
	$(LD) $(LDFLAGS) -o output/image/msk-i386 output/build/boot.o output/build/kernel.o cmd/init.o $(CMD_OBJ)

clean:
	rm -rf output
	rm -f cmd/*.o
	rm -f cmd/init.c
