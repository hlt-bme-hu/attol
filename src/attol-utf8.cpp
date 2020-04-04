#include <iostream>
#include <string>

#include "ArgParser.h"
#include "attol/Transducer.h"
#include "attol/Char.h"

int main(int argc, const char** argv)
{
    std::string transducer_filename;
    double time_limit = 0.0;
    size_t max_depth = 0, max_results = 0;
    bool binary_input = false;
    std::streamsize maxinputsize = 4096;
    int flag_strategy = attol::OBEY;
    {
        arg::Parser<> parser("AT&T Optimized Lookup\n"
                            "Author: Gabor Borbely, Contact: borbely@math.bme.hu",
                            { "-h", "--help" }, std::cout, std::cerr, "", 80);

        parser.AddArg(transducer_filename, {}, 
                        "AT&T (text) format transducer filename", "filename");
        parser.AddFlag(binary_input, { "-bi", "--binary-input" },
            "Read the transducer in a binary format");

        //parser.AddArg(input_filename, { "-i", "--input" },
        //                "input file to analyze, stdin if empty", "filename");
        //parser.AddArg(output_filename, { "-o", "--output" },
        //                "output file, stdout if empty", "filename");
        
        parser.AddArg(time_limit, { "-t", "--time" }, 
                        "time limit (in seconds) when not to search further\n"
                        "unlimited if set to 0");
        parser.AddArg(max_results, { "-n" },
                        "max number of results for one word\n"
                        "unlimited if set to 0");
        parser.AddArg(max_depth, { "-d", "--depth" },
                        "maximum depth to go down during lookup\n"
                        "unlimited if set to 0");
        parser.AddArg(maxinputsize, { "-s", "--size" },
                        "maximum length of input string");

        parser.AddArg(flag_strategy, { "-f", "--flag" }, 
            attol::ToStr("how to treat the flag diacritics\n",
                attol::IGNORE, ": ignore (off)\n",
                attol::OBEY, ": obey\n", 
                attol::NEGATIVE, ": negative, return only those paths that were invalid flag-wise but correct analysis otherwise."),
            "", std::vector<int>({ attol::OBEY, attol::IGNORE, attol::NEGATIVE }));

        parser.Do(argc, argv);
    }
try{
    std::ios_base::sync_with_stdio(false);

    attol::Transducer<attol::UTF8, 32> t;
    FILE* f = fopen(transducer_filename.c_str(), "rb");
    if (!f)
        throw attol::Error("Cannot open \"", transducer_filename, "\"!");
    if (binary_input)
    {
        if (!t.ReadBinary(f))
            throw attol::Error("Binary file \"", transducer_filename, "\" is an invalid transducer!");
    }
    else
    {
        t.Read(f);
    }
    fclose(f);

    t.max_depth = max_depth;
    t.max_results = max_results;
    t.time_limit = time_limit;

    auto lookup = &decltype(t)::Lookup<attol::OBEY, false>;
    switch (flag_strategy)
    {
    case attol::FlagStrategy::IGNORE:
        lookup = &decltype(t)::Lookup<attol::IGNORE, false>;
        break;
    case attol::FlagStrategy::NEGATIVE:
        lookup = &decltype(t)::Lookup<attol::NEGATIVE, false>;
        break;
    }
    std::string word;
    char c;
    t.resulthandler = [&word](const decltype(t)::Path& path)
    {
        fputs(word.c_str(), stdout);
        fputc('\t', stdout);
        for (const auto& v : path)
        {
            fputs(v.InterpretOutput(), stdout);
        }
        fputc('\n', stdout);
    };
    while (!feof(stdin))
    {
        word.clear();
        while (fread(&c, sizeof(c), 1, stdin) == 1 && c != '\n')
        {
            word.push_back(c);
        }
        (t.*lookup)(word.data());
    }
    return 0;
}
catch (const std::exception& e)
{
    std::cerr << e.what() << std::endl;
    return 1;
}
}
