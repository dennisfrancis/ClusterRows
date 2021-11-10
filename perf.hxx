#pragma once

#include <chrono>
#include "logging.hxx"

class TimePerf
{
public:
    TimePerf(const char* sMessage);
    void Stop();

private:
    std::chrono::high_resolution_clock::time_point nStart, nEnd;
    const char* pStr;
};

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
