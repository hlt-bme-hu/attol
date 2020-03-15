# AT&T Optimized Lookup
Given an [AT&amp;T text format](https://github.com/hfst/hfst/wiki/HfstFst2Txt) transducer, this tool performs lookup.

## Compile
All you need is [cmake](https://cmake.org/) (>=3.1) and a c++11 compliant compiler (or Visual Studio 17, or later)

With gcc and make:

    cmake -DCMAKE_BUILD_TYPE=Release .
    make
 
With Visual C++
 
    cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release .
    nmake

## Usage
 - positional argument: AT&T (text) format transducer filename
 - optional arguments:
<PRE>
    -i --input 'filename' default: ""
        input file to analyze, stdin if empty
    -o --output 'filename' default: ""
        output file, stdout if empty
    -t --time 'd' default: 0
        time limit (in seconds) when not to search further
        unlimited if set to 0
    -n 'm' default: 0
        max number of results for one word
        unlimited if set to 0
    -d --depth 'm' default: 0
        maximum depth to go down during lookup
        unlimited if set to 0
    -fs --fs --tab -ts --ts 'unicode decimal' default: 9
        field separator, character between columns of transducer file
        It has to be 1 code-unit wide, meaning that in UTF8 and UTF16 you cannot use characters
        above U+007F and U+D7FF respectively.
    -f --flag 'i' default: 1
        how to treat the flag diacritics
        0: ignore (off)
        1: obey
        2: negative, return only those paths that were invalid flag-wise but correct analysis
        otherwise.
        possible values: 0 1 2
    -e --enc --encoding 'i' default: 2
        encoding of the transducer and also the input/output
        0: ASCII
        1: OCTET    fixed 1 byte
                use this for any of the extended ASCII character tables
                for example ISO-8859 encodings
                (actually it's the same as ASCII)
        2: UTF-8    same as the above two with two notable differences:
                BOM may be used
                One character is calculated according to continuation bytes
        3: UCS-2    endianness depends on your CPU
        4: UTF16    endianness depends on your CPU
        5: UTF32    endianness depends on your CPU
        possible values: 0 1 2 3 4 5
    -bom -BOM --bom --BOM 'b' default: false
        Use Byte Order Mark in front of all input/output and transducer file.
</PRE>
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
Also the peak memory usage is about half (or 2 third, depending on the transducer format).

## Encoding
The tool is able to operate in the following modes:
 - ASCII or OCTET (1byte)
   - including any of the ISO-8859 code pages
 - UTF-8
 - USC-2, which is fixed 2 byte characters (U+0000 - U+D7FF)
 - UTF-16
 - UTF-32, fixed 4 byte characters

You can choose optional BOM for multi-byte encodings (USC-2, UTF-16 and UTF-32)
 - Mind that endianness won't be converted in those cases
 - Meaning that file endianness (for example UTF16-LE or ETF16-BE) should match CPU endianness
   - UTF16-LE is the Windows native
 - Although, endianness is checked if BOM is available.
 - It is advised to use BOM for these encodings

If you have a transducer file in any of the above formats, then you can read and produce similarly encoded inputs and outputs this way:

    attol -e [0-5] [-bom] transducer.txt -i input.txt -o output.txt
## EOL
Windows `\r\n` end-of-line is handled during read, but output is always in Linux `\n` format.
