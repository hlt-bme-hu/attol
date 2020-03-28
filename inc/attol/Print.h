#include <stdio.h>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

#include "attol/Char.h"
#include "attol/Transducer.h"
#include "attol/FlagDiacritics.h"

namespace attol {

template<Encoding enc, size_t storageSize>
class PrintFunction
{
private:
    typedef Transducer<enc, storageSize> T;
    typename T::ResultHandler f;
    typedef std::char_traits<typename T::CharType> CharTraits;
    
    FILE* output;
    const typename T::CharType newline;
    const typename T::CharType input_separator;
    const typename T::CharType field_separator;
    bool has_analyses;
    bool print_at_end;
    
    std::vector<typename T::string> results;
    std::vector<typename T::Float> weights;
    const typename T::CharType* original_word;
public:
    PrintFunction(int print, FILE* o)
    :   output(o), 
        newline('\n'), input_separator('|'), field_separator('\t'),
        has_analyses(false), print_at_end(false)
    {
        const auto bitfield = SignedBitfield<int>(print);
        const auto print_original = bitfield.Get(0, 2);
        const auto print_input = bitfield.Get(2, 4);
        const auto print_output = bitfield.Get(4, 6);
        const auto print_transitions = bitfield.Get(6, 8);
        const auto print_weight = bitfield.Get(8, 10);
        bool is_first = true;
        if (print_weight < 0)
        {   // relative weights
            print_at_end = true;
            f += [this](const typename T::Path& )
            {
                results.emplace_back();
            };
        }
        f += [this](const typename T::Path&) {has_analyses = true; };
        if (print_original)
        {
            if (!print_at_end)
            {
                f += [this](const typename T::Path& )
                {
                    if (original_word)
                    {
                        fwrite(original_word, sizeof(newline), CharTraits::length(original_word), output);
                    }
                };
            }
            else
            {
                f += [this](const typename T::Path& )
                {
                    if (original_word)
                    {
                        results.back() += original_word;
                    }
                };
            }
            is_first = false;
        }
        if (print_input)
        {
            if (!is_first)
            {
                if (print_at_end)
                    f += [this](const typename T::Path&) {results.back() += field_separator; };
                else
                    f += [this](const typename T::Path&) {fwrite(&field_separator, sizeof(field_separator), 1, output); };
            }
            if (print_input > 0 && !print_at_end)
            {
                f += [this](const typename T::Path& path)
                {
                    auto it = path.begin();
                    fwrite(it->GetInput(), sizeof(newline), CharTraits::length(it->GetInput()), output);
                    for (++it; it != path.end(); ++it)
                    {
                        fwrite(&input_separator, sizeof(input_separator), 1, output);
                        fwrite(it->GetInput(), sizeof(newline), CharTraits::length(it->GetInput()), output);
                    }
                };
            }
            else if (print_input < 0 && !print_at_end)
            {
                f += [this](const typename T::Path& path)
                {
                    auto it = path.begin();
                    if (!T::FlagDiacriticsType::IsIt(it->GetInput()))
                        fwrite(it->GetInput(), sizeof(newline), CharTraits::length(it->GetInput()), output);
                    for (++it; it != path.end(); ++it)
                    {
                        fwrite(&input_separator, sizeof(input_separator), 1, output);
                        if (!T::FlagDiacriticsType::IsIt(it->GetInput()))
                            fwrite(it->GetInput(), sizeof(newline), CharTraits::length(it->GetInput()), output);
                    }
                };
            }
            else if (print_input > 0 && print_at_end)
            {
                f += [this](const typename T::Path& path)
                {
                    auto it = path.begin();
                    results.back() += it->GetInput();
                    for (++it; it != path.end(); ++it)
                    {
                        results.back() += input_separator;
                        results.back() += it->GetInput();
                    }
                };
            }
            else if (print_input < 0 && print_at_end)
            {
                f += [this](const typename T::Path& path)
                {
                    auto it = path.begin();
                    if (!T::FlagDiacriticsType::IsIt(it->GetInput()))
                        results.back() += it->GetInput();
                    for (++it; it != path.end(); ++it)
                    {
                        results.back() += input_separator;
                        if (!T::FlagDiacriticsType::IsIt(it->GetInput()))
                            results.back() += it->GetInput();
                    }
                };
            }
            is_first = false;
        }
        if (print_output)
        {
            if (!is_first)
            {
                if (print_at_end)
                    f += [this](const typename T::Path&) {results.back() += field_separator; };
                else
                    f += [this](const typename T::Path&) {fwrite(&field_separator, sizeof(field_separator), 1, output); };
            }
            if (print_output > 0 && !print_at_end)
            {
                f += [this](const typename T::Path& path)
                {
                    for (const auto& v : path)
                    {
                        const auto output_s = v.GetOutput();
                        fwrite(output_s, sizeof(newline), CharTraits::length(output_s), output);
                    }
                };
            }
            else if (print_output < 0 && !print_at_end)
            {
                f += [this](const typename T::Path& path)
                {
                    for (const auto& v : path)
                    {
                        const auto output_s = v.InterpretOutput();
                        fwrite(output_s, sizeof(newline), CharTraits::length(output_s), output);
                    }
                };
            }
            else if (print_output > 0 && print_at_end)
            {
                f += [this](const typename T::Path& path)
                {
                    for (const auto& v : path)
                    {
                        results.back() += v.GetOutput();
                    }
                };
            }
            else if (print_output < 0 && print_at_end)
            {
                f += [this](const typename T::Path& path)
                {
                    for (const auto& v : path)
                    {
                        results.back() += v.InterpretOutput();
                    }
                };
            }
            is_first = false;
        }
        if (print_transitions)
        {
            if (!is_first)
            {
                if (print_at_end)
                    f += [this](const typename T::Path&) {results.back() += field_separator; };
                else
                    f += [this](const typename T::Path&) {fwrite(&field_separator, sizeof(field_separator), 1, output); };
            }
            if (print_at_end)
            {
                f += [this](const typename T::Path& path)
                {
                    auto it = path.begin();
                    results.back() += attol::WriteIndex<typename T::CharType>(it->GetId() + 1);
                    for (++it; it != path.end(); ++it)
                    {
                        results.back() += input_separator;
                        results.back() += attol::WriteIndex<typename T::CharType>(it->GetId() + 1);
                    }
                };
            }
            else
            {
                f += [this](const typename T::Path& path)
                {
                    static typename T::string index_s;
                    auto it = path.begin();
                    index_s.clear();
                    index_s = attol::WriteIndex<typename T::CharType>(it->GetId() + 1);
                    fwrite(index_s.c_str(), sizeof(newline), index_s.size(), output);
                    for (++it; it != path.end(); ++it)
                    {
                        fwrite(&input_separator, sizeof(input_separator), 1, output);
                        index_s = attol::WriteIndex<typename T::CharType>(it->GetId() + 1);
                        fwrite(index_s.c_str(), sizeof(newline), index_s.size(), output);
                    }
                };
            }
        }
        if (print_weight)
        {
            if (print_weight > 0 && is_first)
            {
                f += [this](const typename T::Path& path)
                {
                    static typename T::string weight_s;
                    typename T::Float weight = 0;
                    weight_s.clear();
                    for (const auto& v : path)
                        weight += v.GetWeight();
                    weight_s = WriteFloat<typename T::CharType>(weight);
                    fwrite(weight_s.c_str(), sizeof(newline), weight_s.size(), output);
                };
            }
            else if (print_weight > 0 && !is_first)
            {
                f += [this](const typename T::Path& path)
                {
                    typename T::Float weight = 0;
                    for (const auto& v : path)
                        weight += v.GetWeight();
                    const auto weight_s = WriteFloat<typename T::CharType>(weight);
                    fwrite(&field_separator, sizeof(field_separator), 1, output);
                    fwrite(weight_s.c_str(), sizeof(newline), weight_s.size(), output);
                };
            }
            else if (print_weight < 0 && is_first)
            {
                f += [this](const typename T::Path& path)
                {
                    typename T::Float weight = 0;
                    for (const auto& v : path)
                        weight += v.GetWeight();
                    weights.emplace_back(weight);
                };
            }
            else if (print_weight < 0 && !is_first)
            {
                f += [this](const typename T::Path& path)
                {
                    typename T::Float weight = 0;
                    for (const auto& v : path)
                        weight += v.GetWeight();
                    weights.emplace_back(weight);
                    results.back() += field_separator;
                };
            }
        }
        if (!print_at_end)
            f += [this](const typename T::Path&) { fwrite(&newline, sizeof(newline), 1, output); };
    }
    void Reset(const typename T::CharType* original = nullptr)
    {
        has_analyses = false; 
        original_word = original;
        if (print_at_end)
        {
            results.clear();
            weights.clear();
        }
    }
    bool Succeeded()
    {
        if (print_at_end)
        {
            const auto max_weight = *std::max_element(weights.begin(), weights.end());
            typename T::Float total_weight = 0;
            typename T::string weight_s;
            for (auto& w : weights)
            {
                total_weight += std::exp(w);
                w -= max_weight;
            }
            weight_s = WriteFloat<typename T ::CharType>(total_weight);
            fwrite(weight_s.c_str(), sizeof(newline), weight_s.size(), output);
            fwrite(&newline, sizeof(newline), 1, output);

            total_weight = 0;
            for (auto& w : weights)
            {
                total_weight += std::exp(w);
            }
            for (size_t i = 0; i < results.size(); ++i)
            {
                fwrite(results[i].c_str(), sizeof(newline), results[i].size(), output);
                weight_s = WriteFloat<typename T::CharType>(100 * std::exp(weights[i]) / total_weight);
                fwrite(weight_s.c_str(), sizeof(newline), weight_s.size(), output);
                fwrite(&newline, sizeof(newline), 1, output);
            }
        }
        return has_analyses; 
    }
    typename T::ResultHandler& GetF() { return f; }
};

}
