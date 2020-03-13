#pragma once

#include <cstdint>
#include <cstddef>
#include <type_traits>
#include <vector>
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
#ifdef WIN32
    typedef wchar_t type;
#else
    typedef std::uint16_t type;
#endif
    static const size_t size = sizeof(type);
    type c;
};

// see https://en.cppreference.com/w/cpp/language/types#Character_types
template<>
struct CodeUnit<Encoding::UTF16>
{
#ifdef WIN32
    typedef wchar_t type;
#else
    typedef std::uint16_t type;
#endif
    static const size_t size = sizeof(type);
    type c;
};

// see https://en.cppreference.com/w/cpp/language/types#Character_types
template<>
struct CodeUnit<Encoding::UTF32>
{
#ifdef WIN32
    typedef std::uint32_t type;
#else
    typedef wchar_t type;
#endif
    static const size_t size = sizeof(type);
    type c;
};

template<Encoding e>
const typename CodeUnit<e>::type*& StepNextCharacter(const typename CodeUnit<e>::type*& word)
{
    return ++word;
}

template<>
const typename CodeUnit<Encoding::UTF8>::type*& StepNextCharacter<Encoding::UTF8>(const typename CodeUnit<Encoding::UTF8>::type*& word);

template<>
const typename CodeUnit<Encoding::UTF16>::type*& StepNextCharacter<Encoding::UTF16>(const typename CodeUnit<Encoding::UTF16>::type*& word);

template<Encoding e>
const typename CodeUnit<e>::type* GetNextCharacter(const typename CodeUnit<e>::type* word)
{
    return StepNextCharacter<e>(word);
}

template<class CharType, class StorageType>
bool StrEnds(StorageType word)
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
bool StrEnds(StorageType word)
{
    return StrEnds<typename CodeUnit<e>::type>(word);
}

template<class CharType1, class CharType2>
bool StrEqual(const CharType1* word1, const CharType2* word2)
{
    while (*word1 == *word2 && *word1 != (CharType1)0)
    {
        ++word1;
        ++word2;
    }
    return *word1 == (CharType1)0 && *word2 == (CharType2)0;
}

template<Encoding e>
bool StrEqual(const typename CodeUnit<e>::type* word1, const typename CodeUnit<e>::type* word2)
{
    return StrEqual<typename CodeUnit<e>::type, typename CodeUnit<e>::type>(word1, word2);
}

template<class CharType>
const CharType* StrPrefix(const CharType* word, const CharType* prefix)
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
const typename CodeUnit<e>::type* StrPrefix(const typename CodeUnit<e>::type* word, const typename CodeUnit<e>::type* prefix)
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

union Endianness
{
    unsigned char bytes[4];
    std::uint32_t uint;
};

static const Endianness endianness = { '\x01','\x02', '\x03', '\x04' };

template<class TargetType, class SourceType >
std::basic_string<TargetType> Convert(const SourceType* s)
{
    std::basic_string<TargetType> result;
    result.assign(s, s + std::char_traits<SourceType>::length(s));
    return result;
}

template<class CharType, class SourceType >
std::basic_string<CharType> Convert(const CharType* s)
{
    return std::basic_string<CharType>(s);
}

template<class CharType=char>
struct StrHash
{
private:
    struct FNV
    {
        static const size_t offset = ((sizeof(size_t) == 8) ? 14695981039346656037ULL : 2166136261U);
        static const size_t prime = ((sizeof(size_t) == 8) ? 1099511628211ULL : 16777619U);
    };
public:
    size_t operator()(const CharType* p)const
    {
        size_t result(FNV::offset);
        for (; *p; ++p)
        {
            result ^= (size_t)(*p);
            result *= FNV::prime;
        }
        return result;
    }
};

template<class CharType = char>
struct StrEqualType
{
    bool operator()(const CharType* a, const CharType* b)const
    {
        return StrEqual<CharType, CharType>(a, b);
    }
};

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