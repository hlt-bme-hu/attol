#pragma once

#include <vector>
#include <cstdio>
#include <cstring>
#include <functional>
#include <array>
#include <tuple>
#include <cstdint>
#include <type_traits>
#include <unordered_set>
#include <algorithm>

#include "attol/FlagDiacritics.h"
#include "attol/Char.h"
#include "attol/Utils.h"
#include "attol/Record.h"

namespace attol {
    
template<class CharType, class Index, class Float>
bool AttParse(const std::basic_string<CharType>& line, 
    Index& from, Index& to, Float& weight,
    std::basic_string<CharType>& input, std::basic_string<CharType>& output, 
    CharType field_separator = '\t')
{
    typedef std::basic_string<CharType> string;
    size_t pos = 0, pos_next;
    int i = 0;
    while (string::npos != (pos_next = line.find(field_separator, pos)))
    {
        switch (i++)
        {
        case 0:
            ReadIndex(line.substr(pos, pos_next - pos).c_str(), from);
            break;
        case 1:
            ReadIndex(line.substr(pos, pos_next - pos).c_str(), to);
            break;
        case 2:
            input.assign(line.data() + pos, line.data() + pos_next);
            break;
        case 3:
            output.assign(line.data() + pos, line.data() + pos_next);
            break;
        default:
            return false;
            //throw Error("AT&T file at line ", transitions.size() + 1, " has more than ", i, " columns!");
        }
        pos = pos_next + 1;
    }
    switch (i)
    {
    case 0:
        ReadIndex(line.data() + pos, from);
        to = std::numeric_limits<Index>::max();
        weight = 0;
        input.clear();
        output.clear();
        break;
    case 1:
        to = std::numeric_limits<Index>::max();
        ReadFloat(line.data() + pos, weight);
        input.clear();
        output.clear();
        break;
    case 3:
        output.assign(line.data() + pos);
        weight = 0;
        break;
    case 4:
        ReadFloat(line.data() + pos, weight);
        break;
    default:
        return false;
        // throw Error("AT&T text file at line ", transitions.size() + 1, " has wrong number of columns!");
    }
    return true;
}

template<Encoding enc = UTF8, size_t storageSize = 32>
class Transducer
{
public:
    typedef typename std::conditional<storageSize == 64, std::uint64_t, std::uint32_t>::type Index;
    typedef typename std::conditional<storageSize == 64, double, float>::type Float;
    typedef typename CodeUnit<enc>::type CharType;

    static_assert(sizeof(Float) == sizeof(Index), "");
    static_assert(sizeof(Index) % sizeof(CharType) == 0, "");
private:
    struct Transition
    {
        Index from;
        Index to;
        Index input;
        Index output;
        Float weight;
        bool operator<(const Transition& other)const
        {
            // max value in 'to' means 'go to final state'
            if (to == std::numeric_limits<Index>::max() || other.to == std::numeric_limits<Index>::max())
            {
                return to > other.to;
            }
            // empty_symbol is always the first
            if (input == 0 || other.input == 0)
            {
                return input < other.input;
            }
            return input > other.input;
        }
    };
public:
    typedef FlagDiacritics<CharType, typename std::make_signed<Index>::type> FlagDiacriticsType;
    typedef typename FlagDiacriticsType::State FlagState;
    typedef std::basic_string<CharType> string;
    
    struct PathValue : protected std::tuple<const CharType*, const CharType*, Index, Index, Float, FlagState>
    {
        typedef std::tuple<const CharType*, const CharType*, Index, Index, Float, FlagState> TupleType;
        using TupleType::TupleType;
        
