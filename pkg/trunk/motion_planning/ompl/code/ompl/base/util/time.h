/*********************************************************************
* Software License Agreement (BSD License)
* 
*  Copyright (c) 2008, Willow Garage, Inc.
*  All rights reserved.
* 
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
* 
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the Willow Garage nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
* 
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*********************************************************************/

/* Taken from ROS implementation */
#ifndef OMPL_TIME_
#define OMPL_TIME_

#include <iostream>
#include <cmath>
#include "ompl/base/util/types.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace ompl
{

    namespace time_utils
    {
	
	class Duration
	{
	public:
	    int32_t sec, nsec;
	Duration() : sec(0), nsec(0) { }
	    Duration(int32_t _sec, int32_t _nsec);
	    Duration(double d);
	    Duration(long long t);
	    Duration operator+(const Duration &rhs);
	    Duration operator-(const Duration &rhs);
	    bool operator==(const Duration &rhs) const;
	    bool operator>(const Duration &rhs) const;
	    bool operator<(const Duration &rhs) const;
	    double to_double() { return (double)sec + 1e-9*(double)nsec; }
	    int64_t to_ll() {return (int64_t)sec*1000000000ll + (int64_t)nsec;  }
	    /** sleeps for the time duration specified by this object */
	    bool sleep();
	};
	
	class Time
	{
	public:
	    uint32_t sec, nsec;
#ifdef _WIN32
	    // unless I've missed something obvious, the only way to get high-precision
	    // time on Windows is via the QueryPerformanceCounter() call. However,
	    // this is somewhat problematic in Windows XP on some processors, especially 
	    // AMD, because the Windows implementation can freak out when the CPU clocks
	    // down to save power. Time can jump or even go backwards. Microsoft has
	    // fixed this bug for most systems now, but it can still show up if you have
	    // not installed the latest CPU drivers (an oxymoron). They fixed all these
	    // problems in Windows Vista, and this API is by far the most accurate that
	    // I know of in Windows, so I'll use it here despite all these caveats
	    static LARGE_INTEGER cpu_freq, init_cpu_time;
	    static Time start_time;
#endif
	Time() : sec(0), nsec(0) { }
	Time(uint32_t _sec, uint32_t _nsec) : sec(_sec), nsec(_nsec) { }
	Time(unsigned long long t) : sec(t / 1000000000), nsec(t % 1000000000) { }
	    Duration operator-(const Time &rhs);
	    Time operator+(const Duration &rhs);
	    bool operator==(const Time &rhs) const;
	    bool operator>(const Time &rhs) const;
	    bool operator<(const Time &rhs) const;
	    static Time now();
	    double to_double() { return (double)sec + 1e-9*(double)nsec; }
	    void from_double(double t) { sec = (uint32_t)floor(t); nsec = (uint32_t)round((t-sec) * 1e9);}
	    uint64_t to_ull() {return (uint64_t)sec*1000000000ull + (uint64_t)nsec;  }
	    friend std::ostream &operator <<(std::ostream &os, const Time &rhs);
	    inline bool is_zero() { return sec == 0 && nsec == 0; }
	};	
    }

    std::ostream &operator <<(std::ostream &os, const time_utils::Time &rhs);
    
}

#endif
    
