# elf2rso
Tool to convert ELF (S)hared (O)bject to Nintendo (R)elocatable (S)hared (O)bject

# Command Line Options
* `-i` or `--input` - It's the ELF File to be parse. **Required**
* `-o` or `--output` - File path for the resultant file. Default is to change the input file extension to `.rso`
* `-a` or `--fullpath` - Use the fullpath of the input for the module's name
* `-e` or `--export` - Path of file containing the symbols allowed to be exported (Divided by `\n`)
* `-ne` or `--no-export` - Disable exporting any symbol from the module

# Future Features
* Patch relocations with the `_unresolved` if available and possible (only branch instruction)
* Create Static RSO. Module created from the `main.dol`. This module export the functions/method used by the _child_ modules

# Credits
* [PistonMiner's elf2rel](https://github.com/PistonMiner/ttyd-tools/tree/master/ttyd-tools/elf2rel) for using some of his code as base for building this tool. Since Nintendo's REL module format is the precursor to this format.

# Documentation
* [ELF Specification](http://www.skyfree.org/linux/references/ELF_Format.pdf)
* [RSO Module Format](http://www.metroid2002.com/retromodding/wiki/RSO_(File_Format))
* [REL Module Format](http://www.metroid2002.com/retromodding/wiki/REL_(File_Format))
	
