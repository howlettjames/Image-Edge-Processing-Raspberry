# Archivo Makefile con objetivos falsos, 
# variables automáticas y reglas de patrón

PROYECTO=conversion
DESTDIR=/usr/local/bin
CFLAGS=-Wall
LDFLAGS=-lm -lpthread
CC=gcc
OBJS := $(patsubst %.c,%.o,$(wildcard *.c))

all: $(PROYECTO)

%.o: %.c
	$(CC) -c $< $(CFLAGS)

$(PROYECTO): $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

.PHONY: clean

install: all
	if [ ! -d $(DESTDIR) ]; then \
		sudo mkdir $(DESTDIR); \
	fi; \
	sudo cp $(PROYECTO) $(DESTDIR)

uninstall: 
	sudo rm $(DESTDIR)/$(PROYECTO)

clean:
	rm -f *.o $(PROYECTO)

