# Makefile for RTGP lab lecture exercises - MacOS environment
# author: Davide Gadia
# Real-Time Graphics Programming - a.a. 2020/2021
# Master degree in Computer Science
# Universita' degli Studi di Milano

#name of the file
FILENAME = test

# Xcode compiler
CXX = clang++

# Include path
IDIR = ../../include

# Libraries path
LDIR = ../../libs/mac

# MacOS frameworks
MACFW = -framework OpenGL -framework IOKit -framework Cocoa -framework CoreVideo

# compiler flags:
CXXFLAGS  = -g -O0 -Wall -Wno-invalid-offsetof -std=c++11 -I$(IDIR)

# linker flags:
LDFLAGS = -L$(LDIR) -lglfw3 -lassimp -lz -lIrrXML $(MACFW)

SOURCES = ../../include/glad/glad.c $(FILENAME).cpp


TARGET = $(FILENAME).out

all:
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(SOURCES) -o $(TARGET)

.PHONY : clean
clean :
	-rm $(TARGET)
	-rm -R $(TARGET).dSYM
