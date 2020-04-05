#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <limits>
#include <type_traits>
#include <climits>

#include <attol/Utils.h>

namespace attol {

template<class CharType = char, class FlagStorageType = int>
class FlagDiacritics
{
public:
    FlagDiacritics(): flag_symbol_min(std::numeric_limits<decltype(flag_symbol_min)>::max()){}
    // does the flag state have to be larger than 64bit?
    // typedef typename std::remove_extent<FlagStorageType>::type ValueType;
    static_assert(std::is_integral<CharType>::value, "atto::FlagDiacritics requires an integral type as first template parameter!");
    static_assert(!std::is_const<FlagStorageType>::value, "FlagStorageType cannot be const!");
    static_assert(std::is_integral<FlagStorageType>::value && std::is_signed<FlagStorageType>::value,
        "atto::FlagDiacritics requires a signed integral type as second template parameter!");
    typedef std::basic_string<CharType> string;
    typedef CharType* cstr;
    typedef const CharType* ccstr;
    typedef FlagStorageType StorageType;
    typedef typename std::make_unsigned<StorageType>::type Index;
    struct Operation
    {
        char type;
        unsigned char feature;
        unsigned char value;
        Operation(): type('\0'), feature(0x00), value(0x00) {}
    };
    typedef SignedBitfield<StorageType> State;

    std::vector<StorageType> GetValues(const State& s)const
    {
        std::vector<StorageType> result;
        for (size_t i = 0; i + 1 < offsets.size(); ++i)
        {
            result.push_back(s.Get(offsets[i], offsets[i + 1]));
        }
        return result;
    }

    static bool IsIt(ccstr input)
    {
        // returns false for empty string because input[0] == '\0' != '@'
        return (input[0] == '@' && 
            (input[1] == 'P' || input[1] == 'N' || input[1] == 'D' || input[1] == 'R' || input[1] == 'C' || input[1] == 'U') &&
            input[2] == '.' &&
            input[3] != 0);
    }

    void Memorize(ccstr flags, Index flag_id)
    {
        const auto p = Parse(flags);
        if (p.second.empty())
            flag_map[p.first];
        else
            flag_map[p.first].emplace(p.second);
        op_map[flag_id].assign(flags);
        flag_symbol_min = std::min(flag_symbol_min, flag_id);
    }
    template<class Type>
    void Compile(Type flag_id, Type& output)const
    {
        static_assert(sizeof(Operation) <= sizeof(Type), "");
        reinterpret_cast<Operation&>(output) = operations[flag_id - flag_symbol_min];
    }

    // if false, then state is not modified,
    // if true, then operation is applied to the state 
    bool Apply(const Index& id, State& state)const
    {
        const auto& op = reinterpret_cast<const Operation&>(id);
        const unsigned char bit_start = offsets[op.feature - 1];
        const unsigned char bit_end = offsets[op.feature];
        const auto current_value = state.Get(bit_start, bit_end);
        const auto value = (StorageType)op.value;
        switch (op.type)
        {
        case 'P': // positive set
            state.Set(bit_start, bit_end, value);
            return true;
        
        case 'N': // negative set
            state.Set(bit_start, bit_end, -value);
            return true;
        
        case 'R': // require
            if (value == 0) // empty require
                return current_value != 0;
            else // nonempty require
                return current_value == value;
        
        case 'D': // disallow
            if (value == 0) // empty disallow
                return current_value == 0;
            else // nonempty disallow
                return current_value != value;
        
        case 'C': // clear
            state.Set(bit_start, bit_end, 0);
            return true;
        
        case 'U': // unification
            if (current_value == 0 || /* if the feature is unset or */
                current_value == value || /* the feature is at
                                                      this value already
                                                      or */
                (current_value < 0 &&
                (-current_value != value)) /* the feature is
                                                             negatively set
                                                             to something
                                                             else */
                )
            {
                state.Set(bit_start, bit_end, value);
                return true;
            }
            return false;
        }
        throw; // for the compiler's peace of mind
    }
    void CalculateOffsets()
    {
        offsets.clear();
        unsigned char bits = 0;
        if (flag_map.size() > std::numeric_limits<unsigned char>::max())
        {
            throw Error("There are ", flag_map.size(), " flag diacritic features, "
                "which is more than what a single input character can hold!");
        }
        for (const auto& flag : flag_map)
        {
            const unsigned char required_flag_bits = IntLog2<unsigned char, size_t>(2 * (flag.second.size() + 1));
            offsets.emplace_back(bits);
            bits += required_flag_bits;
        }
        if (bits > sizeof(StorageType) * CHAR_BIT)
            throw Error("Storing the state of the flag diacritics requires ", bits,
                " bits, but FlagDiacritics::State is only ", sizeof(StorageType) * CHAR_BIT, " bits wide!");
        offsets.emplace_back(bits);

        operations.clear();
        operations.resize(op_map.size());
        for (const auto& op : op_map)
        {
            Operation& newop = operations[op.first - flag_symbol_min];
            newop.type = (char)(op.second[1]);
            const auto p = Parse(op.second.c_str());
            const auto b = GetFeatureValue(p.first.c_str(), p.second.c_str());
            newop.feature = b.first;
            newop.value = b.second;
        }
    }
    bool Write(FILE* f)const
    {
        if (fwrite(&flag_symbol_min, sizeof(flag_symbol_min), 1, f) != 1)
            return false;
        if (!WriteBinaryVector(f, offsets))
            return false;
        if (!WriteBinaryVector(f, operations))
            return false;
        return true;
    }
    bool Read(FILE* f)
    {
        if (fread(&flag_symbol_min, sizeof(flag_symbol_min), 1, f) != 1)
            return false;
        if (!ReadBinaryVector(f, offsets))
            return false;
        if (!ReadBinaryVector(f, operations))
            return false;
        return true;
    }
private:
    static std::pair<string, string> Parse(ccstr flags)
    {
        std::pair<string, string> p;
        auto& feature = p.first;
        auto& value = p.second;
        feature = flags + 3;

        // @
        feature.pop_back();

        const size_t i = feature.find(CharType('.'));
        if (i != string::npos)
        {
            value = feature.substr(i + 1);
            feature = feature.substr(0, i);
        }
        return p;
    }
    // typedef typename std::make_unsigned<CharType>::type UCharType;
    std::pair<unsigned char, unsigned char> GetFeatureValue(ccstr feature, ccstr value)const
    {
        std::pair<unsigned char, unsigned char> p((unsigned char)1, (unsigned char)0);
        for (auto& f : flag_map)
        {
            if (f.first == feature)
            {
                if (value[0]) // not empty value
                {
                    p.second = 1;
                    for (auto& v : f.second)
                    {
                        if (v == value)
                        {
                            return p;
                        }
                        ++p.second;
                    }
                }
                else
                {
                    return p;
                }
            }
            ++p.first;
        }
        throw Error("Oops!");
    }
private:
    std::unordered_map<string, std::unordered_set<string>> flag_map;
    std::unordered_map<Index, string> op_map;
    typename std::make_unsigned<StorageType>::type flag_symbol_min;
    std::vector<Operation> operations;
    std::vector<unsigned char> offsets;
};

}
