# AT&T Optimized Lookup
Given an [AT&amp;T text format](https://github.com/hfst/hfst/wiki/HfstFst2Txt) transducer, this tool performs lookup.

## Compile
All you need is [cmake](https://cmake.org/) (>=3.1) and a c++11 compliant compiler (or Visual Studio 17, or later)

With gcc and make:

    cmake .
    make
 
With Visual C++
 
    cmake -G "NMake Makefiles" .
    nmake

## Usage
 - positional argument: AT&T (text) format transducer filename
 - optional arguments:
    
        -i --input 'filename' default: ""
                input file to analyze, stdin if empty
        -o --output 'filename' default: ""
                output file, stdout if empty
        -t --time 'double' default: 0
                time limit (in seconds) when not to search further
                unlimited if set to 0
        -n 'size_t' default: 0
                max number of results for one word
                unlimited if set to 0
        -d --depth 'size_t' default: 0
                maximum depth to go down during lookup
                unlimited if set to 0
        -f --flag 'int' default: 1
                how to treat the flag diacritics
                0: ignore (off)
                1: obey
                2: negative, return only those paths that were invalid flag-wise but correct analysis
                otherwise.
                possible values: 0 1 2
        -e --enc --encoding 'int' default: 2
                encoding of the transducer and also the input/output
                0: ASCII
                1: fixed one byte characters, use this for any ISO-8859 character table (same as
                ASCII.)
                2: UTF8
                3: UCS2,        endianness depends on your CPU
                4: UTF16,       endianness depends on your CPU
                5: UTF32,       endianness depends on your CPU
                possible values: 0 1 2 3 4
                
### Example

    > attol english.hfst.txt
    Transducer states: 466865
    transitions: 1340917
    memory: 33639656bytes
    > be
    be[V]+V+INF
    be[V]+V+PRES
    be[N]+N
    
    > plt
    ?

    > gone
    gone[ADJ]+ADJ
    go[V]+V+PPART
    go[V]+V+PAST
    go+V+PPART

    > done
    done[ADJ]+ADJ
    do[V]+V+PPART
    do+V+PPART

    > goose
    goose[N]+N

    > geese
    goose[N]+N+PL
    goose+N+PL
    
## Performance
This implementation is about twice as fast as a [hfst-lookup](https://github.com/hfst/hfst/wiki/HfstLookUp) with `hfstol` format.
Also the memory usage is about half (or 2 third, depending on the transducer format).

## Encoding
The tool is able to operate in the following modes:
 - ASCII or 1byte
   - including any of the ISO-8859 code pages
 - UTF-8
 - USC-2, wich is fixed 2 byte characters (U+0000 - U+D7FF)
 - UTF-16
   - this is the native Windows encoding
   
If you have a transducer file in any of the above formats, then you can read and produce similarly encoded inputs and outputs this way:

    attol -e [0-5] transducer.txt -i input.txt -o output.txt
   
Mind that for the multi-byte formats BOM is not supported. You have to save/read your files without BOM.

Windows `\r\n` end-of-line is handled during read, but output has  always Linux `\n` line terminators.
