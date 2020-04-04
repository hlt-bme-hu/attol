#include <iostream>
#include <stdio.h>
#include <string>

#include "ArgParser.h"
#include "attol/Transducer.h"
#include "attol/Char.h"
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

double time_limit = 0.0;
size_t max_depth = 0, max_results = 0;
int print_type = 305;
bool bom = false, binary_input = false, binary_output = false;
std::string dump_filename;
size_t field_separator = '\t';

template<attol::Encoding enc, attol::FlagStrategy strategy>
void do_main(std::string transducer_filename, FILE* input, FILE* output)
{
    typedef attol::Transducer<enc, 32> Transducer;
    typedef typename Transducer::CharType CharType;

    Transducer t;
    {
        FILE* f = fopen(transducer_filename.c_str(), "rb");
        if (!f)
            throw attol::Error("Cannot open \"", transducer_filename, "\"!");
        std::cerr << "Reading transducer \"" << transducer_filename << "\" ... " << std::endl;
        if (binary_input)
        {
            if (!t.ReadBinary(f))
                throw attol::Error("Binary file \"", transducer_filename, "\" is an invalid transducer!");
            std::cerr << "Memory (bytes): " << t.GetAllocatedMemory() << std::endl;
        }
        else
        {
            if (bom && !attol::CheckBom<enc>(f))
                throw attol::Error("File \"", transducer_filename, "\" with encoding ", int(enc), " does not match BOM!");
            t.Read(f, CharType(field_separator));
            std::cerr << "States: " << t.GetNumberOfStates() <<
                "\nTransitions: " << t.GetNumberOfTransitions() << std::endl;
        }
        fclose(f);
    }
    if (!dump_filename.empty())
    {
        FILE* f = fopen(dump_filename.c_str(), "wb");
        if (!f)
            throw attol::Error("Cannot open \"", dump_filename, "\" for writing!");
        std::cerr << "Writing transducer \"" << dump_filename << "\" ... ";
        std::cerr.flush();
        if (!(binary_output ? t.WriteBinary(f) : t.Write(f, CharType(field_separator))))
            throw attol::Error("Cannot write transducer into \"", dump_filename, "\"!");
        fclose(f);
        std::cerr << "done" << std::endl;
    }

    if (bom && !attol::CheckBom<enc>(input))
        throw attol::Error("Input file with encoding ", int(enc), " does not match BOM!");
    if (bom && !attol::WriteBom<enc>(output))
        throw attol::Error("Cannot write BOM to output with encoding", int(enc), "!");

    t.max_depth = max_depth;
    t.max_results = max_results;
    t.time_limit = time_limit;

    attol::PrintFunction<enc, 32> printf(print_type, output);
    t.resulthandler = printf.GetF();
    std::basic_string<typename Transducer::CharType> word;
    CharType c;
    while (!feof(input))
    {
        word.clear();
        while (fread(&c, sizeof(c), 1, input) == 1 && c != '\n')
        {
            word.push_back(c);
        }
        if (!word.empty() && word.back() == '\r')
            word.pop_back();
        printf.Reset(word.c_str());
        t.template Lookup<strategy, true>(word.c_str());
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
    int encoding = attol::UTF8;
    int flag_strategy = attol::OBEY;
    {
        arg::Parser<> parser("AT&T Optimized Lookup\n"
                            "Author: Gabor Borbely, Contact: borbely@math.bme.hu",
                            { "-h", "--help" }, std::cout, std::cerr, 
        "As you can see, the --print bitfield is a bit odd. "
        "This is because it is a signed bitfield where each flag can have actually 3 states: "
        "on, off and negative. To store these options, we need 2 bits per field.\n"
        "\t00\t'off' or 0\n"
        "\t01\t'on' or 1\n"
        "\t10\tunused (but it would be -2)\n"
        "\t11\t'negative' or -'on' or simply -1\n"
        "The print options follows:\n"
        "\t0b00000001\t1\tfirst field on\n"
        "\t0b00000100\t4\tsecond field on\n"
        "\t0b00001100\t12\tsecond field negative\n"
        "\t0b00010000\t16\tthird field on\n"
        "\t0b00110000\t48\tthird field negative\n\tand so on...\n"
        "In general, if a field has n bits of place then "
        "that field is represented as a two's complement binary on n bits. "
        "Such a field can hold values from -2^(n-1) to 2^(n-1)-1.\n"
        "This type of bitfield is used in the flag diacritics as well."
                            , 80);

        parser.AddArg(transducer_filename, {}, 
                        "AT&T (text) format transducer filename", "filename");
        parser.AddFlag(binary_input, { "-bi", "--binary-input" },
            "Read the transducer in a binary format");

        parser.AddArg(input_filename, { "-i", "--input" },
                        "input file to analyze, stdin if empty", "filename");
        parser.AddArg(output_filename, { "-o", "--output" },
                        "output file, stdout if empty", "filename");
        
        parser.AddArg(dump_filename, {"-w", "--write", "--dump"},
            "Write the transducer to file after loading, can be used for conversion. \n"
            "Don't convert the transducer if this argument is empty.", "filename");
        parser.AddFlag(binary_output, { "-bo", "--binary-output" },
            "Write the transducer in a binary format");

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
        
        parser.AddArg(print_type, { "-p", "--print" },
            attol::ToStr("What to print about the analyses\nbitfield of the following values:\n",
                1,   ": original word\n",
                4, ": input segmented\n",
                12, ": input segmented, flag diacritics removed\n",
                16, ": output tape\n",
                48, ": output tape with special symbols resolved\n",
                64, ": transition IDs along the path\n",
                256, ": weights of each path in log-probablities\n",
                768, ": weights of each path in relative percent\n",
                "0 means don't print anything"));
        
        parser.AddArg(encoding, { "-e", "--enc", "--encoding" },
            attol::ToStr("encoding of the transducer and also the input/output\n",
                attol::ASCII, ": ASCII\n",
                attol::OCTET, ": OCTET\tfixed 1 byte\n"
                                "\t\tuse this for any of the extended ASCII character tables\n",
                                "\t\tfor example ISO-8859 encodings\n",
                                "\t\t(actually it's the same as ASCII)\n",
                attol::UTF8, ": UTF-8\tsame as the above two with two notable differences:\n",
                                "\t\tBOM may be used\n",
                                "\t\tOne character is calculated according to continuation bytes, "
                                "character != byte\n",
                attol::UCS2, ": UCS-2\tendianness depends on your CPU\n",
                attol::UTF16, ": UTF16\tendianness depends on your CPU\n",
                attol::UTF32, ": UTF32\tendianness depends on your CPU"),
            "", std::vector<int>({ attol::ASCII, attol::CP, attol::UTF8, attol::UCS2, attol::UTF16, attol::UTF32 }));
        
        attol::BOM<attol::UTF16> binarybom;
        char stringbom[10];
        snprintf(stringbom, 10, "0x%02hhX 0x%02hhX", binarybom.bytes[0], binarybom.bytes[1]);
        parser.AddFlag(bom, { "-bom", "-BOM", "--bom", "--BOM" }, 
            attol::ToStr("Use Byte Order Mark in front of all input/output and transducer file.\n",
                "Your endianness: ", stringbom));

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
    mainf(transducer_filename, input, output);
    return 0;
}
catch (const std::exception& e)
{
    std::cerr << e.what() << std::endl;
    return 1;
}
}
