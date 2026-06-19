#!/usr/bin/env bash
# Setup script for hydrothermalVent Arduino development environment
# Target OS: Fedora Linux

set -e  # Exit immediately if any command fails

echo "=== Installing Arduino IDE 2 ==="
# Arduino IDE 2 is not in Fedora's dnf repos, so we download the official
# AppImage directly from Arduino. An AppImage is a self-contained executable
# that runs on any Linux distro without installation.
ARDUINO_IDE_VERSION="2.3.4"
ARDUINO_IDE_APPIMAGE="arduino-ide_${ARDUINO_IDE_VERSION}_Linux_64bit.AppImage"
ARDUINO_IDE_URL="https://downloads.arduino.cc/arduino-ide/${ARDUINO_IDE_APPIMAGE}"

if [ ! -f "$HOME/Applications/$ARDUINO_IDE_APPIMAGE" ]; then
    mkdir -p "$HOME/Applications"
    echo "Downloading Arduino IDE $ARDUINO_IDE_VERSION..."
    curl -L "$ARDUINO_IDE_URL" -o "$HOME/Applications/$ARDUINO_IDE_APPIMAGE"
    chmod +x "$HOME/Applications/$ARDUINO_IDE_APPIMAGE"
    echo "Arduino IDE saved to ~/Applications/$ARDUINO_IDE_APPIMAGE"
    echo "Run it with: ~/Applications/$ARDUINO_IDE_APPIMAGE"
else
    echo "Arduino IDE already downloaded, skipping."
fi

echo "=== Installing Arduino CLI ==="
# Arduino CLI lets you compile and upload sketches from the terminal without
# opening the GUI — useful for scripting or headless environments.
if command -v arduino-cli &>/dev/null; then
    echo "Arduino CLI already installed, skipping."
else
    # BINDIR must be set explicitly; without it the install script defaults to
    # a 'bin/' subdirectory of whatever the current working directory is, which
    # means running this script from inside the repo drops the binary in the
    # repo's bin/ folder instead of ~/bin.
    curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR="$HOME/bin" sh
    # Move the binary system-wide so any user or script can call it without
    # modifying PATH.
    if [ -f "$HOME/bin/arduino-cli" ]; then
        sudo mv "$HOME/bin/arduino-cli" /usr/local/bin/arduino-cli
    fi
fi

echo "=== Initializing Arduino CLI config ==="
# Creates ~/.arduino15/arduino-cli.yaml, which stores board index URLs,
# library paths, and other preferences used by all subsequent CLI commands.
if [ -f "$HOME/.arduino15/arduino-cli.yaml" ]; then
    echo "Arduino CLI config already exists, skipping."
else
    arduino-cli config init
fi

echo "=== Updating board index ==="
# Downloads the latest list of available boards and cores from Arduino's
# servers so the CLI knows what hardware is supported.
arduino-cli core update-index

echo "=== Installing Arduino AVR core (for Mega 2560) ==="
# The AVR core contains the compiler toolchain and board definitions for
# Arduino boards based on AVR chips, including the Mega 2560 used here.
arduino-cli core install arduino:avr

echo "=== Installing required libraries ==="
# Adafruit SHT31 Library: provides the Adafruit_SHT31 class used in the
# sketches to read temperature and humidity from the SHT30 sensor over I2C.
arduino-cli lib install "Adafruit SHT31 Library"
# Adafruit BusIO: a dependency of the SHT31 library that abstracts I2C/SPI
# communication so sensor drivers work across different Arduino-compatible boards.
arduino-cli lib install "Adafruit BusIO"

echo "=== Installing future sensor libraries ==="
# Adafruit BME680 Library: supports the BME688 environmental sensor (Bosch),
# which measures temperature, humidity, pressure, and gas/air quality (VOC).
# The BME688 is backward-compatible with the BME680 driver.
# Planned as a future addition to the hydrothermalVent hardware.
arduino-cli lib install "Adafruit BME680 Library"

