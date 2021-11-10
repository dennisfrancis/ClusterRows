#include <cstdarg>
#include <cstdio>
#include "logging.hxx"

void writeLog(const char* format, ...)
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
