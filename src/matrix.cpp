#include <iostream>
#include <stdio.h>
#include <string>
#include <vector>
#include <algorithm>

#include "ArgParser.h"
#include "attol/Transducer.h"
#include "attol/Print.h"

typedef attol::Transducer<attol::UTF8, 32> Transducer;

////! kind-of a vector implementation of set
//template<typename Value, typename Allocator>
//Value& SortedInsert(std::vector<Value, Allocator>& vec, const Value& value)
//{
//    auto where = std::lower_bound(vec.begin(), vec.end(), value);
//    if (where == vec.end() || *where != value)
//        return *vec.emplace(where, value);
//    else
//        return *where;
//}

template<typename Ty1, typename Ty2>
struct Lesser
{
    bool operator()(const std::pair<Ty1, Ty2>& one, const std::pair<Ty1, Ty2>& other)const
    {
        return one.first < other.first;
    }
};

//template<class Vec1, class Iterator2, class Compare>
//void Intersect(Vec1& v1, Iterator2 first2, Iterator2 last2, const Compare& comp)
//{
//    auto first1 = v1.begin();
//    auto last1 = v1.end();
//
//    while (first1 != last1 && first2 != last2)
//    {
//        if (comp.less(*first1, first2))
//        {   // remove from first
//            first1 = v1.erase(first1);
//            // iterator validity!
//            last1 = v1.end();
//        }
//        else if (comp.greater(*first1, first2))
//        {   // skip from second
//            ++first2;
//        }
//        else
//        {   // leave it
//            ++first1;
//            ++first2;
//        }
//    }
//    // remove the rest, if any
//    v1.erase(first1, last1);
//}
//template<class Vec1, class Iterator2, class Compare>
//void Subtract(Vec1& v1, Iterator2 first2, Iterator2 last2, const Compare& comp)
//{
//    auto first1 = v1.begin();
//    auto last1 = v1.end();
//
//    while (first1 != last1 && first2 != last2)
//    {
//        if (comp(*first1, *first2))
//        {   // keep it
//            ++first1;
//        }
//        else if (comp(*first2, *first1))
//        {   // skip from second
//            ++first2;
//        }
//        else
//        {   // remove from first
//            first1 = v1.erase(first1);
//            // iterator validity!
//            last1 = v1.end();
//            ++first2;
//        }
//    }
//}
//
//template<class Vec1, class Iterator2>
//void Subtract(Vec1& v1, Iterator2 first2, Iterator2 last2)
//{
//    Subtract<Vec1, Iterator2, std::less<typename Vec1::value_type>>(v1, first2, last2, std::less<typename Vec1::value_type>());
//}
//
//template<class Vec1, class Iterator2, class Compare>
//void Union(Vec1& v1, Iterator2 first2, Iterator2 last2, Compare comp)
//{
//    static_assert(std::is_same<typename Vec1::value_type, typename Iterator2::value_type>::value, "the two iterables should have the same value_type!");
//    auto first1 = v1.begin();
//    auto last1 = v1.end();
//
//    while (first1 != last1 && first2 != last2)
//    {
//        if (comp(*first1, *first2))
//        {
//            ++first1;
//        }
//        else if (comp(*first2, *first1))
//        {
//            first1 = v1.insert(first1, *first2);
//            // iterator validity!
//            last1 = v1.end();
//            ++first1;
//            ++first2;
//        }
//        else
//        {
//            ++first1;
//            ++first2;
//        }
//    }
//    // add the rest, if any
//    v1.insert(v1.end(), first2, last2);
//}
//
//template<class Vec1, class Iterator2>
//void Union(Vec1& v1, Iterator2 first2, Iterator2 last2)
//{
//    Union<Vec1, Iterator2, std::less<typename Vec1::value_type>>(v1, first2, last2, std::less<typename Vec1::value_type>());
//}

//! kind-of a vector implementation of map
template<typename Key, typename Value, typename Allocator>
Value& SortedInsert(std::vector<std::pair<Key, Value>, Allocator>& vec, const Key& key)
{
    std::pair<Key, Value> pair;
    pair.first = key;
    pair.second = 0;
    auto where = std::lower_bound(vec.begin(), vec.end(), pair, Lesser<Key, Value>());
    if (where == vec.end() || where->first != key)
        return ++(vec.emplace(where, pair)->second);
    else
        return ++(where->second);
}

