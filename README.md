# AT&T Optimized Lookup
Given an [AT&amp;T text format](https://github.com/hfst/hfst/wiki/HfstFst2Txt) transducer, this tool performs lookup.

## Compile
All you need is [cmake](https://cmake.org/) (>=3.1) and a c++11 compliant compiler or Visual Studio 15 (vc14, v140, `_MSC_VER 1900`), or later.

* With gcc and make:

      cmake -DCMAKE_BUILD_TYPE=Release .
      make
 
* With Visual C++ and NMake
 
      cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release .
      nmake

* With Visual Studio 2019

      cmake -G "Visual Studio 16 2019" -A x64 .
      start attol.sln
    
## Usage

### Help

    attol [-h|--help]
    
### Short arguments

    attol [-bi] 'filename' [-bo] [-w 'filename'] 
          [-i 'filename'] [-o 'filename']
          [-t 'double'] [-n 'size_t'] [-d 'size_t'] 
          [-fs 'unicode decimal'] [-f 'int'] [-p 'int'] 
          [-e 'int'] [-bom]

### Long arguments
 - positional argument: AT&T (text or binary) format transducer filename
 - optional arguments:

        -bi --binary-input 'bool' default: false
                Read the transducer in a binary format
        -i --input 'filename' default: ""
                input file to analyze, stdin if empty
        -o --output 'filename' default: ""
                output file, stdout if empty
        -w --write --dump 'filename' default: ""
                Write the transducer to file after loading, can be used for conversion.
                Don't convert the transducer if this argument is empty.
        -bo --binary-output 'bool' default: false
                Write the transducer in a binary format
        -t --time 'double' default: 0
                time limit (in seconds) when not to search further
                unlimited if set to 0
        -n 'size_t' default: 0
                max number of results for one word
                unlimited if set to 0
        -d --depth 'size_t' default: 0
                maximum depth to go down during lookup
                unlimited if set to 0
        -fs --fs --tab -ts --ts 'unicode decimal' default: 9
                field separator, character between columns of transducer file
                It has to be 1 code-unit wide, meaning that in UTF8 and UTF16 you cannot use characters
                above U+007F and U+D7FF respectively.
        -f --flag 'int' default: 1
                how to treat the flag diacritics
                0: ignore (off)
                1: obey
                2: negative, return only those paths that were invalid flag-wise but correct analysis
                otherwise.
                possible values: 0 1 2
        -p --print 'int' default: 305
                What to print about the analyses
                bitfield of the following values:
                1: original word
                4: input segmented
                12: input segmented, flag diacritics removed
                16: output tape
                48: output tape with special symbols resolved
                64: transition IDs along the path
                256: weights of each path in log-probablities
                768: weights of each path in relative percent
                0 means don't print anything
        -e --enc --encoding 'int' default: 2
                encoding of the transducer and also the input/output
                0: ASCII
                1: OCTET        fixed 1 byte
                                use this for any of the extended ASCII character tables
                                for example ISO-8859 encodings
                                (actually it's the same as ASCII)
                2: UTF-8        same as the above two with two notable differences:
                                BOM may be used
                                One character is calculated according to continuation bytes, character != byte
                3: UCS-2        endianness depends on your CPU
                4: UTF16        endianness depends on your CPU
                5: UTF32        endianness depends on your CPU
                possible values: 0 1 2 3 4 5
        -bom -BOM --bom --BOM 'bool' default: false
                Use Byte Order Mark in front of all input/output and transducer file.

### Example

    > attol english.hfst.txt
    Transducer states: 466865
    Transitions: 1340917
    Memory (bytes): 33639656
    > be
    be[V]+V+INF     5.0791
    be[V]+V+PRES    9.7168
    be[N]+N 18.4541

    > plt
    ?

    > gone
    gone[ADJ]+ADJ   18.4541
    go[V]+V+PPART   8.63184
    go[V]+V+PAST    15.5088
    go+V+PPART      18.4541

    > geese
    goose[N]+N+PL   12.4775
    goose+N+PL      18.4541

## Performance
This implementation is faster than [hfst-optimized-lookup](https://github.com/hfst/hfst/wiki/HfstOptimizedLookup).
Also the peak memory usage is about half of `hfst`, or even less, depending on the transducer format.

* Measured against `hfst-optimized-lookup 1.2` on various CPUs.
  * Pentium measurements were on Windows 10
  * other on Ubuntu 16 or 18
* Only peak memory usage is reported (if available)
* The transducers were converted into optimized (binary) format beforehand. This eliminates loading time as much as possible.
  * In case of hfst, this means hfstol format
  * For attol, a custom binary format is used.

### English
* data: lowercase words of [UMBC](https://ebiquity.umbc.edu/resource/html/id/351), first 1M most frequent types.
* morphology: https://sourceforge.net/projects/hfst/files/resources/morphological-transducers/

| tool | CPU (single thread) | time (sec)| memory (KiB) |
| -----| -----------         | -----:    |-----:        |
|hfst |Pentium N4200|36.4| |
|attol|Pentium N4200|18.45| |
|hfst |Celeron J1800|23.81|159124 |
|attol|Celeron J1800|19.65|29112 |
|hfst |Xeon E5520   |12.54|159552 |
|attol|Xeon E5520   |11.77|29488 |
|hfst |Core i7-4790K|5.41|164980 |
|attol|Core i7-4790K|4.93|29720 |

### Hungarian
* data: lowercase words of [MNSZ2](http://clara.nytud.hu/mnsz2-dev/), first 1M most frequent types.
* morphology: https://github.com/dlt-rilmta/emMorph

| tool | CPU (single thread) | time (sec)| memory (KiB) |
| -----| -----------         | -----:    |-----:        |
|hfst|Pentium N4200|63.49||
|attol|Pentium N4200|23.75||
|hfst|Celeron J1800|39.58|262252|
|attol|Celeron J1800|21.45|39116|
|hfst|Xeon E5520|18.71|262364|
|attol|Xeon E5520|12.22|39192|
|hfst|Core i7-4790K|9.61|267648|
|attol|Core i7-4790K|5.58|39592|

## Encoding
The tool is able to operate in the following modes:
 - ASCII or OCTET (1byte)
   - including any of the ISO-8859 code pages
 - UTF-8
 - USC-2, which is fixed 2 byte characters (U+0000 - U+D7FF)
 - UTF-16
 - UTF-32, fixed 4 byte characters

You can choose optional BOM for multi-byte encodings (UTF-8, USC-2, UTF-16 and UTF-32)
 - Mind that endianness won't be converted in those cases
 - Meaning that file endianness (for example UTF16-LE or UTF16-BE) should match CPU endianness
   - UTF16-LE is the Windows native
 - Although, endianness is checked if BOM is available.
 - It is advised to use BOM for USC-2, UTF-16 and UTF-32 encodings
 - BOM does nothing for ASCII and OCTET formats.
 
If you have a transducer file in any of the above formats, then you can read and write similarly encoded inputs and outputs. For example:

    ./attol --enc 4 --bom data/flags.win.att -i flag.test.win.txt -o output_UTF16-LE.txt

## EOL
Windows `\r\n` end-of-line is handled during read, but output is always in Linux `\n` format.
