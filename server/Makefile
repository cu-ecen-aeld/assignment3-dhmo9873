###############################################################################
# Makefile
#
# Builds the "writer" application.
# Supports native compilation and cross-compilation using CROSS_COMPILE.
# Author : Dhigvijay Mohan
# Date : 25/01/2026
#
# Examples:
#   make
#       Builds using the native gcc compiler
#
#   make CROSS_COMPILE=aarch64-none-linux-gnu-
#       Builds using the aarch64 cross compiler
#
#   make clean
#       Removes the aesdsocket binary and any object files
###############################################################################

# Optional cross-compiler prefix
# If not provided, native gcc will be used
CROSS_COMPILE ?=

# Compiler selection
CC ?= $(CROSS_COMPILE)gcc

# Name of the output executable
TARGET = aesdsocket

# Compiler flags:
# -Wall   Enable all warnings
# -Werror Treat warnings as errors
# -g      Include debug symbols
CFLAGS ?= -Wall -Werror -g 

LDFLAGS ?= -lpthread -lrt

# Source files
SOURCES = aesdsocket.c

# Default target
all: build

# Build target
# Compiles the source file(s) into the aesdsocket executable
build: $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)

# Clean target
# Removes the executable and all object files
.PHONY: clean
clean:
	rm -f *.o $(TARGET)


