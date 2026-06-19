# Development Environment Setup

This directory contains scripts to establish the software environment needed to develop, compile, and upload the Arduino sketches in this project.

## What `setup.sh` installs

### Arduino IDE 2 (AppImage)
The graphical application for editing `.ino` sketch files, compiling them, and uploading to a connected board via USB. Arduino IDE 2 is not available in Fedora's dnf repos, so the script downloads the official AppImage from Arduino's servers and saves it to `~/Applications/`. An AppImage is a self-contained executable — no system installation needed.

### Arduino CLI (`arduino-cli`)
A command-line tool that can do everything the IDE does (compile, upload, manage libraries and boards) without a graphical interface. Useful for running from a terminal or automating builds.

### Arduino AVR Core (`arduino:avr`)
The compiler toolchain and board definitions for AVR-based Arduino boards. The **Arduino Mega 2560** used in this project is AVR-based, so this core is required to compile any of the sketches here.

### Adafruit SHT31 Library
Provides the `Adafruit_SHT31` class used in the sketches to communicate with the **SHT30 temperature and humidity sensor** over I2C. Required by: `two_pumps_temp_logging.ino`, `two_pumps_heater.ino`.

### Adafruit BusIO
A dependency of the SHT31 library. Handles low-level I2C/SPI communication so Adafruit sensor drivers work consistently across boards.

### Adafruit BME680 Library *(future sensor)*
Supports the **BME688** environmental sensor (Bosch), which measures temperature, humidity, barometric pressure, and gas/VOC air quality. The BME688 is backward-compatible with the BME680 driver. Installed now so the environment is ready when the sensor is added to the hardware.

### `dialout` group membership
On Linux, USB serial ports (`/dev/ttyUSB0`, `/dev/ttyACM0`, etc.) require `dialout` group membership to access without `sudo`. This is needed to upload sketches and read Serial Monitor output from the Arduino. A logout/login is required after this change takes effect.

## How to run

```bash
chmod +x setup.sh
./setup.sh
```

After the script finishes, log out and back in to activate serial port access, then verify:

```bash
arduino-cli version
arduino-cli board list   # shows connected Arduino boards
```
