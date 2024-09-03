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

Examples
--------

Convert demo into text, replace `Player` with `Foobar` and convert back into binary:

    mvdtool bin2txt input.dm2 | sed -e 's/Player/Foobar' | mvdtool txt2bin - output.dm2

Extract text messages from demo:

    mvdtool strings demo.dm2 messages.txt

Split demo into two parts, from 5:00 to 10:00, and from 15:00 to 20:00:

    mvdtool split demo.mvd2 5:00,10:00@part1.mvd2 15:00,20:00@part2.mvd2

Remove two parts of the demo, from 1:00 to 2:00, and from 5:00 to 6:00:

    mvdtool cut input.mvd2 output.mvd2 1:00,2:00 5:00,6:00

# WASM

```make wasm```