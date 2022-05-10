![Linux](https://img.shields.io/badge/Linux-FCC624?style=for-the-badge&logo=linux&logoColor=black)

# Binary Translator

This project is the final part of a course on programming and computer architecture by [Ilya Dedinsky (aka ded32)](https://github.com/ded32) that he teaches in MIPT.

## General Information

This program is a binary translator from bytecode generated by [my assembler](https://github.com/KetchuppOfficial/Processor) into executable ELF file with x86-64 machine code.

## Dependencies

Before installing **Binary_Translator** make sure you have [My_Lib](https://github.com/KetchuppOfficial/My_Lib) installed on your PC. This library is an integral part of the project. Nothing will work without it.

## Build and run

**Binary_Translator** is released for Linux only.

Download this repository:
```bash
git clone git@github.com:KetchuppOfficial/Binary_Translator.git
cd Binary_Translator
```

Check if the relative path from **Binary_Translator.h** to **My_Lib.h** is the same as in **Binary_Translator.h**.
```C
#ifndef BINARY_TRANSLATOR_INSLUDED
#define BINARY_TRANSLATOR_INSLUDED

#include "../../My_Lib/My_Lib.h"        // <----- Look here

#define DEBUG 0
```

Then compile the program using tool **make**:
```bash
make
```

Congratulations! You can finally use **Binary_Translator**. To run the program, use **make** again:
```bash
make run IN=<input_file_name> OUT=<output_file_name>
```
Of course, you should change <input_file_name> and <output_file_name> into names of appropriate files. The program won't work if even one of these names isn't specified.

# WARNING!!!

**Binary_Translator** in not done yet!!! For God sake don't follow the instuction above!!!