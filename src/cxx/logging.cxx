/*
 * ClusterRows
 * Copyright (c) 2021 Dennis Francis <dennisfrancis.in@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <logging.hxx>

#include <cstdarg>
#include <cstdio>

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

void logError(const char* pStr) { writeLog("ERROR: %s", pStr); }