int main(int argc, const char** argv)
{
    std::string transducer_filename, input_filename, output_filename;
    double time_limit = 0.0;
    size_t max_depth = 0, max_results = 0;

    {
        arg::Parser<> parser("Collecting structural matrices for weighted FSA learning\n"
                             "Author: Gabor Borbely, Contact: borbely@math.bme.hu",
                             { "-h", "--help" }, std::cout, std::cerr, "", 80);

        parser.AddArg(transducer_filename, {}, 
                        "AT&T (text) format transducer filename", "filename");
            
        parser.AddArg(input_filename, { "-i", "--input" },
                        "input file to analyze, stdin if empty", "filename");
        parser.AddArg(output_filename, { "-o", "--output" },
                        "output model name", "prefix");
        
        parser.AddArg(time_limit, { "-t", "--time" }, 
                        "time limit (in seconds) when not to search further\n"
                        "unlimited if set to 0");
        parser.AddArg(max_results, { "-n" },
                        "max number of results for one word\n"
                        "unlimited if set to 0");
        parser.AddArg(max_depth, { "-d", "--depth" },
                        "maximum depth to go down during lookup\n"
                        "unlimited if set to 0");
        
        parser.Do(argc, argv);
    }
try{
    std::ios_base::sync_with_stdio(false);
    Transducer t;
    FILE* input = input_filename.empty() ? stdin : fopen(input_filename.c_str(), "rb");
    if (!input)
        throw attol::Error("Unable to open input file \"", input_filename, "\"!");
/*
    FILE* output = output_filename.empty() ? stdout : fopen(output_filename.c_str(), "wb");
    if (!output)
        throw attol::Error("Unable to open output file \"", output_filename, "\"!");
*/
    {
    FILE* transducer_file = fopen(transducer_filename.c_str(), "rb");
    if (!transducer_file)
        throw attol::Error("Cannot open \"", transducer_filename, "\"!");

    t.Read(transducer_file, '\t');
    std::cerr << "Transducer states: " << t.GetNumberOfStates() <<
        "\nTransitions: " << t.GetNumberOfTransitions() <<
        "\nMemory (bytes): " << t.GetAllocatedMemory() << std::endl;
    fclose(transducer_file);
    }
    t.max_depth = max_depth;
    t.max_results = max_results;
    t.time_limit = time_limit;

    std::vector<std::pair<Transducer::Index, Transducer::Index>> Pline;

    std::string filename = output_filename + ".prob";
    FILE* p_file = fopen(filename.c_str(), "w");
    if (!p_file)
        throw attol::Error("Cannot open \"", filename, "\" for writing!");

    filename = output_filename + ".unrecognized";
    FILE* pp_file = fopen(filename.c_str(), "w");
    if (!pp_file)
        throw attol::Error("Cannot open \"", filename, "\" for writing!");

    filename = output_filename + ".M";
    FILE* M_file = fopen(filename.c_str(), "w");
    if (!M_file)
        throw attol::Error("Cannot open \"", filename, "\" for writing!");
    
    filename = output_filename + ".P";
    FILE* P_file = fopen(filename.c_str(), "w");
    if (!P_file)
        throw attol::Error("Cannot open \"", filename, "\" for writing!");

    Transducer::Index number_of_analyses = 0;
    Transducer::Index number_of_paths = 0;
    Transducer::Index number_of_input = 0;
    Transducer::Index number_of_recognized = 0;
    t.resulthandler = [&](const Transducer::Path& path)
    {
        Pline.clear();
        for (const auto v : path)
            SortedInsert(Pline, v.GetId());
        for (const auto& p : Pline)
            fprintf(P_file, "%u %u ", p.first, p.second);
        fputc('\n', P_file);
        fprintf(M_file, "%u %u ", number_of_paths, 1);
        ++number_of_paths;
        ++number_of_analyses;
    };

    Transducer::string word;
    Transducer::CharType c;
    size_t tab_pos;
    Transducer::Float w, total_w = 0, recognized_w = 0;
    while (!feof(input))
    {
        word.clear();
        while (fread(&c, sizeof(c), 1, input) == 1 && c != '\n')
        {
            word.push_back(c);
        }
        if (!word.empty() && word.back() == '\r')
            word.pop_back();
        tab_pos = word.find('\t');
        if (tab_pos == std::string::npos)
        {
            w = 1;
        }
        else
        {
            word[tab_pos++] = '\0';
            w = std::atof(word.data() + tab_pos);
        }
        total_w += w;
        ++number_of_input;
        number_of_analyses = 0;
        t.Lookup<>(word.c_str());
        if (number_of_analyses > 0)
        {
            fprintf(p_file, "%g\n", w);
            fputc('\n', M_file);
            ++number_of_recognized;
            recognized_w += w;
        }
        else
        {
            fprintf(pp_file, "%g\n", w);
        }
        if ((number_of_input & ((1 << 12) - 1)) == 0)
            fprintf(stderr, "\r%u words processed, "
                            "%6.2f%% of them were recognized,"
                            "probability of recognition is %6.2f%%",
                            number_of_input,
                            (100.0f*number_of_recognized)/number_of_input,
                            (100.0f*recognized_w) / total_w);
    }
    fprintf(stderr, "\r%u words processed, "
                    "%6.2f%% of them were recognized,"
                    "probability of recognition is %6.2f%%\n",
                    number_of_input,
                    (100.0f*number_of_recognized)/number_of_input,
                    (100.0f*recognized_w) / total_w);
    return 0;
}
catch (const std::exception& e)
{
    std::cerr << e.what() << std::endl;
    return 1;
}
}
