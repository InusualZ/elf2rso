# elf2rso
Tool to convert ELF (S)hared (O)bject to Nintendo (R)elocatable (S)hared (O)bject

# Command Line Options
* `-i` or `--input` - It's the ELF File to be parse. **Required**
* `-o` or `--output` - File path for the resultant file. Default is to change the input file extension to `.rso`

# Known Bugs
* Internal Relocations have a bug that I haven't being able to trace. (Currently don't have time to fix, but after that it should work)

# Credits
* [PistonMiner's elf2rel](https://github.com/PistonMiner/ttyd-tools/tree/master/ttyd-tools/elf2rel) for using some of his code as base for building this tool. Since Nintendo's REL module format is the precursor to this format.

# Documentation
* [ELF Specification](http://www.skyfree.org/linux/references/ELF_Format.pdf)
* [RSO Module Format](http://www.metroid2002.com/retromodding/wiki/RSO_(File_Format))
* [REL Module Format](http://www.metroid2002.com/retromodding/wiki/REL_(File_Format))
	
