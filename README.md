# 8085 Two-Pass Assembler

A C++ implementation of a two-pass assembler for the Intel 8085 microprocessor.

The assembler converts 8085 assembly language instructions into hexadecimal machine code and binary output. It supports instruction parsing, symbol-table generation, label resolution, address calculation, syntax validation, and machine-code generation.

## Features

* Two-pass assembler architecture
* 8085 instruction parsing
* Mnemonic-to-opcode translation
* Label and symbol resolution
* 8-bit and 16-bit operands
* Immediate addressing
* Register and register-pair instructions
* Conditional and unconditional jumps
* CALL and RET instructions
* Memory instructions such as LDA, STA, LHLD and SHLD
* IN and OUT instructions
* RST instructions
* DB directive
* Symbol table generation
* Hexadecimal machine-code output
* Binary machine-code output
* Assembly error reporting with line numbers

## Architecture

The assembler uses two passes.

### Pass 1 — Symbol Table Generation

The source program is scanned to determine the address of each instruction and label.

For example:

```asm
LOOP:   INR A
        CPI 05H
        JNZ LOOP
```

The assembler records the address associated with `LOOP`.

### Pass 2 — Machine-Code Generation

The source is scanned again. Instructions are converted into their corresponding 8085 opcodes and labels are replaced with their resolved memory addresses.

```text
Assembly Source
       |
       v
   Lexical Parsing
       |
       v
      Pass 1
       |
       v
   Symbol Table
       |
       v
      Pass 2
       |
       v
 Machine Code
    /       \
   v         v
HEX Output  Binary Output
```

## Example

Input:

```asm
MVI A, 05H
MVI B, 03H
ADD B
STA 2050H
HLT
```

Generated machine code:

```text
3E 05 06 03 80 32 50 20 76
```

## Requirements

* C++17 or later
* GCC / MinGW-w64

## Compilation

Compile the assembler using:

```bash
g++ -std=c++17 src/assembler8085.cpp -o assembler8085
```

On Windows:

```powershell
g++ -std=c++17 src/assembler8085.cpp -o assembler8085.exe
```

## Usage

Run the assembler with an 8085 assembly source file:

```powershell
.\assembler8085.exe examples/program.asm
```

The assembler generates:

```text
output.hex
output.bin
```

## Example Assembly Program

```asm
; Add two numbers and store result at 2050H

        MVI A, 05H
        MVI B, 03H
        ADD B
        STA 2050H
        HLT
```

## Concepts Demonstrated

This project demonstrates:

* Compiler/assembler design
* Two-pass assembly
* Symbol tables
* Instruction encoding
* Opcode generation
* Address resolution
* Little-endian 16-bit address encoding
* File I/O
* Error handling
* C++ data structures and parsing

## Future Improvements

Possible extensions include:

* Complete 8085 instruction-set coverage
* Better lexical analysis
* More detailed syntax diagnostics
* Listing-file generation
* Interactive command-line interface
* Debugging and breakpoint support
* 8085 emulator integration
* Memory and register visualization
* Intel HEX output
* GUI-based assembler interface

## Author

Anmolabjot

Electronics and Communication Engineering
