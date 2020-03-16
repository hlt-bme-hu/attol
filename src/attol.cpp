#include <iostream>
#include <stdio.h>
#include <string>

#include "ArgParser.h"
#include "attol/Transducer.h"
#include "attol/char.h"
#include "attol/Print.h"

#ifdef WIN32
# include <io.h>
# include <fcntl.h>
void SetBinary()
{
    auto result = _setmode(_fileno(stdin), _O_BINARY);
    if (result == -1)
        throw attol::Error("unable to set stdin binary mode!");
    result = _setmode(_fileno(stdout), _O_BINARY);
    if (result == -1)
        throw attol::Error("unable to set stdout binary mode!");
}
#else
void SetBinary()
{
}
#endif

template<attol::Encoding enc, attol::FlagStrategy strategy>
void do_main(std::string transducer_filename, FILE* input, FILE* output,
    size_t max_results, size_t max_depth, double max_time, int print, bool bom, size_t fs)
{
    typedef attol::Transducer<enc, 32> Transducer;
    typedef typename Transducer::CharType CharType;

    FILE* f = fopen(transducer_filename.c_str(), "rb");
    if (!f)
        throw attol::Error("Cannot open \"", transducer_filename, "\"!");
    if (bom && !attol::CheckBom<enc>(f))
        throw attol::Error("File \"", transducer_filename, "\" with encoding ", int(enc), " does not match BOM!");
    Transducer t(f, CharType(fs));
    fclose(f);

    std::cerr << "Transducer states: " << t.GetNumberOfStates() <<
        "\nTransitions: " << t.GetNumberOfTransitions() <<
        "\nMemory (bytes): " << t.GetAllocatedMemory() << std::endl;

    if (bom && !attol::CheckBom<enc>(input))
        throw attol::Error("Input file with encoding ", int(enc), " does not match BOM!");
    if (bom && !attol::WriteBom<enc>(output))
        throw attol::Error("Cannot write BOM to output with encoding", int(enc), "!");

    t.max_depth = max_depth;
    t.max_results = max_results;
    t.time_limit = max_time;

    attol::PrintFunction<enc, 32> printf(print, output);
    t.resulthandler = printf.GetF();

    std::basic_string<typename Transducer::CharType> word;
    CharType c;
    while (!feof(input))
    {
        word.clear();
        while (fread(&c, sizeof(c), 1, input) == 1 && c != '\n')
        {
            if (c == '\r')
                continue;
            word.push_back(c);
        }
        printf.Reset();
        t.template Lookup<strategy>(word.c_str());
        if (!printf.Succeeded())
        {
            c = '?';
            fwrite(&c, sizeof(c), 1, output);
            c = '\n';
            fwrite(&c, sizeof(c), 1, output);
        }
        c = '\n';
        fwrite(&c, sizeof(c), 1, output);
    }
}

