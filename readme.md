# ARM-disasm

This disassembler library was initially developed for the ["Black Magic" debugger](https://github.com/compuphase/Black-Magic-Probe-Book). This debugger is a front-end for GDB, with specific support for the [Black Magic Probe](https://github.com/blackmagic-debug/blackmagic), and a focus on debugging micro-controller projects. The Black Magic Probe targets ARM Cortex M0, M3, and A7 architectures, so these are also the target architectures for the ARM-disasm library. Only the 32-bit architecture is supported (32-bit ARM and Thumb/Thumb-2).

The library is written in plain C with C99 extensions.

# License

ARM-disasm is licensed under the [Apache License version 2](https://www.apache.org/licenses/LICENSE-2.0).

# Usage

The first step is to initialize the library.

    void disasm_init(ARMSTATE *state, int flags);

The first parameter contains the state of the disassembly, and it is updated after each step. The decoded instructions are read from this variable too. The `flags` parameter contains options for decoding.

* `DISASM_ADDRESS`  prefix the instruction address to the result
* `DISASM_INSTR`    prefix the binary encoding of the instruction to the result
* `DISASM_COMMENT`  add comments with symbolic information, or the value in a different base, if applicable

When done, you need to clean up the internal structures. The `disasm_cleanup` function frees all memory that was allocation during the use of the other functions.

    void disasm_cleanup(ARMSTATE *state);

On the ARM architecture, jump targets (for control flow and function calls) are relative to the instruction address (register `PC`). Therefore, to resolve the address that is really jumped to, you need to know the value of `PC`. The starting value is set with `disasm_address`. The disassembler library then updates the instruction address while disassembling.

    void disasm_address(ARMSTATE *state, uint32_t address);

If you do not disassemble the code in a single run (from the starting address to the end), but instead (for example) on a function-by-function basis, you can call `disasm_address` at the start of each run.

## Decoding a single instruction

At a low level, there are two functions for disassembling ARM instructions: one for Thumb/Thumb-2 and one for the original 32-bit ARM instructions.

    bool disasm_thumb(ARMSTATE *state, uint16_t hw, uint16_t hw2);

    bool disasm_arm(ARMSTATE *state, uint32_t w);

The reason for splitting it into two functions is that this is how an ARM processor works, too. An ARM processor (assuming it supports both instruction sets) is in either ARM mode or in Thumb mode. This "mode" is set in the low bit of the instruction address (register `PC`), but it is not as simple as that instructions at an odd address are Thumb and instructions at an even address are ARM. In fact, instructions are always at an even address; the ARM processor only uses the low bit *to check which mode it is in*, it ignores this bit when fetching instructions from memory.

For the `disasm_thumb` function, you have to pass in two (consecutive) 16-bit "half" words. A Thumb instruction can be either 16-bit (in which case the `hw2` parameter is ignored) or 32-bit. The function returns the instruction size in bytes (so the return value is either 2 or 4). Function `disasm_arm` always returns 4, because all ARM instructions are 32-bit.

After calling either function, the output (as a string) can be obtained with:

    const char *disarm_result(ARMSTATE *state);

The pointer that this function returns is to an internal buffer, which gets overwritten on the next call to `disasm_thumb` or `disasm_arm`. Therefore, you will probably need to make a copy of it.

## Decoding a buffer

A complete buffer (in memory) can be disassembled with one function call. 

    bool disasm_buffer(ARMSTATE *state,
                       const unsigned char *buffer, size_t buffersize, int mode,
                       DISASM_CALLBACK callback, void *user);

Function `disasm_buffer` disassembles an entire buffer with binary machine language. It calls a callback function for each instruction, with the text of that instruction. Flags set with `disasm_init` apply here too. Parameter `mode` is set to the initial mode (`ARMMODE_ARM` or `ARMMODE_THUMB`), but if symbols (with a known mode) were added, `disasm_buffer` switches the mode as soon as it crosses the address of the symbol.

    typedef bool (*DISASM_CALLBACK)(uint32_t address, const char *text, void *user);

On each call to the callback, parameter `address` has the memory address of the instruction, and `text` contains one line of disassembled output. Parameter `user` contains the value that was passed in to function `disasm_buffer`. The callback function must return `true` to proceed with disassembly (it will make `disasm_buffer` abort if it returns `false`).

In principle, using `disasm_buffer` is similar to calling the `disasm_thumb` or `disasm_arm` functions in a loop, but since `disasm_buffer` knows the buffer size, it can look up and down the memory block to improve parameter decoding.

# ARM mode versus Thumb mode

This disassembler library offers no help in determining whether to use ARM mode or Thumb mode. The mode cannot be determined from the instructions themselves. You have to already know the mode to disassemble the buffer correctly. The ARM micro-controller itself uses the low bit of the `PC` register to indicate Thumb mode, even though the Thumb instructions are not actually on an odd address.

The DWARF debugging information stores the real (even) function addresses (and does not store the mode). Hence, DWARF dymbolic information is of no help (for this matter). The MAP file created by the linker, likewise, reports the real (even) function addresses of all symbols.

However, the ELF file format also has a symbol table, and this table records the address of Thumb mode functions on an odd address. Therefore, for an educated guess of whether to use ARM or Thumb modes, you need the ELF file, and you need a library to read and parse the ELF symbol table. The "ELF symbol table parser" is beyond the scope of this disassembler project.

Assuming that you have managed to read the ELF symbol table, the easiest way to pass this information to the disassembler library, is by adding each symbol with a call to `disasm_symbol`; see below for more on this function. Of note is that you need to correct the address of Thumb functions in the call to `disasm_symbol`; as stated above, the function is not really at an odd address. The ELF format uses the low bit in the address as a ARM/Thumb mode indicator.

A final note is that only function `disasm_buffer` uses the symbol table to select (and switch) the mode, based on the address. The `disasm_thumb` or `disasm_arm` functions ignore the symbol table.

# Symbolic information and string literals

If you have symbolic information for the binary code, you can add this with function `disasm_symbol` prior to starting to disassemble instructions.

    bool disasm_symbol(ARMSTATE *state, const char *name, uint32_t address, int mode);

Each call to this function adds a single symbol (either a function or a global variable). Only static addresses are supported, so local variables (with stack relative addresses) cannot be added. The purpose of adding these symbols is to show a function name on each `BL` or `BLX` instruction, or the name of a global variable on an `LDR` instruction. The option `DISASM_COMMENT` must also be set on `disarm_init` for this feature.

Parameter `mode` of function `disasm_symbol` is one of:

    ARMMODE_UNKNOWN     unknown mode for the symbol
    ARMMODE_ARM         this symbol refers to code in ARM mode (function)
    ARMMODE_THUMB       this symbol refers to code in Thumb mode (function)
    ARMMODE_DATA        this symbol refers to a data object

You can also mark only the mode for an address, by adding a symbol without a name. In this case, set the `name` parameter to `NULL` in the call to `disasm_symbol`.

The functions `disasm_thumb` and `disasm_arm` do not use the ARM/Thumb mode flags. However, function `disasm_buffer` uses it to switch between ARM mode and Thumb mode. See also the preceding section.

Function `disasm_buffer` also tries to look up literal strings, that the code refers to. For that end, `disasm_buffer` may need to look beyond the block that it has been passed (for disassembly). Many compilers store literal strings in the `.rodata` section in an ELF file. So, what you can do is to pass that section to `disasm_literals` before calling `disasm_buffer`.

    bool disasm_literals(ARMSTATE *state, const uint8_t *block, size_t blocksize, uint32_t address);

The function makes a copy of the block, so you may deallocate it after the call to `disasm_literals`. Multiple blocks may be added (these should not overlap, but no check is made whether they do). To clear the blocks with read-only data, call `disasm_cleanup` (which frees all memory, you need to call `disasm_init` again if you want to continue).

# Code pool

While disassembling, the ARM-disasm library inspects `LDR` instructions to find the start of literal pools, and it inspects the branch instructions to find the end of literal pools. Literal pools may appear in the middle of a function (with the code branching around it). It builds a map of known locations for instructions or literals, which it refers to as the "code pool".

When you throw completely unrelated bits of binary code at `disasm_thumb`, `disasm_arm` or `disasm_buffer`, you will need to clear the code pool in between those calls. Otherwise, there is a risk that an instruction isn't decoded, because the address was flagged as "literal pool" during the disassembly of an earlier block of code.

    void disasm_clear_codepool(ARMSTATE *state);

For example, the unit test for this library, which tests a bunch of instructions one by one, clears the code pool at the start of every test.

Alternatively, you can compact the code pool (or a range of it):

    void disasm_compact_codepool(ARMSTATE *state, uint32_t address, uint32_t size);

This function removes redundant entries in the codepool. Calling it is optional, but it optimizes processing the pool and reduces memory requirements. It should, however, only be called for address regions for which the codepool can be considered stable. For example, you can compact the address range of a full function after that function has been disassembled completely.

# Why build my own

I am aware of [pebble-disthumb](https://github.com/radare/pebble-disthumb) by pancake/radare.org, and of [DARM](https://github.com/jbremer/darm) by Jurriaan Bremer. The first, pebble-disthumb, is quite limited; but DARM also didn't get a couple of instructions right. DARM is not maintained anymore, and since I am not well versed in Python, the option of forking it and maintaining it myself was not attractive.

I am also aware of [Capstone](http://www.capstone-engine.org/). However, my goal was to "enrich" the disassembly with symbols and information extracted ELF and DWARF. I did not immediately see any "hooks" in Capstone to plug the symbolic information in. Plus, I felt that Capstone is overkill for a tiny debugger front-end that only supports ARM Cortex.

