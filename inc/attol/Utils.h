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

namespace attol{

enum FlagStrategy
{
    IGNORE,
    OBEY,
    NEGATIVE
};

//template<size_t width>
//struct Round
//{
//    template<class Number>
//    static Number Do(Number i)
//    {
//        static const Number mask1 = width - 1;
//        static const Number mask2 = ~mask1;
//        return (i + mask1) & mask2;
//    }
//};

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

}
