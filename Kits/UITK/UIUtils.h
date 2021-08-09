#pragma once

#include <functional>
#include <type_traits>
#include <chrono>

template<typename ...Ts>
std::string concat(Ts const& ... args)
{
    std::stringstream ss;
    (ss << ... << args);
    return ss.str();
}

template<typename T, typename Callable>
void diff(std::shared_ptr<T> a, std::shared_ptr<T> b, Callable&& onDiff)
{
    static int count = 0;
    if (a != b)
    {
        onDiff();
        LOG(++count);
    }
}
