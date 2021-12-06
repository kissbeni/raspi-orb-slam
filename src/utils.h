
#ifndef _UTILS_H
#define _UTILS_H

#include <iostream>
#include <type_traits>

// https://stackoverflow.com/a/23815961

template <typename Arg1, typename Arg2>
constexpr typename std::common_type<Arg1, Arg2>::type vmax(Arg1&& arg1, Arg2&& arg2)
{
    return arg1 > arg2 ? std::forward<Arg1>(arg1) : std::forward<Arg2>(arg2);
}

template <typename Arg, typename... Args>
constexpr typename std::common_type<Arg, Args...>::type vmax(Arg&& arg, Args&&... args)
{
    return vmax(std::forward<Arg>(arg), vmax(std::forward<Args>(args)...));
}

#endif
