#ifndef __LOGGING_H__
#define __LOGGING_H__

#include <iostream>

using std::cout;
using std::endl;

//#define ENABLE_LOGGING_AT_DEBUG_LEVEL

#define INFO(x) \
    do { \
        cout << "INFO " << __func__ << ": " << x; \
    } while (0)
#define INFO_CONT(x) \
    do { \
        cout << x; \
    } while (0)

#define WARNING(x) \
    do { \
        cout << "WARNING " << __func__ << ": " << x; \
    } while (0)
#define ERROR(x) \
    do { \
        cout << "ERROR " << __func__ << ": " << x; \
    } while (0)
#define FATAL(x) \
    do { \
        cout << "FATAL " << __func__ << ": " << x; \
        abort(); \
    } while (0)

#ifdef ENABLE_LOGGING_AT_DEBUG_LEVEL
#define DEBUG(x) \
    do { \
        cout << "DEBUG " << __func__ << ": " << x; \
    } while (0)
#else
#define DEBUG(x) 
#endif

#endif
