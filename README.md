# jpp Compiler for Termux (ARM64)

The jpp compiler is a custom compiler implemented in C++, designed and highly optimized for ARM64 architecture. It is specifically built to deliver maximum execution performance within the Termux environment on mobile devices and single-board computers.

---

## Key Features

- **ARM64 Optimization:** Engineered specifically to leverage the performance characteristics of ARM64 processors.
- **High Performance:** Minimal binary size coupled with rapid compilation speeds.
- **Termux Native:** Built specifically for deployment and execution inside the Termux environment without requiring complex system configurations.

---

## Prerequisites

Before proceeding with the installation, ensure your Termux packages are updated and that the required utilities (`curl`, `wget`, `git`, and `make`) are installed. Execute the following command:

```bash
pkg update && pkg upgrade -y
pkg install curl wget git make -y
```

---

## Installation Methods

Select one of the following methods to install the jpp compiler on your environment.

### Method 1: Automated Installation via wget
This method downloads and runs the installation script automatically using wget:

```bash
wget -qO- https://githubusercontent.com | bash
```

### Method 2: Automated Installation via curl
Alternatively, you can use curl to fetch and execute the installation script:

```bash
curl -fsSL https://githubusercontent.com | bash
```

### Method 3: Manual Compilation from Source
If the automated scripts fail, or if you prefer to build the project manually, clone the repository and compile it using make:

```bash
git clone https://github.com/jaokupp/jppcompiler.git
cd jppcompiler
make && make install
source ~/.bashrc
```



---

## License and Intellectual Property

All rights reserved. The source code of this project remains proprietary. The jpp compiler is distributed exclusively as compiled binary executables or controlled builds to protect intellectual property.
