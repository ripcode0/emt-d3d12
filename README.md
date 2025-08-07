# EMT Dependency Installer

### EMT Dependency Installer  
A CMake-based CLI tool for installing C/C++ libraries.  
With just a few flags, it can automatically clone, build, and install external libraries into your project.

### Features

- Pure CMake-based — no external scripting or dependencies
- Simple CLI interface with flexible flags
- Automatically clones and installs libraries from Git repositories
- Supports project integration via custom install `prefix`

### Requirements

- **CMake 3.22** or higher
- **Git**
- A **C++20** (or later) compatible compiler
- A build system such as **Ninja**, or any CMake-compatible generator

### Basic Usage

```bash
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=external
cmake --build build --target install
```