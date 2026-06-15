.SILENT:
.EXPORT_ALL_VARIABLES:
.PHONY: all setup build clean fbink libvterm

# Define build directory name
BUILD_DIR ?= build
TOOLCHAIN_PATH = /home/clara/Documents/Projects/kobo/gcc-arm-9.2-2019.12-x86_64-arm-none-linux-gnueabihf
CROSS_TC = $(TOOLCHAIN_PATH)/bin/arm-none-linux-gnueabihf

CC:=$(CROSS_TC)-gcc
CXX:=$(CROSS_TC)-g++
STRIP:=$(CROSS_TC)-strip
AR:=$(CROSS_TC)-gcc-ar
RANLIB:=$(CROSS_TC)-gcc-ranlib
CFLAGS?=-O2


all: build

setup:
	cmake -S . -B $(BUILD_DIR)/ -G Ninja

build:
	cmake --build $(BUILD_DIR)/
clean:
	cmake --build $(BUILD_DIR) --target clean

deps: fbink libvterm 

fbink:
	cd third-party/FBInk && make static 

libvterm:
	cd third-party/libvterm && make CROSS_TC=$(CROSS_TC) -f ../../Makefile.vterm
