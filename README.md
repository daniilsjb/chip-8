# CHIP-8 Emulator

![CHIP-8 Emulator](https://github.com/daniilsjb/CHIP-8/tree/master/res)

[CHIP-8](https://en.wikipedia.org/wiki/CHIP-8) is a virtual machine that was developed in the 1970s to allow programmers to write video games more easily. This project contains a functional emulator that can run programs written for the original CHIP-8, as well as provide some debugging facilities - users may pause execution at any time to inspect memory, or change the frequency at which the chip runs.

The goal of this project is to be fun and educational. Although CHIP-8 is a fairly simple system to implement, it involves some fundamental concepts necessary for any emulation or virtualization. This particular implementation is written in C using [SDL2](https://www.libsdl.org/). The source code includes comments that try to explain in more detail how the program works.

## Resources

If you want to learn more about how CHIP-8 operates, the following resources were used in the development of this project:

* <https://en.wikipedia.org/wiki/CHIP-8>
* <http://devernay.free.fr/hacks/chip8/C8TECH10.HTM>
* <http://mattmik.com/files/chip8/mastering/chip8.html>

There are plenty of ROMs available on the internet. This project includes some that were found [here](https://github.com/kripod/chip8-roms)!

## Building

To build this program, you need:

* C99
* SDL2
* CMake

First, clone this project into a directory on your computer:

```console
git clone https://github.com/daniilsjb/CHIP-8.git
cd CHIP-8
```

Second, run CMake to configure and build the program:

```console
mkdir build
cd build
cmake ..
cmake --build .
```

The above command assumes that `SDL2` is on your path. You may also explicitly specify path to the directory containing `SDL2` via the `SDL2_PATH` argument to CMake:

```console
cmake .. -DSDL2_PATH="/path/to/sdl2"
```

To run the executable, place the `SDL2` dynamic library file and the `res` folder into the same directory as your executable.

## Usage

Once you start the emulator, it will execute a default ROM program. To load another program into it, you need to drag the ROM file onto the emulator window. It is expected that the file has `.ch8` extension.

Alternatively, you may run the emulator from command prompt, in which case you may specify the path to ROM to be loaded on start. You may still load another ROM at runtime by using drag-and-drop.

## Controls

The following controls are supported:

| Key               | Description
|:------------------|:-----------------
| `P`               | Pause the emulator
| `0`               | Restart the current ROM
| `[`               | Decrease clock frequency
| `]`               | Increase clock frequency
| `=`               | Reset clock frequency
| `L`               | Disable audio
| `Up/Down`         | Move memory cursor when paused
| `Backspace`       | Load the default ROM

As for the key mappings, CHIP-8 uses a hexpad for input. The emulator provides virtual key mappings to suit modern keyboard:

```
     CHIP-8                       Emulator
+---------------+            +---------------+
| 1 | 2 | 3 | C |            | 1 | 2 | 3 | 4 |
+---+---+---+---+            +---+---+---+---+
| 4 | 5 | 6 | D |            | Q | W | E | R |
+---+---+---+---+   ----->   +---+---+---+---+
| 7 | 8 | 9 | E |            | A | S | D | F |
+---+---+---+---+            +---+---+---+---+
| A | 0 | B | F |            | Z | X | C | V |
+---+---+---+---+            +---+---+---+---+
```
