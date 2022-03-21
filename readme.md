# ARM-disasm

This disassembler library was initially developed for the "Black Magic" debugger. This debugger is a front-end for GDB, with specific support for the Black Magic Probe, and a focus on debugging micro-controller projects. The Black Magic Probe targets ARM Cortex M0, M3, and A7 architectures, so that are also the target archtitectures for the ARM-disasm library. Only the 32-bit archiecture is supported (32-bit ARM and Thumb/Thumb-2).

The library is written in plain C with C99 extensions.

## License

ARM-disasm is licensed under the [Apache License version 2](https://www.apache.org/licenses/LICENSE-2.0).

## Usage

The first step is to initiallize the library.

    void disasm_init(ARMSTATE *state, int flags);

The first parameter contains the state of the disassembly, and it is updated after each step. The decoded instructions are read from this variable too. The `flags` parameter contains options for decoding.

* `DISASM_ADDRESS`  prefix the instruction address to the the result
* `DISASM_INSTR`    prefix the binary encoding of the instruction to the result
* `DISASM_COMMENT`  add comments with symbolic information, or the value in a different base, if applicable

When done, you need to clean up the internal structures. The `disasm_cleanup` function frees all memory that was allocation during the use of the other functions.

    void disasm_cleanup(ARMSTATE *state);

Jump targets (for control flow and function calls) is relative to the instruction address (register `PC`) on the ARM architecture. Therefore, to resolve the address that is really jumped to, you need to know the value of `PC`. The starting value is set with `disasm_address`. The disassembler library then updates the instruction address while disassembling.

    void disasm_address(ARMSTATE *state, uint32_t address);

If you do not disassemble the code in a single run (from the starting address to the end), but instead (for example) on a function-by-function basis, you can call `disasm_address` at the start of each run.

At a low level, there are two functions for disassembling ARM instructions: one for Thumb/Thumb-2 and one for the original 32-bit ARM instructions.

    int disasm_thumb(ARMSTATE *state, uint16_t hw, uint16_t hw2);

    int disasm_arm(ARMSTATE *state, uint32_t w);

The reason for splitting it into two functions is that this is how an ARM processor works, too. An ARM processor (assuming it supports both instruction sets) is in either ARM mode or in Thumb mode. This "mode" is set in the low bit of the instruction address (register `PC`), but it is not as simple as that instructions at an odd address are Thumb and instructions at an even address are ARM. In fact, instructions are always at an even address; the ARM processor *only* uses the low bit to check which mode it is in, it ignores this bit when fetching instructions from memory.

For the `disasm_thumb` function, you have to pass in two (consecutive) 16-bit "half" words. A Thumb instruction can be either 16-bit (in which case the `hw2` parameter is ignored) or 32-bit. The function returns the instruction size in bytes (so the return value is either 2 or 4). Function `disasm_arm` always returns 4, because all ARM instructions are 32-bit.

After calling either function, the output (as a string) can be obtained with:

    const char *disarm_result(ARMSTATE *state);

The pointer that this function returns is to an internal buffer, which gets overwritten on the next call to `disasm_thumb` or `disasm_arm`. Therefore, you will probably need to make a copy of it.

If you have symbolic information for the binary code, you can add this with function `disasm_symbol` prior to starting to disassemble instructions.

    void disasm_symbol(ARMSTATE *state, const char *name, uint32_t address, int mode);

Each call to this function adds a single symbol (either a function or a global variable). Only static addresses are supported, so local variables (with stack relative addresses) cannot be added. The purpose of adding these symbols is to show a function name on each `BL` or `BLX` instruction. The option `DISASM_COMMENT` must be set on `disarm_init` for this feature, too.

Parameter `mode` of function `disasm_symbol` is one of:

    ARMMODE_UNKNOWN     unknown mode for the symbol
    ARMMODE_ARM         this symbol refers to code in ARM mode (function)
    ARMMODE_THUMB       this symbol refers to code in Thumb mode (function)
    ARMMODE_DATA        this symbol refers to a data object

The functions `disasm_thumb` and `disasm_arm` do not use the symbol mode. However, function `disasm_buffer` uses it to switch between ARM mode and Thumb mode.

    void disasm_buffer(ARMSTATE *state,
                       const unsigned char *buffer, size_t buffersize,
                       DISASM_CALLBACK callback, int mode);

Function `disasm_buffer` disassembles an entire buffer with binary machine language. It calls a callback function for each instruction, with the text of that instruction. Flags set with `disasm_init` apply here too.

    typedef void (*DISASM_CALLBACK)(const char *text);

This disassembler library offers no help in determining whether to use ARM mode or Thumb mode. The mode cannot be determined from the instructions themselves. You have to already know the mode to disassemble the buffer correctly. The ARM micro-controller itself uses the low bit of the `PC` register to indicate Thumb mode, even though the Thumb instructions are not actually on an odd address. The DWARF debugging information stores the real (even) function addresses (and does not store the mode). However, the ELF file format also has a symbol table, and this table records the address of Thumb mode functions on an odd address.

To summarize, if you read the ELF symbol table, and then add the functions in that table to this library with `disasm_symbol` (with corrected address and mode appropriately set), function `disasm_buffer` select (and switch) the mode automatically, based on the address.

## Why build my own

I am aware of [pebble-disthumb](https://github.com/radare/pebble-disthumb) by pancake/radare.org, and of [DARM](https://github.com/jbremer/darm) by Jurriaan Bremer. The first, pebble-disthumb, is quite limited; but DARM also didn't get a couple of instructions right. DARM is not maintained anymore, and since I am not well versed in Python, the option of forking it and maintaining it myself was not attractive.

I am also aware of [Capstone](http://www.capstone-engine.org/). However, my goal was to "enrich" the disassembly with information extracted from DWARF debugging information. I did not immediately see any "hooks" in Capstone to plug the symbolic information in. Plus, I felt that Capstone is overkill for a tiny debugger that only supports ARM Cortex.