        const CharType* GetInput()const { return std::get<0>(*this); }
        const CharType* GetOutput()const { return std::get<1>(*this); }
        const Index& GetId()const { return std::get<2>(*this); }
        const Index& GetFrom()const { return std::get<3>(*this); }
        const Float& GetWeight()const { return std::get<4>(*this); }
        const FlagState& GetFlag()const { return std::get<5>(*this); }
        //! retrieves output string, but the special symbols and flag diacritics are resolved
        /*!
            unfortunately, if UNKNOWN_SYMBOL is on the output tape then there is not much I can do.
        */
        const CharType* InterpretOutput()const
        {
            static CharType buffer[5]; // should be plenty for all encodings
            const auto output = GetOutput();
            if (FlagDiacriticsType::IsIt(output))
                return output + std::char_traits<CharType>::length(output);
            else if (StrEqual(output, "@_IDENTITY_SYMBOL_@"))
            {
                std::memset(buffer, 0, sizeof(buffer));
                std::memcpy(buffer, GetInput(), (const char*)GetNextCharacter<enc>(GetInput()) - (const char*)GetInput());
                return buffer;
            }
            else
                return output;
        }
    };
    typedef std::vector<PathValue> Path;
private:
    template<class Alphabet>
    void CompileAlphabet(Alphabet& alphabet_hash)
    {
        alphabet.resize(alphabet_hash.size());
        reverse_alphabet_map.clear();
        raw_alphabet.clear();
        raw_alphabet.reserve(2 * alphabet_hash.size());
        for (const auto& symbol : alphabet_hash)
        {
            reverse_alphabet_map[symbol.first] = symbol.second;
            alphabet[symbol.second] = (Index)raw_alphabet.size();
            raw_alphabet.insert(raw_alphabet.end(), symbol.first.begin(), symbol.first.end());
            raw_alphabet.emplace_back((CharType)0);
        }
    }
    const CharType* GetSymbolStr(Index symbol)const
    {
        return raw_alphabet.data() + alphabet[symbol];
    }
    std::vector<CharType> raw_alphabet;
    std::vector<Index> alphabet;
    std::unordered_map<string, Index> reverse_alphabet_map;
    std::vector<Transition> transitions;
    size_t n_states;
    FlagDiacriticsType fd_table;
    Index unknown_symbol, identity_symbol, empty_symbol, flag_symbol;
    // during the lookup
    // TODO make the above const during the lookup
    // static inheritance
    Path path;
    size_t n_results;
    std::vector<Index> input_tape;
    Index input_tape_pos;
    Clock<> myclock;
    bool flag_failed;
public:
    Transducer()
        : n_states(0), input_tape_pos(0), max_results(0), max_depth(0), time_limit(0), resulthandler([](const Path&) {})
    {
    }
    Transducer(FILE* f, CharType field_separator = '\t')
        : n_states(0), input_tape_pos(0), max_results(0), max_depth(0), time_limit(0), resulthandler([](const Path&) {})
    {
        Read(f, field_separator);
    }
    void Read(FILE* f, CharType field_separator = '\t')
    {
        std::unordered_map<Index, Index> start_pointers;
        start_pointers[0] = 0;
        transitions.clear();
        alphabet.clear();
        n_states = 0;
        Counter<string, Index> alphabet_hash, flag_hash;
        {
            CharType c;
            Index previous_state = 0;
            string line;
            Index from = 0, to = std::numeric_limits<Index>::max();
            string input, output;
            Float weight;
            empty_symbol = alphabet_hash[Convert<CharType>("")];
            unknown_symbol = alphabet_hash[Convert<CharType>("@_UNKNOWN_SYMBOL_@")];
            identity_symbol = alphabet_hash[Convert<CharType>("@_IDENTITY_SYMBOL_@")];
            while (!feof(f))
            {
                line.clear();
                while (fread(&c, sizeof(CharType), 1, f) == 1 && c != '\n')
                    line.push_back(c);
                if (!line.empty() && line.back() == '\r')
                    line.pop_back();
                if (line.empty())
                    break;
                if (!AttParse<CharType>(line, from, to, weight, input, output, field_separator))
                    throw Error("AT&T file at line ", transitions.size() + 1, " is invalid!");

                if (previous_state != from)
                {
                    if (start_pointers.find(from) != start_pointers.end())
                        // this state has already been visited
                        throw Error("Transitions are not ordered by starting state! Starting state of transition ", transitions.size() + 1, " has already been visited.");
                    start_pointers[from] = SaturateCast<Index>::Do(transitions.size());
                    previous_state = from;
                }
                transitions.emplace_back();

                if (to != std::numeric_limits<Index>::max())
                {
                    for (auto s : { &input, &output })
                    {
                        if (StrEqual(s->c_str(), "@0@") || StrEqual(s->c_str(), "@_EPSILON_SYMBOL_@"))
                            s->clear();
                    }
                    if (fd_table.IsIt(input.c_str()))
                    {
                        // save these special cases for later
                        transitions.back().input = std::numeric_limits<Index>::max();
                        transitions.back().output = flag_hash[output];
                    }
                    else
                    {
                        transitions.back().input = alphabet_hash[input];
                        transitions.back().output = alphabet_hash[output];
                    }
                }
                transitions.back().from = from;
                transitions.back().to = to;
                transitions.back().weight = weight;
            }
        }

        n_states = start_pointers.size();
        flag_symbol = SaturateCast<Index>::Do(alphabet_hash.size());
        for (const auto& flags : flag_hash)
        {
            fd_table.Memorize(flags.first.c_str(), alphabet_hash[flags.first]);
        }
        CompileAlphabet(alphabet_hash);
        fd_table.CalculateOffsets();

        Index i, j;
        for (i = 0, j = 0; i < transitions.size(); i = j)
        {
            for (; j < transitions.size() && transitions[j].from == transitions[i].from; ++j)
            {
                auto& t = transitions[j];
                if (t.to != std::numeric_limits<Index>::max())
                {   // non-final state
                    auto it = start_pointers.find(t.to);
                    if (it != start_pointers.end())
                        t.to = start_pointers[t.to];
                    else // dangling edge
                        t.to = (Index)transitions.size();
                }

                if (t.input >= flag_symbol)
                {
                    for (const auto& flag : flag_hash)
                    {
                        // re-find which string is it exactly
                        if (flag.second == t.output)
                        {
                            t.input = alphabet_hash[flag.first];
                            break;
                        }
                    }
                    fd_table.Compile(t.input, t.output);
                }
            }
            std::sort(transitions.begin() + i, transitions.begin() + j);
        }
    }
    bool Write(FILE* f, CharType field_separator = '\t')const
    {
        //    std::vector<Index> sorted_pointers;
        //    const RecordIterator<const Index, const CharType> end(transitions.data() + transitions.size());
        //    {
        //        std::unordered_set<Index> pointers;
        //        pointers.reserve(n_states + 1);
        //        pointers.emplace(start_state[0]);
        //        pointers.emplace(start_state[1]);
        //        for (RecordIterator<const Index, const CharType> i(transitions.data()); i < end; ++i)
        //        {
        //            if (i.GetTo() != std::numeric_limits<Index>::max())
        //            {
        //                pointers.emplace(i.GetFrom());
        //                pointers.emplace(i.GetTo());
        //            }
        //        }
        //        pointers.emplace((Index)transitions.size());
        //        sorted_pointers.assign(pointers.begin(), pointers.end());
        //    }
        //    std::sort(sorted_pointers.begin(), sorted_pointers.end());
        //    string line;
        //    Index from_state = 0, to_state;
        //    const CharType newline = '\n';
        // 
        //    for (RecordIterator<const Index, const CharType> i(transitions.data()); i < end; ++i)
        //    {
        //        while (sorted_pointers[from_state + 1] <= (const Index*)i - transitions.data())
        //        {
        //            ++from_state;
        //        }
        //        line = WriteIndex<CharType>(from_state);
        //        line += field_separator;
        //        if (i.GetTo() == std::numeric_limits<Index>::max())
        //        {   // final state
        //            line += WriteFloat<CharType>(i.GetWeight());
        //        }
        //        else if (i.GetFrom() != i.GetTo())
        //        {   // non-final and non-dangling
        //            to_state = (Index)(std::lower_bound(sorted_pointers.begin(), sorted_pointers.end(), i.GetFrom()) - sorted_pointers.begin());
        //            line += WriteIndex<CharType>(to_state);
        //            line += field_separator;
        //            if (fd_table.IsIt(i.GetOutput()))
        //                line += i.GetOutput();
        //            else
        //                line += i.GetInput();
        //            line += field_separator;
        //            line += i.GetOutput();
        //            line += field_separator;
        //            line += WriteFloat<CharType>(i.GetWeight());
        //        }
        //        else
        //            continue;
        //        line += newline;
        //        if (fwrite(line.c_str(), sizeof(CharType), line.size(), f) != line.size())
        //            return false;
        //    }
        return true;
    }
    bool WriteBinary(FILE* f)const
    {
        // BOM
        if (!WriteBom<enc>(f))
            return false;
        // storage size info
        {
            const Index width = storageSize;
            if (fwrite(&width, sizeof(Index), 1, f) != 1)
                return false;
        }
        // alphabet
        {
            if (!WriteBinaryVector(f, raw_alphabet))
                return false;
            if (!WriteBinaryVector(f, alphabet))
                return false;
        }
        if (!fd_table.Write(f))
            return false;
        
        if (fwrite(&unknown_symbol, sizeof(Index), 1, f) != 1)
            return false;
        if (fwrite(&identity_symbol, sizeof(Index), 1, f) != 1)
            return false;
        if (fwrite(&empty_symbol, sizeof(Index), 1, f) != 1)
            return false;
        if (fwrite(&flag_symbol, sizeof(Index), 1, f) != 1)
            return false;

        // transitions themselves
        return WriteBinaryVector(f, transitions);
    }
    bool ReadBinary(FILE* f)
    {
        n_states = 0;
        // BOM
        if (!CheckBom<enc>(f))
            return false;
        // storage size info
        {
            Index width;
            if (fread(&width, sizeof(Index), 1, f) != 1)
                return false;
            if (width != storageSize)
                return false;
        }
        // alphabet
        {
            if (!ReadBinaryVector(f, raw_alphabet))
                return false;
            if (!ReadBinaryVector(f, alphabet))
                return false;
        }
        if (!fd_table.Read(f))
            return false;

        if (fread(&unknown_symbol, sizeof(Index), 1, f) != 1)
            return false;
        if (fread(&identity_symbol, sizeof(Index), 1, f) != 1)
            return false;
        if (fread(&empty_symbol, sizeof(Index), 1, f) != 1)
            return false;
        if (fread(&flag_symbol, sizeof(Index), 1, f) != 1)
            return false;

        reverse_alphabet_map.clear();
        for (Index i = 0; i < alphabet.size(); ++i)
        {
            reverse_alphabet_map[GetSymbolStr(i)] = i;
        }
        return ReadBinaryVector(f, transitions);
    }

