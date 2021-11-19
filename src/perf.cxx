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

#include "perf.hxx"
#include "logging.hxx"

TimePerf::TimePerf(const char* sMessage)
{
    nStart = std::chrono::high_resolution_clock::now();
    pStr = sMessage;
}

void TimePerf::Stop()
{
    nEnd = std::chrono::high_resolution_clock::now();
    float fSec = float(std::chrono::duration_cast<std::chrono::milliseconds>(nEnd - nStart).count())
                 / 1000.0;
    writeLog("DEBUG>>> Finished %s in %.4f seconds.\n", pStr, fSec);
}
