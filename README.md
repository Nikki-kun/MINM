![Главный интерфейс](assets/logo.png)

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

## Generating Documentation

### Prerequisites

- Doxygen
- Graphviz (optional, for diagram generation)

### Generating Documentation

To generate HTML documentation using Doxygen:

```bash
doxygen Doxyfile
```

### Documentation Location

The generated HTML documentation will be located in the `docs/html/` directory. You can open `docs/html/index.html` in any web browser to browse the documentation.

### Documentation Configuration

The project includes a `Doxyfile` configuration file that sets up:
- Source code parsing from `src/` directory
- HTML output to `docs/html/`
- Class diagrams and inheritance graphs (if Graphviz is installed)