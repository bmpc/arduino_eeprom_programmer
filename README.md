I started following the [Ben Eater's awesome tutorials](https://eater.net/6502) to create my own retro computer using chips from the golden age of PCs. The 6502 and the z80 CPUs started the personal computer revolution back in the 70s and 80s.

Therefore, the plan is to build a very simple computer using one of these CPUs, a ROM, RAM, IO, Serial, and other logic chips.

I needed a way to flash the EEPROM.

<p float="left">
  <img src="/assets/IMG_0759.jpg" alt="Arduino EEPROM programmer photo 1" width="40%" />
  <img src="/assets/IMG_0760.jpg" alt="Arduino EEPROM programmer photo 2" width="40%" />
</p>

# Description

The goal of this project is to provide a way to flash a ROM file in an EEPROM chip using an Arduino Mega. The EEPROM chip that I'm using is the [AT28C25615U from ATMEL](/assets/eeprom_AT28C25615U.pdf).

I decided to use an Arduino shield with a ROM socket to flash a ROM file from my computer to the EEPROM.

The first step was to solder a bunch of wires for the address and data bus. There are also 3 control lines for the **Write Enable**, **Output Enable**, and **Chip Enable** pins specifically, that I needed to redirect to the Arduino as well. There are a total of 16 pins I need to control and the Arduino Mega is great for this job as it has enough digital pins. Using an Arduino Uno (or mini) is also possible, provided that we [use shift registers for the address lines](https://www.youtube.com/watch?v=K88pgWhEb1M).

# How it works

I created a small program (which you can find in this repository) for the Arduino to read and write the EEPROM.

There is also a Python program to flash a ROM file using the Arduino serial port. I reckon I was too lazy to implement a protocol like xmodem to allow a program like "screen" to upload files directly to the Arduino without the need for an additional script.

The Arduino program has the following options:
1. Read ROM
2. Write ROM
3. Read byte at address
4. Write byte to address
5. Erase ROM - 6502 NOP
6. Erase ROM

This menu is displayed when the Arduino is connected using a serial interface (e.g. Arduino monitor, or using a tool like screen or putty). 

Option 2 provides the ROM flashing capability. Of course, updating a ROM file using the monitor (or any other serial connection) is not practical as you need to type or paste the ROM into the monitor input.

This was the reason that I created the python script. This script is just an interface between the Arduino and a computer. Right now, the script only flashes the ROM file. 

All remaining options can be accessed using a serial interface.


# How to use

## Flashing a ROM file into the EEPROM

Required libs:
```
> pip install pyserial-asyncio
```

Running the script:
```
> python flash_eeprom.py <UART> <ROM_FILE>
```

Example (macOS):
```
> python flash_eeprom.py /dev/cu.usbmodem1413401 rom.bin
```

## Serial Interface

**Baud rate: 115200**

If you need to use any of the other options (Reading, Writing At, Reading At, and Erasing), please use a serial tool like screen or putty to connect to the Arduino.

Example (using screen in macOS):

```
screen -S eeprom_prog /dev/cu.usbmodem1413401 115200
```