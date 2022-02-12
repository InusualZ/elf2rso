# elf2rso
Tool to convert ELF (S)hared (O)bject to Nintendo (R)elocatable (S)hared (O)bject

# Command Line Options
* `-i` or `--input` - It's the ELF File to be parse. **Required**
* `-o` or `--output` - File path for the resultant file. Default is to change the input file extension to `.rso`

# Future Features
* List all `extern` (weak) symbol in the `import_symbol_table`. Currently only the needed (for relocations) symbol are listed
* Patch relocations with the `_unresolved` if available and possible (only branch instruction)
* Add argument option that would allow the user to pass a file with the functions/methods that you want to export from the module

# Credits
* [PistonMiner's elf2rel](https://github.com/PistonMiner/ttyd-tools/tree/master/ttyd-tools/elf2rel) for using some of his code as base for building this tool. Since Nintendo's REL module format is the precursor to this format.

# Documentation
* [ELF Specification](http://www.skyfree.org/linux/references/ELF_Format.pdf)
* [RSO Module Format](http://www.metroid2002.com/retromodding/wiki/RSO_(File_Format))
* [REL Module Format](http://www.metroid2002.com/retromodding/wiki/REL_(File_Format))
	