    //! including to finishing from a final state
    size_t GetNumberOfTransitions()const { return transitions.size(); }
    //! including start state
    size_t GetNumberOfStates()const { return n_states; }
    size_t GetAllocatedMemory()const
    {
        return sizeof(Transition) * transitions.size() +
            sizeof(Index) * alphabet.size() +
            sizeof(CharType) * raw_alphabet.size();
    }

    size_t max_results;
    size_t max_depth;
    double time_limit;
    std::function<void(const Path& path)> resulthandler;

    template<FlagStrategy strategy = FlagStrategy::OBEY, bool check_limits = false>
    void Lookup(const CharType* s)
    {
        Segment(s, input_tape);
        n_results = 0;
        myclock.Tick();
        flag_failed = false;
        return lookup<strategy, check_limits>(0);
    }
    template<bool check_limits = false>
    void Lookup(const CharType* s, FlagStrategy strategy)
    {
        switch (strategy)
        {
        case FlagStrategy::IGNORE:
            return Lookup<IGNORE, check_limits>(s);
        case FlagStrategy::NEGATIVE:
            return Lookup<NEGATIVE, check_limits>(s);
        default:
            return Lookup<OBEY, check_limits>(s);
        };
    }
private:
    void Segment(const CharType* s, std::vector<Index>& inputtape)const
    {
        inputtape.clear();
        for (; *s; StepNextCharacter<enc>(s))
        {
            const string letter(s, GetNextCharacter<enc>(s));
            const auto it = reverse_alphabet_map.find(letter);
            if (it != reverse_alphabet_map.end())
                inputtape.emplace_back(it->second);
            else
                inputtape.emplace_back(unknown_symbol);
        }
    }
    template<FlagStrategy strategy, bool check_limits>
    void lookup(Index i)
    {
        if (check_limits)
        {
            if ((max_results > 0 && n_results >= max_results) ||
                (max_depth > 0 && path.size() >= max_depth) ||
                (time_limit > 0 && myclock.Tock() >= time_limit))
            {
                return;
            }
        }
        const auto flag_state = path.empty() ? FlagState() : path.back().GetFlag();
        const auto state = transitions[i].from;
        if (input_tape_pos == input_tape.size() && (strategy != NEGATIVE || flag_failed))
        {   // try final transitions
            for (; i < transitions.size() &&
                transitions[i].from == state &&
                transitions[i].to == std::numeric_limits<Index>::max();
                ++i)
            {   //final transition
                if (check_limits)
                    ++n_results;
                path.emplace_back(GetSymbolStr(empty_symbol), GetSymbolStr(empty_symbol), i, state, transitions[i].weight, flag_state);
                resulthandler(path);
                path.pop_back();
            }
        }
        else
        {   // skip final transitions
            // TODO logarithmic ?
            while (i < transitions.size() &&
                transitions[i].from == state &&
                transitions[i].to == std::numeric_limits<Index>::max())
            {
                ++i;
            }
        }
        // try epsilon transitions
        for (; i < transitions.size() &&
            transitions[i].from == state &&
            transitions[i].input == empty_symbol;
            ++i)
        {
            path.emplace_back(GetSymbolStr(empty_symbol), GetSymbolStr(transitions[i].output), i, state, transitions[i].weight, flag_state);
            lookup<strategy, check_limits>(transitions[i].to);
            path.pop_back();
        }
        // try diacritic flags
        for (; i < transitions.size() &&
            transitions[i].from == state &&
            transitions[i].input >= flag_symbol;
            ++i)
        {
            if (strategy == IGNORE)
            {   // go with it, no matter what
                path.emplace_back(GetSymbolStr(transitions[i].input), GetSymbolStr(transitions[i].input), i, state, transitions[i].weight, flag_state);
                lookup<strategy, check_limits>(transitions[i].to);
                path.pop_back();
            }
            else
            {
                auto new_flag_state = flag_state;
                if (fd_table.Apply(transitions[i].output, new_flag_state))
                {
                    path.emplace_back(GetSymbolStr(transitions[i].input), GetSymbolStr(transitions[i].input), i, state, transitions[i].weight, new_flag_state);
                    lookup<strategy, check_limits>(transitions[i].to);
                    path.pop_back();
                }
                else if (strategy == NEGATIVE)
                {
                    const bool previous_fail = flag_failed;
                    flag_failed = true;
                    path.emplace_back(GetSymbolStr(transitions[i].input), GetSymbolStr(transitions[i].input), i, state, transitions[i].weight, new_flag_state);
                    lookup<strategy, check_limits>(transitions[i].to);
                    path.pop_back();
                    flag_failed = previous_fail;
                }
            }
        }
        // try only if there is any symbol left to read
        if (input_tape_pos < input_tape.size())
        {   // normal transitions
            // TODO binary-search
            for (; i < transitions.size() &&
                transitions[i].from == state &&
                transitions[i].input != unknown_symbol &&
                transitions[i].input != identity_symbol;
                ++i)
            {
                if (input_tape[input_tape_pos] == transitions[i].input)
                {   // a lead to follow
                    path.emplace_back(GetSymbolStr(transitions[i].input), GetSymbolStr(transitions[i].output), i, state, transitions[i].weight, flag_state);
                    ++input_tape_pos;
                    lookup<strategy, check_limits>(transitions[i].to);
                    --input_tape_pos;
                    path.pop_back();
                }
            }
            // id/unk transitions, always fallow
            for (; i < transitions.size() && transitions[i].from == state; ++i)
            {
                path.emplace_back(GetSymbolStr(transitions[i].input), GetSymbolStr(transitions[i].output), i, state, transitions[i].weight, flag_state);
                ++input_tape_pos;
                lookup<strategy, check_limits>(transitions[i].to);
                --input_tape_pos;
                path.pop_back();
            }
        }
    }
};

}
