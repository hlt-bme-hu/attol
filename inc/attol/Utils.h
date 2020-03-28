#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <sstream>
#include <exception>
#include <chrono>
#include <limits>
#include <typeinfo>
#include <utility>
#include <list>
#include <functional>

namespace attol{

enum FlagStrategy
{
    IGNORE,
    OBEY,
    NEGATIVE
};

template<class Arg>
struct Handlers
{
    std::list<std::function<void(const Arg&)>> handlers;
    Handlers() : handlers(0) {}
    void operator()(const Arg& path)const
    {
        for (auto& f : handlers)
            f(path);
    }
    Handlers& operator+=(const Handlers& other)
    {
        handlers.emplace(handlers.end(),
            other.handlers.begin(),
            other.handlers.end());
        return *this;
    }
    Handlers& operator+=(const std::function<void(const Arg&)>& g)
    {
        handlers.emplace_back(g);
        return *this;
    }
    Handlers operator+(const Handlers& other)const
    {
        Handlers result(*this);
        return result += other;
    }
    Handlers operator+(const std::function<void(const Arg&)>& g)const
    {
        Handlers result(*this);
        return result += g;
    }
};

template<class Return, class Arg>
Return IntLog2(Arg s)
{
    s -= 1;
    Return r = 0;
    while (s)
    {
        ++r;
        s >>= 1;
    }
    return r;
}

//! simple class to measure time, not thread safe!
template<class ClockTy = std::chrono::steady_clock>
class Clock
{
public:
    //! marks instantiation time
    Clock()
    {
        Tick();
    }
    //! marks current time
    void Tick()
    {
        _timePoint = MyClock::now();
    }
    //! returns time between former time mark and now
    /*!
    @return time since last Tick or construction.
    */
    double Tock()
    {
        auto const now = MyClock::now();
        const auto elapsed = _frequency * (now - _timePoint).count();
        return elapsed;
    }
    //! returns time between former time marker and now, also resets time marker to 'now'
    /*!
    */
    double Tack()
    {
        auto const now = MyClock::now();
        const auto elapsed = _frequency * (now - _timePoint).count();
        _timePoint = now;
        return elapsed;
    }
    ~Clock() {}
private:
    typedef ClockTy MyClock;
    typedef typename MyClock::duration MyDuration;
    typename MyClock::time_point _timePoint;
    static constexpr double _frequency = (double)MyDuration::period::num / MyDuration::period::den;
};

template<typename Arg1>
std::string ToStr(Arg1 arg1)
{
    std::ostringstream oss;
    oss << arg1;
    return oss.str();
}

template<typename Arg1, typename... Args>
std::string ToStr(Arg1 arg1, Args... args)
{
    return ToStr(arg1) + ToStr(args...);
}

class Error : public std::exception
{
public:
    template<typename ...Args>
    explicit Error(Args... args)
        : msg(ToStr(args...))
    {
    }
    virtual const char* what() const throw() { return msg.c_str(); }
private:
    std::string msg;
};

template<class TargetType, bool enable = true>
struct SaturateCast
{
    template<class SourceType>
    static TargetType Do(SourceType i)
    {
        // TODO assert both numeric
        if (enable && std::numeric_limits<SourceType>::min() < std::numeric_limits<TargetType>::min())
        {
            if (i < std::numeric_limits<TargetType>::min())
                throw Error("SaturateCast error: ", i, " of type ", typeid(SourceType).name(), " is smaller then minimum of ", typeid(TargetType).name());
        }
        if (enable && std::numeric_limits<SourceType>::max() > std::numeric_limits<TargetType>::max())
        {
            if (i > std::numeric_limits<TargetType>::max())
                throw Error("SaturateCast error: ", i, " of type ", typeid(SourceType).name(), " is greater then maximum of ", typeid(TargetType).name());
        }
        return static_cast<TargetType>(i);
    }
};

template<class T, class Count = size_t, class Hasher = std::hash<T>, class Equaler = std::equal_to<T>>
class Counter : public std::unordered_map<T, Count, Hasher, Equaler>
{
public:
    template<typename KeyArg>
    Count operator[](KeyArg&& key)
    {
        return this->emplace(std::forward<KeyArg>(key), SaturateCast<Count>::Do(this->size())).first->second;
    }
};

template<class StorageType>
struct SignedBitfield
{
    static_assert(std::is_signed<StorageType>::value, "");
    SignedBitfield() : bitfield(0) {}
    SignedBitfield(const StorageType& b) : bitfield(b) {}
    //! returns a mask, where there are 1's in places [i, j)
    static inline StorageType Mask(unsigned char i, unsigned char j)
    {
        // mask 0000001111000000
        //      5432109876543210
        //           j   i      
        return (((StorageType)1 << (j - i)) - (StorageType)1) << i;
    }
    StorageType Get(unsigned char i, unsigned char j)const
    {
        auto x = (((StorageType)1 << (j - i)) - (StorageType)1) & (bitfield >> i);
        if (x >> (j - i - 1))
        {   // negative value
            x |= (StorageType)(-1) << (j - i);
        }
        return x;
    }
    void Set(unsigned char i, unsigned char j, StorageType new_value)
    {
        const auto mask = Mask(i, j);
        // delete what's in place
        bitfield &= ~mask;
        // add new value to the right place
        bitfield |= mask & (new_value << i);
    }
    StorageType GetRaw()const
    {
        return bitfield;
    }
    void clear()
    {
        bitfield = 0;
    }
protected:
    StorageType bitfield;
};

}
