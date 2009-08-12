INCLUDES=-I/usr/include/tcl8.5
LIBS=-ltcl8.5
CFLAGS=-W -Wall -g -O2
LDFLAGS=-pg

ifdef $(PROFILE)
	CFLAGS+=-pg
endif

APP=sigcont

SRC=$(wildcard *.c)
OBJS=$(SRC:.c=.o)

%.o:%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

all: $(APP)

$(APP): $(OBJS)
	$(CC) $(LDFLAGS) $(LIBS) -o $@ $<

clean:
	-rm -f $(OBJS) $(APP) gmon.out >/dev/null 2>&1

.PHONY: all clean

