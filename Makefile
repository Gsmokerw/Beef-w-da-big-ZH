# Cross-Platform Makefile for Shadow Dimension (Linux / macOS / Windows)

CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall
TARGET = shadow_dimension

# Detect OS
UNAME_S := $(shell uname -s 2>/dev/null || echo Windows)

ifeq ($(UNAME_S), Linux)
    LDFLAGS += -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
endif

ifeq ($(UNAME_S), Darwin)
    CXX = clang++
    LDFLAGS += -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
endif

ifeq ($(findstring MINGW,$(UNAME_S)), MINGW)
    LDFLAGS += -lraylib -lopengl32 -lgdi32 -lwinmm -static -static-libgcc -static-libstdc++
    TARGET := $(TARGET).exe
endif

ifeq ($(findstring MSYS,$(UNAME_S)), MSYS)
    LDFLAGS += -lraylib -lopengl32 -lgdi32 -lwinmm -static -static-libgcc -static-libstdc++
    TARGET := $(TARGET).exe
endif

ifeq ($(UNAME_S), Windows)
    LDFLAGS += -lraylib -lopengl32 -lgdi32 -lwinmm -static -static-libgcc -static-libstdc++
    TARGET := $(TARGET).exe
endif

SRC = main.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET) $(TARGET).exe *.o

.PHONY: all clean
