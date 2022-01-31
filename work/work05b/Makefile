# Makefile for RTGP lab lecture exercises - MacOS environment
# author: Davide Gadia
#Real-Time Graphics Programming - a.a. 2020/2021
#Master degree in Computer Science
#Universita' degli Studi di Milano

#name of the file
FILENAME = work05b

# Xcode compiler
CXX = clang++

# Include path
IDIR1 = ../../include
IDIR2 = ../../include/bullet

# Libraries path
LDIR = ../../libs/mac

# MacOS frameworks
MACFW = -framework OpenGL -framework IOKit -framework Cocoa -framework CoreVideo

# compiler flags:
CXXFLAGS  = -g -O0 -Wall -Wno-invalid-offsetof -std=c++11 -I$(IDIR1) -I$(IDIR2)

# linker flags:
LDFLAGS = -L$(LDIR) -lglfw3 -lassimp -lz -lIrrXML -lBullet3Common -lBulletCollision -lBulletDynamics -lLinearMath  $(MACFW)

SOURCES = ../../include/glad/glad.c $(FILENAME).cpp


TARGET = $(FILENAME).out

all:
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(SOURCES) -o $(TARGET)

.PHONY : clean
clean :
	-rm $(TARGET)
	-rm -R $(TARGET).dSYM
