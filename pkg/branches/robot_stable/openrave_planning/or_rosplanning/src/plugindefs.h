// Copyright (C) 2006-2008 Rosen Diankov (rdiankov@cs.cmu.edu)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
#ifndef RRT_PLANNER_STDAFX
#define RRT_PLANNER_STDAFX

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE

#include <assert.h>
#include <cstdio>
#include <cmath>
#include <cstdlib>

#ifdef _MSC_VER
#include <boost/typeof/std/string.hpp>
#include <boost/typeof/std/vector.hpp>
#include <boost/typeof/std/list.hpp>
#include <boost/typeof/std/map.hpp>
#include <boost/typeof/std/string.hpp>

#define FOREACH(it, v) for(BOOST_TYPEOF(v)::iterator it = (v).begin(); it != (v).end(); (it)++)
#define FOREACHC(it, v) for(BOOST_TYPEOF(v)::const_iterator it = (v).begin(); it != (v).end(); (it)++)
#define RAVE_REGISTER_BOOST
#else

#include <string>
#include <vector>
#include <list>
#include <map>
#include <string>

#define FOREACH(it, v) for(typeof((v).begin()) it = (v).begin(); it != (v).end(); (it)++)
#define FOREACHC FOREACH

#endif

#include <fstream>
#include <iostream>
#include <sstream>

using namespace std;
typedef unsigned int u32;

#include <sys/timeb.h>    // ftime(), struct timeb

// declaring variables with stdcall can be a little complex
#ifdef _MSC_VER

#define PROT_STDCALL(name, paramlist) __stdcall name paramlist
#define DECL_STDCALL(name, paramlist) __stdcall name paramlist

#else

#ifdef __x86_64__
#define DECL_STDCALL(name, paramlist) name paramlist
#else
#define DECL_STDCALL(name, paramlist) __attribute__((stdcall)) name paramlist
#endif

#endif // _MSC_VER

template<class T>
inline T CLAMP_ON_RANGE(T value, T min, T max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

inline unsigned long timeGetTime()
{
#ifdef _WIN32
    _timeb t;
    _ftime(&t);
#else
    timeb t;
    ftime(&t);
#endif

    return (unsigned long)(t.time*1000+t.millitm);
}

inline float RANDOM_FLOAT()
{
#if defined(__IRIX__)
    return drand48();
#else
    return rand()/((float)RAND_MAX);
#endif
}

inline float RANDOM_FLOAT(float maximum)
{
#if defined(__IRIX__)
    return (drand48() * maximum);
#else
    return (RANDOM_FLOAT() * maximum);
#endif
}

inline int RANDOM_INT(int maximum)
{
#if defined(__IRIX__)
    return (random() % maximum);
#else
    return (rand() % maximum);
#endif
}

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x)/(sizeof( (x)[0] )))
#endif

#define FORIT(it, v) for(it = (v).begin(); it != (v).end(); (it)++)

#ifdef _WIN32

#define WCSTOK(str, delim, ptr) wcstok(str, delim)

// define wcsicmp for MAC OS X
#elif defined(__APPLE_CC__)

#define WCSTOK(str, delim, ptr) wcstok(str, delim, ptr);

#define strnicmp strncasecmp
#define stricmp strcasecmp

inline int wcsicmp(const wchar_t* s1, const wchar_t* s2)
{
  char str1[128], str2[128];
  sprintf(str1, "%S", s1);
  sprintf(str2, "%S", s2);
  return stricmp(str1, str2);
}


#else

#define WCSTOK(str, delim, ptr) wcstok(str, delim, ptr)

#define strnicmp strncasecmp
#define stricmp strcasecmp
#define wcsnicmp wcsncasecmp
#define wcsicmp wcscasecmp

#endif

#include <rave/rave.h>
using namespace OpenRAVE;

extern EnvironmentBase* g_pEnviron;

#endif