echo "=== Adding user to dialout group (serial port access) ==="
# On Linux, USB serial ports (e.g. /dev/ttyUSB0, /dev/ttyACM0) are owned by
# the 'dialout' group. Without this, uploads will fail with "permission denied".
sudo usermod -aG dialout "$USER"

echo "=== Installing dfu-programmer (16U2 firmware recovery tool) ==="
# The Arduino Mega 2560 uses an ATmega16U2 chip as its USB-to-serial bridge.
# Under normal operation this chip presents as /dev/ttyACM0.
#
# DISCOVERY: On first setup, lsusb showed the 16U2 stuck in DFU (Device
# Firmware Upgrade) bootloader mode (USB ID 03eb:2fef) instead of the normal
# serial bridge firmware. In DFU mode no /dev/ttyACM0 appears and uploads fail
# with "cannot open port". This can happen if the 16U2 firmware was erased or
# the board was powered on with the HWB and GND pads on the 16U2 ICSP header
# shorted.
#
# dfu-programmer is the tool used to reflash the 16U2 over USB while it is in
# DFU mode. It does not require a separate programmer.
#
# NOTE: dfu-programmer is not in Fedora's standard dnf repos, so we build it
# from source. Build deps: git, autoconf, libusb-devel.
if command -v dfu-programmer &>/dev/null; then
    echo "dfu-programmer already installed, skipping."
else
    # On Fedora the libusb development package is libusb1-devel (not libusb-devel).
    # automake provides aclocal, required by dfu-programmer's bootstrap.sh.
    sudo dnf install -y git autoconf automake libusb1-devel
    rm -rf /tmp/dfu-programmer
    git clone https://github.com/dfu-programmer/dfu-programmer.git /tmp/dfu-programmer
    cd /tmp/dfu-programmer
    ./bootstrap.sh
    ./configure
    make
    sudo make install
    cd -
fi

echo "=== Restoring ATmega16U2 USB-serial firmware (if board is in DFU mode) ==="
# Check whether the Mega is currently stuck in DFU mode (USB ID 03eb:2fef).
# If it is, reflash the stock Arduino USB-serial firmware so the board appears
# as a normal serial port again.
#
# The firmware hex is downloaded from the official ArduinoCore-avr repository.
# This is the same file bundled with the Arduino IDE under:
#   hardware/arduino/avr/firmwares/atmegaxxu2/arduino-usbserial/
FIRMWARE_HEX="/tmp/Arduino-usbserial-atmega16u2-Mega2560-Rev3.hex"
FIRMWARE_URL="https://raw.githubusercontent.com/arduino/ArduinoCore-avr/master/firmwares/atmegaxxu2/arduino-usbserial/Arduino-usbserial-atmega16u2-Mega2560-Rev3.hex"

if lsusb | grep -q "03eb:2fef"; then
    echo "ATmega16U2 detected in DFU mode — reflashing USB-serial firmware..."
    curl -fsSL "$FIRMWARE_URL" -o "$FIRMWARE_HEX"
    sudo dfu-programmer atmega16u2 erase --force
    sudo dfu-programmer atmega16u2 flash "$FIRMWARE_HEX"
    sudo dfu-programmer atmega16u2 reset
    echo "Firmware flashed. Unplug and replug the USB cable, then re-run:"
    echo "  arduino-cli board list"
    echo "to confirm /dev/ttyACM0 appears before uploading sketches."
else
    echo "ATmega16U2 not in DFU mode — no firmware recovery needed."
fi

echo ""
echo "=== Setup complete ==="
echo "NOTE: Log out and back in (or run 'newgrp dialout') for serial port access to take effect."
echo "To launch Arduino IDE: ~/Applications/$ARDUINO_IDE_APPIMAGE"
echo "To verify CLI: arduino-cli version"
echo "To list connected boards: arduino-cli board list"