int main(int argc, const char** argv)
{
    std::string transducer_filename, input_filename, output_filename;
    double time_limit = 0.0;
    size_t max_depth = 0, max_results = 0;
    int print = 3, encoding = attol::UTF8;
    bool bom = false;
    size_t field_separator = '\t';

    int flag_strategy = attol::OBEY;
    {
        arg::Parser<> parser("AT&T Optimized Lookup\n"
                             "Author: Gabor Borbely, Contact: borbely@math.bme.hu",
                             { "-h", "--help" }, std::cout, std::cerr, "", 80);

        parser.AddArg(transducer_filename, {}, 
                        "AT&T (text) format transducer filename", "filename");
            
        parser.AddArg(input_filename, { "-i", "--input" },
                        "input file to analyze, stdin if empty", "filename");
        parser.AddArg(output_filename, { "-o", "--output" },
                        "output file, stdout if empty", "filename");
        
        parser.AddArg(time_limit, { "-t", "--time" }, 
                        "time limit (in seconds) when not to search further\n"
                        "unlimited if set to 0");
        parser.AddArg(max_results, { "-n" },
                        "max number of results for one word\n"
                        "unlimited if set to 0");
        parser.AddArg(max_depth, { "-d", "--depth" },
                        "maximum depth to go down during lookup\n"
                        "unlimited if set to 0");
        
        parser.AddArg(field_separator, { "-fs", "--fs", "--tab", "-ts", "--ts" },
            "field separator, character between columns of transducer file\n"
            "It has to be 1 code-unit wide, meaning that in UTF8 and UTF16 you cannot use characters above "
            "U+007F and U+D7FF respectively.",
            "unicode decimal");

        parser.AddArg(flag_strategy, { "-f", "--flag" }, 
            attol::ToStr("how to treat the flag diacritics\n",
                attol::IGNORE, ": ignore (off)\n",
                attol::OBEY, ": obey\n", 
                attol::NEGATIVE, ": negative, return only those paths that were invalid flag-wise but correct analysis otherwise."),
            "", std::vector<int>({ attol::OBEY, attol::IGNORE, attol::NEGATIVE }));
        
        parser.AddArg(print, { "-p", "--print" },
            attol::ToStr("What to print about the analyses\n",
                0, ": output tape result\n",
                1, ": output tape result with weights\n",
                2, ": output tape with special symbols resolved\n",
                3, ": output tape with weights and special symbols resolved\n",
                4, ": transition IDs along the path\n",
                5, ": transition IDs along the path with weights\n",
                6, ": input segmented\n",
                7, ": input segmented with weights\n",
                8, ": input segmented but flag diacritics removed\n",
                9, ": input segmented with weights but flag diacritics removed\n",
                "If the print argument is none of the above, then a questionmark will be printed for every input word."));
  
        parser.AddArg(encoding, { "-e", "--enc", "--encoding" },
            attol::ToStr("encoding of the transducer and also the input/output\n",
                attol::ASCII, ": ASCII\n",
                attol::OCTET, ": OCTET\tfixed 1 byte\n"
                                "\t\tuse this for any of the extended ASCII character tables\n",
                                "\t\tfor example ISO-8859 encodings\n",
                                "\t\t(actually it's the same as ASCII)\n",
                attol::UTF8, ": UTF-8\tsame as the above two with two notable differences:\n",
                                "\t\tBOM may be used\n",
                                "\t\tOne character is calculated according to continuation bytes\n",
                attol::UCS2, ": UCS-2\tendianness depends on your CPU\n",
                attol::UTF16, ": UTF16\tendianness depends on your CPU\n",
                attol::UTF32, ": UTF32\tendianness depends on your CPU"),
            "", std::vector<int>({ attol::ASCII, attol::CP, attol::UTF8, attol::UCS2, attol::UTF16, attol::UTF32 }));

        parser.AddFlag(bom, { "-bom", "-BOM", "--bom", "--BOM" }, "Use Byte Order Mark in front of all input/output and transducer file.");

        parser.Do(argc, argv);
    }
try{
    std::ios_base::sync_with_stdio(false);
    SetBinary();
    
    FILE* input = input_filename.empty() ? stdin : fopen(input_filename.c_str(), "rb");
    if (!input)
        throw attol::Error("Unable to open input file \"", input_filename, "\"!");

    FILE* output = output_filename.empty() ? stdout : fopen(output_filename.c_str(), "wb");
    if (!output)
        throw attol::Error("Unable to open output file \"", output_filename, "\"!");

    auto mainf = do_main<attol::UTF8, attol::OBEY>;
    switch (attol::FlagStrategy(flag_strategy))
    {
    case attol::NEGATIVE:
        switch (encoding)
        {
        case attol::ASCII:
            mainf = do_main<attol::ASCII,   attol::NEGATIVE>;
            break;
        case attol::CP:
            mainf = do_main<attol::CP,      attol::NEGATIVE>;
            break;
        case attol::UTF16:
            mainf = do_main<attol::UTF16,   attol::NEGATIVE>;
            break;
        case attol::UCS2:
            mainf = do_main<attol::UCS2,    attol::NEGATIVE>;
            break;
        case attol::UTF32:
            mainf = do_main<attol::UTF32,   attol::NEGATIVE>;
            break;
        default:
            mainf = do_main<attol::UTF8,    attol::NEGATIVE>;
            break;
        } break;
    case attol::IGNORE:
        switch (encoding)
        {
        case attol::ASCII:
            mainf = do_main<attol::ASCII,   attol::IGNORE>;
            break;
        case attol::CP:
            mainf = do_main<attol::CP,      attol::IGNORE>;
            break;
        case attol::UTF16:
            mainf = do_main<attol::UTF16,   attol::IGNORE>;
            break;
        case attol::UCS2:
            mainf = do_main<attol::UCS2,    attol::IGNORE>;
            break;
        case attol::UTF32:
            mainf = do_main<attol::UTF32,   attol::IGNORE>;
            break;
        default:
            mainf = do_main<attol::UTF8,    attol::IGNORE>;
            break;
        } break;
    default:
        switch (encoding)
        {
        case attol::ASCII:
            mainf = do_main<attol::ASCII,   attol::OBEY>;
            break;
        case attol::CP:
            mainf = do_main<attol::CP,      attol::OBEY>;
            break;
        case attol::UTF16:
            mainf = do_main<attol::UTF16,   attol::OBEY>;
            break;
        case attol::UCS2:
            mainf = do_main<attol::UCS2,    attol::OBEY>;
            break;
        case attol::UTF32:
            mainf = do_main<attol::UTF32,   attol::OBEY>;
            break;
        default:
            mainf = do_main<attol::UTF8,    attol::OBEY>;
            break;
        } break;
    }
    mainf(transducer_filename, input, output,
            max_results, max_depth, time_limit,
            print, bom, field_separator);
    return 0;
}
catch (const std::exception& e)
{
    std::cerr << e.what() << std::endl;
    return 1;
}
}
