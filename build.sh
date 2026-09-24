#!/bin/bash
set -e

# Detect OS
OS_NAME="$(uname -s)"
echo "Building Shadow Dimension for: $OS_NAME"

case "$OS_NAME" in
    Linux*)
        echo "Linux detected."
        # Check if raylib is installed or build via cmake/g++
        if pkg-config --exists raylib; then
            echo "Using system raylib via pkg-config..."
            g++ -std=c++17 -O2 main.cpp -o shadow_dimension $(pkg-config --cflags --libs raylib) -lGL -lm -lpthread -ldl -lrt -lX11
        else
            echo "Building with CMake (will automatically download Raylib 5.0)..."
            mkdir -p build
            cd build
            cmake .. -DCMAKE_BUILD_TYPE=Release
            cmake --build .
            cp shadow_dimension ../
            cd ..
        fi
        echo "========================================================"
        echo "Build SUCCESSFUL! Run with: ./shadow_dimension"
        echo "========================================================"
        ;;
    Darwin*)
        echo "macOS detected."
        if brew list raylib &>/dev/null; then
            echo "Using Homebrew raylib..."
            BREW_PREFIX="$(brew --prefix)"
            clang++ -std=c++17 -O2 main.cpp -o shadow_dimension \
                -I"$BREW_PREFIX/include" -L"$BREW_PREFIX/lib" -lraylib \
                -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -framework CoreAudio
        else
            echo "Building with CMake (will automatically download Raylib 5.0)..."
            mkdir -p build
            cd build
            cmake .. -DCMAKE_BUILD_TYPE=Release
            cmake --build .
            cp shadow_dimension ../
            cd ..
        fi
        echo "========================================================"
        echo "Build SUCCESSFUL! Run with: ./shadow_dimension"
        echo "========================================================"
        ;;
    *)
        echo "Unsupported OS: $OS_NAME. Please use CMake directly: cmake -B build && cmake --build build"
        exit 1
        ;;
esac
