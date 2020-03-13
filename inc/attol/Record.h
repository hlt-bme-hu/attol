#pragma once

#include <tuple>
#include <type_traits>

#include "attol/char.h"

namespace attol {

template<class StorageType = unsigned int, class CharType = char>
class RecordIterator
{
    static_assert(std::is_integral<CharType>::value, "");
    static_assert(sizeof(StorageType) % sizeof(CharType) == 0, "");
    // TODO works only if sizeof(StorageType) == 4
public:
    RecordIterator(StorageType* p) : p(p), s_p(nullptr){}
    StorageType& GetId() const { return *p; }
    StorageType& GetFrom() const { return *(p + 1); }
    StorageType& GetTo() const { return *(p + 2); }
    
    float GetWeight() const { return *reinterpret_cast<float*>(p + 3); }
    CharType* GetInput() const { return reinterpret_cast<CharType*>(&(*(p + 4))); }
    CharType* GetOutput()
    {
        if (!s_p)
        {
            s_p = p + 4;
            while (!StrEnds<CharType, StorageType>(*s_p))
                ++s_p;
            ++s_p;
        }
        return reinterpret_cast<CharType*>(s_p);
    }
    RecordIterator& operator++()
    {
        GetOutput();
        while (!StrEnds<CharType, StorageType>(*s_p))
            ++s_p;
        p = ++s_p;
        s_p = nullptr;
        return *this;
    }
    bool operator<(const RecordIterator<StorageType, CharType>& other)const
    {
        return p < other.p;
    }
    bool operator<(const StorageType* other)const
    {
        return p < other;
    }
private:
    StorageType* p;
    StorageType* s_p;
};

template<class StorageType = unsigned int, class CharType = char>
class CRecordIterator
{
    static_assert(std::is_integral<CharType>::value, "");
    static_assert(sizeof(StorageType) % sizeof(CharType) == 0, "");
    // TODO works only if sizeof(StorageType) == 4
public:
    CRecordIterator(const StorageType* p) : p(p), s_p(nullptr) {}
    const StorageType& GetId() const { return *p; }
    const StorageType& GetFrom() const { return *(p + 1); }
    const StorageType& GetTo() const { return *(p + 2); }

    float GetWeight() const { return *reinterpret_cast<const float*>(p + 3); }
    const CharType* GetInput() const { return reinterpret_cast<const CharType*>(&(*(p + 4))); }
    const CharType* GetOutput()
    {
        if (!s_p)
        {
            s_p = p + 4;
            while (!StrEnds<CharType, StorageType>(*s_p))
                ++s_p;
            ++s_p;
        }
        return reinterpret_cast<const CharType*>(s_p);
    }
    CRecordIterator& operator++()
    {
        GetOutput();
        while (!StrEnds<CharType, StorageType>(*s_p))
            ++s_p;
        p = ++s_p;
        s_p = nullptr;
        return *this;
    }
    bool operator<(const CRecordIterator<StorageType, CharType>& other)const
    {
        return p < other.p;
    }
    bool operator<(const StorageType* other)const
    {
        return p < other;
    }
private:
    const StorageType * p;
    const StorageType* s_p;
};

}
