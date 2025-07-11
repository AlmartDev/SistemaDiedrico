<p align="center">
<img src="https://github.com/AlmartDev/SistemaDiedrico/blob/main/assets/favicon.ico">
</p>
<h1 align="center">Sistema Diedrico (Dihedral System)</h1>

[![Web Version](https://img.shields.io/badge/Live%20Demo-Available-green)](https://almartdev.github.io/SistemaDiedrico)

A 3D visualization tool for dihedral system projections.

### Want to check out the web version?
Development web build (ENGLISH): https://almartdev.github.io/SistemaDiedrico/

Versión de desarrollo (ESPAÑOL): https://almartdev.github.io/SistemaDiedrico/?lang=ES

▶ **How does dihedral look like?** [YouTube](https://youtu.be/H5uxDwpfXNs)

## About

The dihedral system is a drawing method that represents three-dimensional objects on a plane using two mutually perpendicular projection planes. 
As a student struggling with spatial visualization, I couldn't find adequate software to help me understand these projections - so I built my own! This tool provides:

- Interactive 3D visualization with guizmos and UI!
- Simultaneous dihedral projections (horizontal and vertical planes)
- Support for points, lines, and planes (for now)
- Web and desktop versions
- Save/Loading projects
- Documentation (ES): ![Github Wiki](https://github.com/AlmartDev/SistemaDiedrico/wiki)

## Installation

### Requirements
- CMake ≥ 3.14
- C++17 compatible compiler
- Python 3 (for GLAD)

### Linux/macOS
```bash
git clone https://github.com/almartdev/sistemadiedrico.git
cd sistemadiedrico
mkdir build && cd build
cmake ..
make
# Run with: ./diedrico
```

### Windows
Consider using CMake GUI and Visual Studio.

### Web Build
```bash
# On project root
.\web_build.bat # UPDATE YOUR USERNAME ON THIS FILE
cd .\build_web\
ninja

# Simple server to test your build
python -m http.server 8000
```

### License
Please see "LICENSE" for more information!
