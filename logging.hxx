#ifndef COM_GITHUB_DENNISFRANCIS_LOGGING_HXX
#define COM_GITHUB_DENNISFRANCIS_LOGGING_HXX

#include <cstdarg>
#include <cstdio>

static void writeLog(const char *format, ...)
{
    #ifdef LOGGING_ENABLED
    {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        fflush(stdout);
        va_end(args);
    }
    #else
    {
        (void)format;
    }
    #endif
}

#endif