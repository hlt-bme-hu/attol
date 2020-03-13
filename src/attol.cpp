#include <iostream>
#include <stdio.h>
#include <string>

#include "ArgParser.h"
#include "attol/Transducer.h"

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
    size_t max_results, size_t max_depth, double max_time, int )
{
    typedef attol::Transducer<enc> Transducer;
    typedef typename Transducer::CharType CharType;
    FILE* f = fopen(transducer_filename.c_str(), "rb");
    if (!f)
        throw attol::Error("Cannot open \"", transducer_filename, "\"!");
    Transducer t(f);
    fclose(f);

    std::cerr << "Transducer states: " << t.GetNumberOfStates() <<
        "\ntransitions: " << t.GetNumberOfTransitions() <<
        "\nmemory: " << t.GetAllocatedMemory() << "bytes" << std::endl;

    t.max_depth = max_depth;
    t.max_results = max_results;
    t.time_limit = max_time;

    bool has_analyses = false;
    t.resulthandler = [&has_analyses, output](const typename Transducer::Path& path)
    {
        static CharType c;
        for (const auto& v : path)
        {
            fwrite(v.GetOutput(), sizeof(c), std::char_traits<CharType>::length(v.GetOutput()), output);
        }
        c = '\n';
        fwrite(&c, sizeof(c), 1, output);
        has_analyses = true;
    };

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
        has_analyses = false;
        t.template Lookup<strategy>(word.c_str());
        if (!has_analyses)
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
    int print = 1, encoding = attol::UTF8;
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
        
        parser.AddArg(flag_strategy, { "-f", "--flag" }, 
            attol::ToStr("how to treat the flag diacritics\n",
                attol::IGNORE, ": ignore (off)\n",
                attol::OBEY, ": obey\n", 
                attol::NEGATIVE, ": negative, return only those paths that were invalid flag-wise but correct analysis otherwise."),
            "", std::vector<int>({ attol::OBEY, attol::IGNORE, attol::NEGATIVE }));
        
        //parser.AddArg(print, { "-p", "--print" },
        //    attol::ToStr("What to print about the analyses\n",
        //        0, ": output tape result\n",
        //        1, ": output tape result with weights\n",
        //        2, ": transition IDs along the path\n",
        //        3, ": transition IDs along the path with weights"),
        //    "", std::vector<int>({ 0,1,2,3 }));
  
        parser.AddArg(encoding, { "-e", "--enc", "--encoding" },
            attol::ToStr("encoding of the transducer and also the input/output\n",
                attol::ASCII, ": ASCII\n",
                attol::CP, ": fixed one byte characters, use this for any ISO-8859 character table (same as ASCII.)\n",
                attol::UTF8, ": UTF8\n",
                attol::UCS2, ": UCS2,\tendianness depends on your CPU\n",
                attol::UTF16, ": UTF16,\tendianness depends on your CPU\n",
                attol::UTF32, ": UTF32,\tendianness depends on your CPU"),
            "", std::vector<int>({ attol::ASCII, attol::CP, attol::UTF8, attol::UCS2, attol::UTF16 }));

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
        //case attol::UTF32:
        //    mainf = do_main<attol::UTF32,   attol::NEGATIVE>;
        //    break;
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
        //case attol::UTF32:
        //    mainf = do_main<attol::UTF32,   attol::IGNORE>;
        //    break;
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
        //case attol::UTF32:
        //    mainf = do_main<attol::UTF32,   attol::OBEY>;
        //    break;
        default:
            mainf = do_main<attol::UTF8,    attol::OBEY>;
            break;
        } break;
    }
    mainf(transducer_filename, input, output, max_results, max_depth, time_limit, print);
    return 0;
}
catch (const std::exception& e)
{
    std::cerr << e.what() << std::endl;
    return 1;
}
}
