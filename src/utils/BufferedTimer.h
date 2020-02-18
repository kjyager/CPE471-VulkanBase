#ifndef KJY_BUFFERED_TIMER_H_
#define KJY_BUFFERED_TIMER_H_
#include <chrono>
#include <string>

class BufferedTimer
{
 public:
    BufferedTimer(size_t aTimeBufferSize = 1024U) : mTimeBufferSize(aTimeBufferSize){}

    void startStep();
    void finishStep();
    void reset(); 

    bool isBufferFull() const {return(mStepNumber >= mTimeBufferSize);}

    void reportAndReset() {report(); reset();}
    virtual std::string getReportString() const;
    double currentMeanTime() const;
    double lastStepTime() const;

    size_t getStepNumber() const {return(mStepNumber);}

 protected:
    void report() const;
    
    size_t mStepNumber = 0;
    std::chrono::high_resolution_clock::duration mLastStep{0U};
    std::chrono::high_resolution_clock::duration mTotalTime{0U};
    std::chrono::high_resolution_clock::time_point mStartTime;

    const size_t mTimeBufferSize;
};

class FpsTimer : public BufferedTimer
{
 public:
    using BufferedTimer::BufferedTimer;

    double currentFPS() const; 

    virtual std::string getReportString() const override;

    inline void frameStart() {startStep();}
    inline void frameFinish() {finishStep();}
    inline double currentFrameTime() const {return(currentMeanTime());}

    inline size_t getFrameNumber() const {return(getStepNumber());}
};

/// Time the execution of _STATEMENT and report it along with the contents of _MSG
/// WARNING: Creates an anonymous code block. Any stack variable created by _STATEMENT will be lost!
#define QUICK_TIME(_MSG, _STATEMENT) {\
    BufferedTimer _tmptimer_(1U);\
    _tmptimer_.startStep();\
    _STATEMENT;\
    _tmptimer_.finishStep();\
    std::cout << "QUICK_TIME | " STRIFY(_MSG) ": " << _tmptimer_.getReportString() << std::endl;\
}


#endif