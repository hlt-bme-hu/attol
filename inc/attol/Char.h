#pragma once

#include <cstdint>
#include <type_traits>
#include <vector>
#include <array>
#include <string>

#include "attol/Utils.h"

namespace attol {

enum Encoding
{
    ASCII,
    CP,
    OCTET = CP,
    UTF8,
    UCS2,
    UTF16,
    UTF32
};

template<Encoding e>
struct CodeUnit
{
    typedef char type;
    static const size_t size = sizeof(type);
    type c;
};

template<>
struct CodeUnit<Encoding::UCS2>
{
    typedef char16_t type;
    static const size_t size = sizeof(type);
    type c;
};

// see https://en.cppreference.com/w/cpp/language/types#Character_types
template<>
struct CodeUnit<Encoding::UTF16>
{
    typedef char16_t type;
    static const size_t size = sizeof(type);
    type c;
};

// see https://en.cppreference.com/w/cpp/language/types#Character_types
template<>
struct CodeUnit<Encoding::UTF32>
{
    typedef char32_t type;
    static const size_t size = sizeof(type);
    type c;
};

template<Encoding e>
const typename CodeUnit<e>::type*& StepNextCharacter(const typename CodeUnit<e>::type*& word)noexcept
{
    typedef typename CodeUnit<e>::type CharType;
    switch (e)
    {
    case Encoding::UTF8:
        // step over continuation bytes
        do
        {
            ++word;
        } while (((*word) & CharType(0xC0)) == CharType(0x80));
        return word;
    case Encoding::UTF16:
        if (((*word) & CharType(0xFC00)) == CharType(0xD800))
            word += 2;
        else
            ++word;
        return word;
    default:
        return ++word;
    }
}

template<Encoding e>
const typename CodeUnit<e>::type* GetNextCharacter(const typename CodeUnit<e>::type* word) noexcept
{
    return StepNextCharacter<e>(word);
}

template<class CharType, class StorageType>
bool StrEnds(StorageType word) noexcept
{
    static_assert(sizeof(StorageType) % sizeof(CharType) == 0, "Size of StorageType should be a multiple of CharType!");
    for (size_t i = 0; i < sizeof(StorageType) / sizeof(CharType); ++i)
    {
        if (((const CharType*)(&word))[i] == (CharType)0)
            return true;
    }
    return false;
}

template<Encoding e, class StorageType>
bool StrEnds(StorageType word)noexcept
{
    return StrEnds<typename CodeUnit<e>::type>(word);
}

template<class CharType1, class CharType2>
bool StrEqual(const CharType1* word1, const CharType2* word2) noexcept
{
    while (*word1 == *word2 && *word1 != CharType1(0))
    {
        ++word1;
        ++word2;
    }
    return *word1 == CharType1(0) && *word2 == CharType2(0);
}

template<Encoding e>
bool StrEqual(const typename CodeUnit<e>::type* word1, const typename CodeUnit<e>::type* word2) noexcept
{
    return StrEqual<typename CodeUnit<e>::type, typename CodeUnit<e>::type>(word1, word2);
}

template<class CharType>
const CharType* StrPrefix(const CharType* word, const CharType* prefix) noexcept
{
    static_assert(std::is_integral<CharType>::value, "");
    while (*word == *prefix && *word != (CharType)0)
    {
        ++word;
        ++prefix;
    }
    return (*prefix == (CharType)0) ? word : nullptr;
}

template<Encoding e>
const typename CodeUnit<e>::type* StrPrefix(const typename CodeUnit<e>::type* word, const typename CodeUnit<e>::type* prefix) noexcept
{
    return StrPrefix<typename CodeUnit<e>::type>(word, prefix);
}

template<class String, class StorageType>
static void CopyStr(const String& str, std::vector<StorageType>& v)
{
    typedef typename String::value_type CharType;
    const auto end = v.size();
    const auto space_required = ((str.size() + 1) * sizeof(CharType) + sizeof(StorageType) - 1) / sizeof(StorageType);
    v.resize(end + space_required, 0);
    CharType* target = (CharType*)(&(v[end]));
    const CharType* s = str.data();
    while (*s)
    {
        *target++ = *s++;
    }
}

template<class TargetType, class SourceType >
std::basic_string<TargetType> Convert(const SourceType* s)
{
    std::basic_string<TargetType> result;
    result.assign(s, s + std::char_traits<SourceType>::length(s));
    return result;
}

template<class CharType, class ResultType>
void ReadFloat(const CharType* s, ResultType& x)
{
    const std::string ascii = Convert<char>(s);
    std::sscanf(ascii.c_str(), std::is_same<ResultType, double>::value ? "%lf" : "%f", &x);
}

template<class CharType, class FloatType>
std::basic_string<CharType> WriteFloat(FloatType&& x)
{
    static char result[32];
    std::snprintf(result, 32, "%g", x);
    return Convert<CharType>(result);
}

template<class CharType, class Index>
std::basic_string<CharType> WriteIndex(Index&& x)
{
    static char result[32];
    std::snprintf(result, 32, "%llu", static_cast<unsigned long long>(x));
    return Convert<CharType>(result);
}

template<Encoding e>
union BOM
{
    const std::array<char, sizeof(typename CodeUnit<e>::type)> bytes;
    const typename CodeUnit<e>::type value;
    BOM() : value((typename CodeUnit<e>::type)(0xFEFF)){}
};

template<> union BOM<Encoding::UTF8>
{
    const std::array<char, 3> bytes;
    BOM() : bytes({ char(0xEF), char(0xBB), char(0xBF) }) {};
};

template<Encoding e>
bool CheckBom(FILE* input) noexcept
{
    static const BOM<e> bom_standard;
    BOM<e> bom_actual;
    return (sizeof(bom_standard) == 1) ||
        (fread((void*)bom_actual.bytes.data(), 1, sizeof(BOM<e>), input) == sizeof(BOM<e>) &&
        bom_actual.bytes == bom_standard.bytes);
}

template<Encoding e>
bool WriteBom(FILE* output) noexcept
{
    static const BOM<e> bom_standard;
    return (sizeof(BOM<e>) == 1) ||
        (fwrite(bom_standard.bytes.data(), 1, sizeof(BOM<e>), output) == sizeof(BOM<e>));
}

}

#ifdef  __GNUC__
// Oh, you bastards!
namespace std {

    template<class _Elem, class _Traits, class _Alloc>
    struct hash<basic_string<_Elem, _Traits, _Alloc>>
    {
        static_assert(sizeof(size_t) == 8 || sizeof(size_t) == 4, "This code is for 32/64 bit only!");
        static const size_t _Val = sizeof(size_t) == 8 ? 14695981039346656037UL : 2166136261U;
        static const size_t _FNV_prime = sizeof(size_t) == 8 ? 1099511628211UL : 16777619U;

        size_t operator()(const basic_string<_Elem, _Traits, _Alloc>& _Keyval) const noexcept
        {
            size_t result = _Val;
            for (const _Elem* first = _Keyval.data(); *first; ++first)
            {
                result ^= static_cast<size_t>(*first);
                result *= _FNV_prime;
            }
            return result;
        }
    };

}

#endif