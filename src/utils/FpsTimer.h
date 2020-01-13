#ifndef FPS_COUNTER_H_
#define FPS_COUNTER_H_
#include <chrono>
#include <string>

class FpsTimer
{
 public:
    FpsTimer(size_t aTimeBufferSize = 1024U) : mTimeBufferSize(aTimeBufferSize){}

    void frameStart();
    void frameFinish();
    void reset(); 

    bool isBufferFull() const {return(mFrameNumber >= mTimeBufferSize);}
    void reportAndReset() {report(); reset();}
    std::string getReportString() const;
    double currentFPS() const; 
    double currentFrameTime() const;

    size_t getFrameNumber() const {return(mFrameNumber);}

 protected:
    void report() const;
    
    size_t mFrameNumber = 0;
    std::chrono::high_resolution_clock::duration mTotalTime{0U};
    std::chrono::high_resolution_clock::time_point mStartTime;

    const size_t mTimeBufferSize;
};



#endif