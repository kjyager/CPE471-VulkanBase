#include "FpsTimer.h"
#include <iostream>
#include <sstream>
#include <iomanip>

void FpsTimer::frameStart(){
    mStartTime = std::chrono::high_resolution_clock::now();
}
void FpsTimer::frameFinish(){
    auto finish = std::chrono::high_resolution_clock::now();
    mTotalTime += finish-mStartTime;
    ++mFrameNumber;
}
void FpsTimer::reset(){
    mTotalTime = std::chrono::high_resolution_clock::duration(0);
    mFrameNumber = 0;
}

void FpsTimer::report() const{
    std::cout << getReportString() << std::endl;
}

std::string FpsTimer::getReportString() const{
    std::ostringstream reportBuilder;
    reportBuilder.setf(std::ios_base::fixed, std::ios_base::floatfield);
    reportBuilder.precision(3);
    reportBuilder << currentFPS() << " fps ("  << currentFrameTime() << " microseconds)";
    return(reportBuilder.str());
}

double FpsTimer::currentFPS() const{
    double averageFrameTimeMicro = currentFrameTime();
    return(1e6/averageFrameTimeMicro);
}

double FpsTimer::currentFrameTime() const{
    uint64_t totalMicro = std::chrono::duration_cast<std::chrono::microseconds>(mTotalTime).count();
    double denominator = static_cast<double>(mTimeBufferSize > 0 ? mTimeBufferSize : mFrameNumber);
    double averageFrametimeMicro = static_cast<double>(totalMicro) / denominator;
    return(averageFrametimeMicro);
}
