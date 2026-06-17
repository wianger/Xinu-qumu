all: fs.img

FS_FILES = A \
		   B \
		   note.txt \
		   A.c \
		   B.c

fs.img: mkfs $(FS_FILES)
	./mkfs fs.img $(FS_FILES)

mkfs: mkfs.c
	$(CC) -o mkfs mkfs.c

CFLAGS32 = -m32 -fno-builtin -fno-stack-protector -fno-pic -nostdlib -O0 -Wall
LDFLAGS32 = -m elf_i386 -e main -Ttext 0x50001000

%.o: %.c
	$(CC) $(CFLAGS32) -c -o $@ $^

%.o: %.S
	$(CC) $(CFLAGS32) -c -o $@ $^

test: test.o asm.o
	$(LD) -m elf_i386 -e main -Ttext 0x80001000 -o $@ $^

A: A.o userlib.o
	$(LD) $(LDFLAGS32) -o $@ $^

B: B.o userlib.o
	$(LD) $(LDFLAGS32) -o $@ $^

clean:
	rm -f mkfs fs.img *.o test A B
