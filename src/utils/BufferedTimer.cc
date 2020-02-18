#include "BufferedTimer.h"
#include <iostream>
#include <sstream>
#include <iomanip>

void BufferedTimer::startStep(){
    mStartTime = std::chrono::high_resolution_clock::now();
}
void BufferedTimer::finishStep(){
    auto finish = std::chrono::high_resolution_clock::now();
    mLastStep = finish-mStartTime;
    mTotalTime += mLastStep;
    ++mStepNumber;
}
void BufferedTimer::reset(){
    mTotalTime = std::chrono::high_resolution_clock::duration(0);
    mStepNumber = 0;
}

void BufferedTimer::report() const{
    std::cout << getReportString() << std::endl;
}

std::string BufferedTimer::getReportString() const{
    std::ostringstream reportBuilder;
    reportBuilder.setf(std::ios_base::fixed, std::ios_base::floatfield);
    reportBuilder.precision(3);
    reportBuilder << " fps ("  << currentMeanTime() << " microseconds)";
    return(reportBuilder.str());
}

double BufferedTimer::currentMeanTime() const{
    uint64_t totalMicro = std::chrono::duration_cast<std::chrono::microseconds>(mTotalTime).count();
    double denominator = static_cast<double>(mTimeBufferSize > 0 ? mTimeBufferSize : mStepNumber);
    double averageFrametimeMicro = static_cast<double>(totalMicro) / denominator;
    return(averageFrametimeMicro);
}



// FpsTimer

double FpsTimer::currentFPS() const{
    double averageFrameTimeMicro = currentFrameTime();
    return(1e6/averageFrameTimeMicro);
} 

std::string FpsTimer::getReportString() const {
    std::ostringstream reportBuilder;
    reportBuilder.setf(std::ios_base::fixed, std::ios_base::floatfield);
    reportBuilder.precision(3);
    reportBuilder << currentFPS() << " fps ("  << currentFrameTime() << " microseconds)";
    return(reportBuilder.str());
}