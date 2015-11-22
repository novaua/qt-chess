// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifdef WIN32
#include "targetver.h"
#endif

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include <iostream>
#include <memory>
#include <vector>
#include <functional>
#include <cassert>
#include <ostream>
#include <fstream>

#ifdef WIN32
#include <filesystem>
#endif

#include <stack>
#include <list>
#include <set>
#include <map>
#include <queue>

#include <mutex>
#include <thread>
#include <numeric>
#include <iterator>
#include <algorithm>
#include <random>
#include <strstream>

#ifndef WIN32
namespace std {
    template <typename T>
    std::unique_ptr<T> make_unique()
    {
        std::unique_ptr<T> result(new T());
        return result;
    }
}
#endif
