CROSS_COMPILE ?= 
CC = $(CROSS_COMPILE)gcc
TARGET = writer
CFLAGS := -Wall -Werror -g
SOURCES = writer.c

all: build 

build: $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f *.o $(TARGET) 

