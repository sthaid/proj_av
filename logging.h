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
