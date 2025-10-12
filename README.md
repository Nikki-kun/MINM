# MINM Messenger

A linux messenger application built with C++ and Qt.

## Building

### Prerequisites

- CMake 3.16 or higher
- Qt6 (Core, Widgets, Gui)
- C++20 compatible compiler

### Linux

```bash
git clone https://github.com/Nikki-kun/MINM.git
cd MINM

mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

./MINM
```

The executable will be created in the `build` directory.