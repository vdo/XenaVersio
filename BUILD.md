# Building XenaVersio

## Prerequisites

### 1. ARM GCC Toolchain

Install the ARM embedded toolchain for compiling code for the STM32H750 microcontroller.

**macOS (Homebrew):**
```bash
brew install --cask gcc-arm-embedded
```

**Linux (Debian/Ubuntu):**
```bash
sudo apt install gcc-arm-none-eabi
```

**Windows:**
Download from [ARM Developer](https://developer.arm.com/downloads/-/gnu-rm)

### 2. Daisy Libraries

Clone and build the required Daisy libraries:

```bash
# Create a directory for Daisy libraries (default location: ~/src/)
mkdir -p ~/src
cd ~/src

# Clone libDaisy
git clone https://github.com/electro-smith/libDaisy
cd libDaisy
make

# Clone DaisySP
cd ~/src
git clone https://github.com/electro-smith/DaisySP
cd DaisySP
make
```

### 3. dfu-util (for USB flashing)

**macOS:**
```bash
brew install dfu-util
```

**Linux:**
```bash
sudo apt install dfu-util
```

## Building

From the project root directory:

```bash
make
```

Build artifacts are placed in the `build/` directory:
- `XenaVersio.bin` - Binary for flashing
- `XenaVersio.hex` - Intel HEX format
- `XenaVersio.elf` - ELF executable

### Custom Library Paths

If your Daisy libraries are in a different location, override the paths:

```bash
make LIBDAISY_DIR=/path/to/libDaisy DAISYSP_DIR=/path/to/DaisySP
```

### Clean Build

```bash
make clean
make
```

## Flashing to Versio

### Method 1: USB DFU (Recommended)

1. Put the Versio module into bootloader mode:
   - Hold the encoder button while powering on, OR
   - Press the BOOT button on the Daisy Seed

2. Flash the firmware:
```bash
make program-dfu
```

### Method 2: ST-Link Programmer

Connect an ST-Link programmer to the SWD header and run:

```bash
make program
```

## Troubleshooting

**"arm-none-eabi-gcc: command not found"**
- Ensure the ARM toolchain is installed and in your PATH

**"Cannot find libDaisy"**
- Check that LIBDAISY_DIR points to the correct location
- Ensure libDaisy has been built (`make` in the libDaisy directory)

**"dfu-util: No DFU capable USB device available"**
- Ensure the module is in bootloader mode
- Check USB connection
- On Linux, you may need udev rules for USB access
