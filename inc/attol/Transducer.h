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
    struct ToPointers : std::array<Index, 2>
    {
        ToPointers() { this->fill((Index)0); }
    };

    std::vector<Index> transitions_table;
    ToPointers start_state;
    size_t n_transitions, n_states;
public:
    typedef FlagDiacritics<CharType, typename std::make_signed<Index>::type> FlagDiacriticsType;
    typedef typename FlagDiacriticsType::State FlagState;
    typedef std::basic_string<CharType> string;
private:
    FlagDiacriticsType fd_table;
public:
    struct PathValue : protected std::tuple<const CharType*, const CharType*, Index, Float, FlagState>
    {
        typedef std::tuple<const CharType*, const CharType*, Index, Float, FlagState> TupleType;
        PathValue(const CharType* input, const CharType* output, Index id, Float weight, FlagState flag)
            : TupleType(input, output, id, weight, flag)
        {}
        PathValue(RecordIterator<const Index, const CharType>& it, FlagState flag)
            : TupleType(it.GetInput(), it.GetOutput(), it.GetId(), it.GetWeight(), flag)
        {}
        const CharType* GetInput()const { return std::get<0>(*this); }
        const CharType* GetOutput()const { return std::get<1>(*this); }
        const Index& GetId()const { return std::get<2>(*this); }
        const Float& GetWeight()const { return std::get<3>(*this); }
        const FlagState& GetFlag()const { return std::get<4>(*this); }
        //! retrieves output string, but the special symbols and flag diacritics are resolved
        /*!
            unfortunately, if UNKNOWN_SYMBOL is on the output tape then there is not much I can do.
        */
        const CharType* InterpretOutput()const
        {
            static CharType buffer[5]; // should be plenty for all encodings
            const auto output = GetOutput();
            if (FlagDiacriticsType::IsIt(output))
                return output - 1; // it will be the end of input, which will be NULL
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

    Transducer()
        : n_transitions(0), n_states(0), max_results(0), max_depth(0), time_limit(0), results()
    {
    }
    Transducer(FILE* f, CharType field_separator = '\t')
        : n_transitions(0), n_states(0), max_results(0), max_depth(0), time_limit(0), results()
    {
        Read(f, field_separator);
    }
    void Read(FILE* f, CharType field_separator = '\t')
    {
        std::unordered_map<Index, ToPointers> state_pointers;

        transitions_table.clear();
        n_transitions = 0;
        n_states = 0;
        {
        CharType c;
        Index previous_state = 0;
        string line;
        Index from = 0, to;
        // Counter<const CharType*, Index, StrHash<CharType>, StrEqualType<CharType>> states;
        Counter<string, Index> states;
        string input, output;
        Float weight;

        while (!feof(f))
        {
            line.clear();
            while (fread(&c, sizeof(CharType), 1, f) == 1 && c != '\n')
                line.push_back(c);
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            if (line.empty())
                break;
            size_t pos = 0, pos_next;
            int i = 0;
            while (string::npos != (pos_next = line.find(field_separator, pos)))
            {
                switch (i++)
                {
                case 0:
                    from = states[line.substr(pos, pos_next - pos)];
                    break;
                case 1:
                    to = states[line.substr(pos, pos_next - pos)];
                    break;
                case 2:
                    input.assign(line.data() + pos, line.data() + pos_next);
                    break;
                case 3:
                    output.assign(line.data() + pos, line.data() + pos_next);
                    break;
                default:
                    throw Error("AT&T file at line", n_transitions + 1, " has more than ", i, " columns!");
                }
                pos = pos_next + 1;
            }
            switch (i)
            {
            case 0:
                from = states[line.substr(pos)];
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
                throw Error("AT&T text file at line ", n_transitions + 1, " has wrong number of columns!");
            }
            
            if (previous_state != from)
            {
                if (state_pointers.find(from) != state_pointers.end())
                    // this state has already been visited
                    throw Error("Transitions are not ordered by starting state! Starting state of transition ", n_transitions + 1, " has already been visited.");
                state_pointers[from][0] = Index(transitions_table.size());
                state_pointers[previous_state][1] = state_pointers[from][0];
                previous_state = from;
            }

            //! write the binary format (pre-compile)
            transitions_table.emplace_back(Index(n_transitions));
            transitions_table.emplace_back(from); // this will be to_start
            transitions_table.emplace_back(to); // this will be to_end
            transitions_table.emplace_back();
            std::memcpy(&transitions_table.back(), &weight, sizeof(weight));

            for (auto s : { &input, &output })
            {
                if (StrEqual(s->c_str(), "@0@") || StrEqual(s->c_str(), "@_EPSILON_SYMBOL_@"))
                    s->clear();
            }
            if (fd_table.IsIt(input.c_str()))
            {
                fd_table.Read(input.c_str());
            }
            CopyStr(input, transitions_table);
            CopyStr(output, transitions_table);

            ++n_transitions;
        }
        state_pointers[previous_state][1] = SaturateCast<Index>::Do(transitions_table.size());
        n_states = state_pointers.size();
        }
        fd_table.CalculateOffsets();

        // write the binary format
        const auto end = transitions_table.data() + transitions_table.size();
        for (RecordIterator<Index, CharType> i(transitions_table.data()); i < end; ++i)
        {
            auto& from = i.GetFrom();
            auto& to = i.GetTo();

            if (to != std::numeric_limits<Index>::max())
            {   // non-final state
                from = state_pointers[to][0];
                to = state_pointers[to][1];
            }// these are 'pointers' now, not indexes

            if (fd_table.IsIt(i.GetInput()))
                fd_table.Compile(i.GetInput());
        }

        // supposing that the START is the first row
        start_state = state_pointers[(Index)0];
    }
    bool Write(FILE* f, CharType field_separator = '\t')const
    {
        std::vector<Index> sorted_pointers;
        const RecordIterator<const Index, const CharType> end(transitions_table.data() + transitions_table.size());
        {
            std::unordered_set<Index> pointers;
            pointers.reserve(n_states + 1);
            pointers.emplace(start_state[0]);
            pointers.emplace(start_state[1]);
            for (RecordIterator<const Index, const CharType> i(transitions_table.data()); i < end; ++i)
            {
                if (i.GetTo() != std::numeric_limits<Index>::max())
                {
                    pointers.emplace(i.GetFrom());
                    pointers.emplace(i.GetTo());
                }
            }
            pointers.emplace((Index)transitions_table.size());
            sorted_pointers.assign(pointers.begin(), pointers.end());
        }
        std::sort(sorted_pointers.begin(), sorted_pointers.end());
        string line;
        Index from_state = 0, to_state;
        const CharType newline = '\n';

        for (RecordIterator<const Index, const CharType> i(transitions_table.data()); i < end; ++i)
        {
            while (sorted_pointers[from_state + 1] <= (const Index*)i - transitions_table.data())
            {
                ++from_state;
            }
            line = WriteIndex<CharType>(from_state);
            line += field_separator;
            if (i.GetTo() == std::numeric_limits<Index>::max())
            {   // final state
                line += WriteFloat<CharType>(i.GetWeight());
            }
            else if (i.GetFrom() != i.GetTo())
            {   // non-final and non-dangling
                to_state = (Index)(std::lower_bound(sorted_pointers.begin(), sorted_pointers.end(), i.GetFrom()) - sorted_pointers.begin());
                line += WriteIndex<CharType>(to_state);
                line += field_separator;
                if (fd_table.IsIt(i.GetOutput()))
                    line += i.GetOutput();
                else
                    line += i.GetInput();
                line += field_separator;
                line += i.GetOutput();
                line += field_separator;
                line += WriteFloat<CharType>(i.GetWeight());
            }
            else
                continue;
            line += newline;
            if (fwrite(line.c_str(), sizeof(CharType), line.size(), f) != line.size())
                return false;
        }
        return true;
    }
    bool WriteBinary(FILE* f)const
    {
        if (!WriteBom<enc>(f))
            return false;
        {
            const Index width = storageSize;
            if (fwrite(&width, sizeof(Index), 1, f) != 1)
                return false;
        }
        if (fwrite(start_state.data(), sizeof(start_state), 1, f) != 1)
            return false;
        auto offsets = fd_table.GetOffsets();
        offsets.resize(256, '\xFF');
        if (fwrite(offsets.data(), 256, 1, f) != 1)
            return false;
        if (fwrite(transitions_table.data(), sizeof(Index), transitions_table.size(), f) != transitions_table.size())
            return false;
        return true;
    }
    bool ReadBinary(FILE* f)
    {
        if (!CheckBom<enc>(f))
            return false;
        {
            Index width;
            if (fread(&width, sizeof(Index), 1, f) != 1)
                return false;
            if (width != storageSize)
                return false;
        }
        if (fread(start_state.data(), sizeof(start_state), 1, f) != 1)
            return false;

        std::vector<unsigned char> offsets(256);
        if (fread(offsets.data(), 256, 1, f) != 1)
            return false;
        fd_table.SetOffsets(offsets);
        const auto pos = ftell(f);
        fseek(f, 0, SEEK_END);
        const auto size = ftell(f) - pos;
        fseek(f, pos, SEEK_SET);
        transitions_table.resize(size / sizeof(Index));
        if (fread(transitions_table.data(), sizeof(Index), transitions_table.size(), f) != transitions_table.size())
            return false;

        return true;
    }

    //! including to finishing from a final state
    size_t GetNumberOfTransitions()const { return n_transitions; }
    //! including start state
    size_t GetNumberOfStates()const { return n_states; }
    size_t GetAllocatedMemory()const{ return sizeof(Index)*transitions_table.size(); }

    // typedef Handlers<Path> ResultHandler;

    size_t max_results;
    size_t max_depth;
    double time_limit;
    std::vector<Path> results;

    template<FlagStrategy strategy = FlagStrategy::OBEY>
    void Lookup(const CharType* s)
    {
        results.clear();
        lookup<strategy>(nullptr, 0, 0);
        return lookup<strategy>(s, start_state[0], start_state[1]);
    }
    void Lookup(const CharType* s, FlagStrategy strategy)
    {
        switch (strategy)
        {
        case FlagStrategy::IGNORE:
            return Lookup<IGNORE>(s);
        case FlagStrategy::NEGATIVE:
            return Lookup<NEGATIVE>(s);
        default:
            return Lookup<OBEY>(s);
        };
    }
private:
    template<FlagStrategy strategy>
    void lookup(const CharType* s, Index beg, const Index end)
    {
        static thread_local Path path;
        static thread_local size_t n_results;
        static thread_local Clock<> myclock;
        static thread_local bool flag_failed;

        if (s == nullptr)
        {
            path.clear();
            n_results = 0;
            myclock.Tick();
            flag_failed = false;
            return;
        }
        if ((max_results > 0 && n_results >= max_results) ||
            (max_depth > 0 && path.size() >= max_depth) ||
            (time_limit > 0 && myclock.Tock() >= time_limit))
        {
            return;
        }
        const auto flag_state = path.empty() ? FlagState() : path.back().GetFlag();
        for (RecordIterator<const Index, const CharType> i(transitions_table.data() + beg); i < transitions_table.data() + end; ++i)
        {   // try outgoing edges
            const auto input = i.GetInput();
            
            if (i.GetTo() == std::numeric_limits<Index>::max())
            {   //final state
                if (*s == 0 && (strategy != NEGATIVE || flag_failed))
                {   // that's a result
                    ++n_results;
                    path.emplace_back(i, flag_state);
                    results.emplace_back(path);
                    path.pop_back();
                }
            }
            else if (*s != 0 && (StrEqual(input, "@_IDENTITY_SYMBOL_@") || StrEqual(input, "@_UNKNOWN_SYMBOL_@")))
            {   // consume one character, is s is empty, then there is nothing to consume
                // TODO this can be hastened if the special symbols are shorter!
                path.emplace_back(s, i.GetOutput(), i.GetId(), i.GetWeight(), flag_state);
                lookup<strategy>(GetNextCharacter<enc>(s), i.GetFrom(), i.GetTo());
                path.pop_back();
            }
            // 
            // epsilon is handled with a simple empty string
            // 
            else if (fd_table.IsIt(input))
            {   // flag diacritic
                if (strategy == IGNORE)
                {   // go with it, no matter what
                    path.emplace_back(i.GetOutput(), i.GetOutput(), i.GetId(), i.GetWeight(), flag_state);
                    lookup<strategy>(s, i.GetFrom(), i.GetTo());
                    path.pop_back();
                }
                else
                {
                    auto new_flag_state = flag_state;
                    if (fd_table.Apply(input, new_flag_state))
                    {
                        path.emplace_back(i.GetOutput(), i.GetOutput(), i.GetId(), i.GetWeight(), new_flag_state);
                        lookup<strategy>(s, i.GetFrom(), i.GetTo());
                        path.pop_back();
                    }
                    else if (strategy == NEGATIVE)
                    {
                        const bool previous_fail = flag_failed;
                        flag_failed = true;
                        path.emplace_back(i.GetOutput(), i.GetOutput(), i.GetId(), i.GetWeight(), new_flag_state);
                        lookup<strategy>(s, i.GetFrom(), i.GetTo());
                        path.pop_back();
                        flag_failed = previous_fail;
                    }
                }
            }
            else if (const CharType* next = StrPrefix<enc>(s, input))
            {   // a lead to follow
                path.emplace_back(i, flag_state);
                lookup<strategy>(next, i.GetFrom(), i.GetTo());
                path.pop_back();
            }
        }
    }
};

}
