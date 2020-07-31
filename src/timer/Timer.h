//
// Created by 杨成进 on 2020/7/30.
//

#ifndef MYTINYWEBSERVER_TIMER_H
#define MYTINYWEBSERVER_TIMER_H

#include <unordered_map>
#include <functional>
#include <chrono>
#include <queue>
#include <vector>
#include <algorithm>
#include <assert.h>

typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::microseconds MS;
typedef Clock::time_point TimeStamp;

struct TimerNode {
    int id;
    TimeStamp expires;
    TimeoutCallBack callBack;

    bool operator<(const TimerNode &t) const {
        return expires < t.expires;
    }
};

class Timer {
public:
    Timer();

    ~Timer();

    void adjust(int id, int newExpires);

    void add(int id, int timeOut, const TimeoutCallBack &cb);

    void clear();

    void tick();

    void pop();

    int GetNextTick();

private:

    void del_(size_t index);

    void siftup_(size_t i);

    bool siftdown_(size_t index, size_t n);

    void SwapNode_(size_t i, size_t j);

    std::vector<TimerNode> heap_;
    std::unordered_map<int, size_t> ref_;
};


#endif //MYTINYWEBSERVER_TIMER_H
