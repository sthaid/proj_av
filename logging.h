/*
Copyright (c) 2015 Steven Haid

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef __LOGGING_H__
#define __LOGGING_H__

#include <iostream>
#include <sstream>

using std::cout;
using std::endl;

//#define ENABLE_LOGGING_AT_DEBUG_LEVEL

#define INFO(x) \
    do { \
        std::ostringstream _s; \
        _s << "INFO " << __func__ << ": " << x; \
        cout << _s.str(); \
    } while (0)

#define WARNING(x) \
    do { \
        std::ostringstream _s; \
        _s << "WARNING " << __func__ << ": " << x; \
        cout << _s.str(); \
    } while (0)

#define ERROR(x) \
    do { \
        std::ostringstream _s; \
        _s << "ERROR " << __func__ << ": " << x; \
        cout << _s.str(); \
    } while (0)

#define FATAL(x) \
    do { \
        std::ostringstream _s; \
        _s << "FATAL " << __func__ << ": " << x; \
        cout << _s.str(); \
        abort(); \
    } while (0)

#ifdef ENABLE_LOGGING_AT_DEBUG_LEVEL
    #define DEBUG(x) \
        do { \
            std::ostringstream _s; \
            _s << "DEBUG " << __func__ << ": " << x; \
            cout << _s.str(); \
        } while (0)
#else
    #define DEBUG(x) 
#endif

#endif
