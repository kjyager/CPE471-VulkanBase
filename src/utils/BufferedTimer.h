#ifndef KJY_BUFFERED_TIMER_H_
#define KJY_BUFFERED_TIMER_H_
#include <chrono>
#include <string>
#include <vector>
#include <stack>

class BufferedTimer
{
 public:
    BufferedTimer(size_t aTimeBufferSize = 1024U) : mTimeBufferSize(aTimeBufferSize){}
    virtual ~BufferedTimer() = default;

    void startStep();
    void finishStep();
    void reset(); 

    bool isBufferFull() const {return(mStepNumber >= mTimeBufferSize);}

    void reportAndReset() {report(); reset();}
    virtual std::string getReportString() const;

    /// Current mean average time per step in microseconds
    double currentMeanTime() const;

    /// Duration of last step in microseconds
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
    virtual ~FpsTimer() = default;

    double currentFPS() const; 

    virtual std::string getReportString() const override;

    /// Delimit the start of a new frame
    inline void frameStart() {startStep();}
    /// Delimit the end of a frame
    inline void frameFinish() {finishStep();}

    /// Returns the time average time per frame in microseconds
    inline double currentFrameTime() const {return(currentMeanTime());}

    /// Returns the number of frames tracked by this timer 
    inline size_t getFrameNumber() const {return(getStepNumber());}
};

namespace _internal{
    static std::vector<BufferedTimer> quickTimerPool;
    static std::stack<size_t> quickTimerStack; 

    inline static void quick_timer_push(){
        quickTimerStack.push(quickTimerPool.size());
        quickTimerPool.emplace_back(1U);
    }

    inline static void quick_timer_pop(){
        quickTimerStack.pop();
        quickTimerPool.pop_back();
    }

    inline static BufferedTimer& quick_timer_top(){ return(quickTimerPool[quickTimerStack.top()]);}
}

/// Time the execution of _STATEMENT and report it along with the contents of _MSG
#define QUICK_TIME(_MSG, _STATEMENT) \
    _internal::quick_timer_push();\
    _internal::quick_timer_top().startStep();\
    _STATEMENT;\
    _internal::quick_timer_top().finishStep();\
    std::cout \
        << "QUICK_TIME | " << std::string((_internal::quickTimerStack.size()-1)*2, ' ')\
        << STRIFY(_MSG) ": " << _internal::quick_timer_top().getReportString()\
    << std::endl;\
    _internal::quick_timer_pop();


#endif