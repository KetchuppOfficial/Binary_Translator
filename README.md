![Linux](https://img.shields.io/badge/Linux-FCC624?style=for-the-badge&logo=linux&logoColor=black)

# Binary Translator

This project is the final part of the programming and computer architecture course by [Ilya Dedinsky (aka ded32)](https://github.com/ded32).

## General Information

This program is a binary translator from bytecode generated by [my assembler](https://github.com/KetchuppOfficial/Processor) into x86-64 machine code. Execution of the machine code is implemented as a JIT-compiler. You can find information about my processor instructions and their nasm and x86-64 equivalents [here](/ISA.md) (pay special attention to **in** and **out**).

## Dependencies

Before installing **Binary_Translator** make sure you have [My_Lib](https://github.com/KetchuppOfficial/My_Lib) installed on your PC. This library is an integral part of the project. Nothing will work without it.

## Build and run

**Binary_Translator** is released for Linux only.

**Step 1:** Make empty folder.

**Step 2:** Clone Binary_Translator and My_Lib repositories to this folder:
```bash
git clone git@github.com:KetchuppOfficial/Binary_Translator.git
git clone git@github.com:KetchuppOfficial/My_Lib.git
cd Binary_Translator
```

**Step 3:** Build the project. 
```bash
username@machine:~/Some_Temp_Dir/Binary_Translator$ make
Collecting dependencies for "src/Binary_Translator.c"...
Collecting dependencies for "src/main.c"...
Compiling "src/main.c"...
Compiling "src/Binary_Translator.c"...
Building library...
Linking project...
```
Some options are supported:

1) You can turn on all **MY_ASSERT** macros from **My_Lib** (kind of debug mode):
```bash
make OPT=-DDEBUG
```
2) Some basic compiler options such as optimization flag **-O2** can be used:
```bash
make OPT=-O2
```
3) Binary translator supports *stress test mode* (program is executed for 10 000 000 times):
```bash
make OPT=-DSTRESS_TEST
```
4) It's possible to use previous options simultaneously:
```bash
make OPT=-DDEBUG\ -DSTRESS_TEST\ -O2    # don't forget backslash!
```

**Step 4:** Running
```bash
make run IN=input_file_name
```
The program won't work if you don't specify **input_file_name**.

## The aim of the project

My virtual processor shows low performance in many cases. Programs on my assembler language are executed indirectly: not on hardware CPU itself but via the C program. Let's try to boost programs written on my assembler by translating them into x86-64 machine code. So the main criterion of binary translation quality is the execution boost.

## Performance comparison

The goal is to compare execution time and find out, how faster the binary translator is comparing to the processor emulator. I carried out the measurements by tool **time**. The measurement error caused by translation bytecode into machine code (~0.002) is negligible comparing to the whole execution time.

The performance of binary translator and processor emulator was tested on two programs. The first one solves the good old quadratic equation 124.1 * x^2 - 2345.8 * x + 294.4 = 0. You can find source code [here](/data/Quadratic_For_Tests.txt). The second one calculates factorial of 10. You can find source code [here](/data/Factorial_For_Tests.txt).

### Results

|                                | Quadratic equation |  Factorial    |
|--------------------------------|--------------------|---------------|
| Emulator execution time, s     | 74.7 +/- 0.4       | 120.8 +/- 0.5 |
| Hardware CPU execution time, s | 3.276 +/- 0.003    | 5.71 +/- 0.01 |
| Performance boost, times       | 22.8 +/- 0.1       | 21.2 +/- 0.1  |

## Future

First of all, it's reasonable to perform some optimizations on the machine code before execution. It will reduce the number of instructions and, consequently, the performance will increase.

The second and the last, this task can be continued it terms of generating an ELF file as a result of binary translation.
