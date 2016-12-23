SRCS=6809.c cfdisk.c command.c disk.c eon.c fileio.c gtron.c \
	imux.c ioexpand.c machine.c main.c mc6850.c miscsbc.c \
	mmu.c monitor.c serial.c symtab.c timer.c wpc.c wpclib.c \
	debug.c

CFLAGS=-DHAVE_READLINE -DHAVE_TERMIOS -DH6309
LDFLAGS=

LIBS=-lreadline

CC=gcc
LD=gcc

HDRS=$(wildcard *.h)
OBJS=$(subst .c,.o,$(SRCS))
EXE=em09

.c.o: $(HDRS)
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY:	all info clean

all: $(EXE)

info:
	@echo Sources $(SRCS)
	@echo Headers $(HDRS)
	@echo Objects $(OBJS)

$(EXE): $(OBJS)
	$(LD) -o $@ $(OBJS) $(LIBS)

clean:
	rm -f $(EXE) $(OBJS) *~

