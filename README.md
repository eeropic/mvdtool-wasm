mvdtool
=======

Utility for working with demos in DM2 and MVD2 formats for Quake 2.

The following modes of operation are supported:

* bin2txt — Convert demo from binary to text format
* txt2bin — Convert demo from text to binary format
* strings — Parse demo and print messages with timestamps
* split — Split demo by timestamps into multiple files
* cut — Remove parts of demo specified by timestamps

Split and cut modes support MVD2 demos, but not DM2.

Developer modes:

* hash — Compute SHA-1 hashes of binary demo blocks
* diff — Compare demos block-by-block

Syntax
------

    mvdtool bin2txt [input] [output]
    mvdtool txt2bin [input] [output]
    mvdtool strings [input] [output]
    mvdtool split <input> <start1,end1@output1> [start2,end2@output2] [...]
    mvdtool cut <input> <output> <start1,end1> [start2,end2]  [...]
    mvdtool hash [input] [output]
    mvdtool diff [input1] [input2]

Start/end timestamps can be specified in one of the following formats:

* N — raw block number
* SS.F, where SS are seconds, F are tenths of second
* MM:SS, where MM are minutes, SS are seconds
* MM:SS.F, where MM are minutes, SS are seconds, F are tenths of second

Both input and output files can be omitted if syntax allows, or the special
argument `-` can be used instead, which means to read from stdin or write to
stdout.
