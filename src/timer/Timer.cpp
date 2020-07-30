//
// Created by 杨成进 on 2020/7/30.
//

#include "Timer.h"

Timer::Timer() {
    /*
     *reserver函数用来给vector预分配存储区大小，即capacity的值 ，
     *但是没有给这段内存进行初始化。reserve 的参数n是推荐预分配内存的大小，
     *实际分配的可能等于或大于这个值，即n大于capacity的值，就会reallocate内存 capacity的值会大于或者等于n 。
     *这样，当vector调用push_back函数使得size 超过原来的默认分配的capacity值时 避免了内存重分配开销。
     */
    heap_.reserve(64);
}

Timer::~Timer() {
    clear();
}

void Timer::add(int id, int timeOut, const TimeoutCallBack &cb) {
    assert(id >= 0);
    size_t i;
    //新节点 在堆尾插入并调整
    if (ref_.count(id) > 0) {
        i = heap_.size();
        ref_[id] = i;
        heap_.push_back({id, Clock::now() + MS(timeOut), cb});
        siftup_(i);
    } else {
        //已有节点 调整堆
        i = ref_[id];
        heap_[i].expires = Clock::now() + MS(timeOut);
        heap_[i].callBack = cb;
        if (!siftdown_(i, heap_.size()))
            siftup_(i);
    }
}

void Timer::clear() {
    ref_.clear();
    heap_.clear();
}

//清除超时节点
void Timer::tick() {
    if (heap_.empty()) return;
    while (!heap_.empty()) {
        TimerNode node = heap_.front();
        if (std::chrono::duration_cast<MS>(node.expires - Clock::now()).count() > 0)
            break;
        node.callBack();
        pop();
    }
}

void Timer::pop() {
    del_(0);
}

int Timer::GetNextTick() {
    tick();
    size_t res = -1;
    if(!heap_.empty()){
        res = std::chrono::duration_cast<MS>(heap_.front().expires - Clock::now()).count();
        if(res < 0) res = 0;
    }
    return res;
}

//删除指定位置元素，先将要删除元素移至尾部，然后调整堆
void Timer::del_(size_t index) {
    size_t i = index;
    size_t n = heap_.size() - 1;
    assert(i <= n);
    if (i < n) {
        SwapNode_(i, n);
        //如未下移 则需要上移
        if (!siftdown_(i, n))
            siftup_(i);
    }
    ref_.erase(heap_.back().id);
    heap_.pop_back();
}

//调整指定id节点
void Timer::adjust(int id, int timeout) {
    assert(!heap_.empty() && ref_.count(id) > 0);
    heap_[ref_[id]].expires = Clock::now() + MS(timeout);
    siftdown_(ref_[id], heap_.size());
}

void Timer::siftup_(size_t i) {
    assert(i >= 0 && i < heap_.size());
    size_t j = (i - 1) / 2;
    while (j >= 0) {
        if (heap_[j] < heap_[i]) break;
        if (heap_[i] < heap_[j]) {
            SwapNode_(i, j);
            i = j;
            j = (i - 1) / 2;
        }
    }
}

bool Timer::siftdown_(size_t index, size_t n) {
    assert(index >= 0 && index < heap_.size()
           && n >= 0 && n <= heap_.size());
    size_t i = index;
    size_t j = i * 2 + 1;
    while (j < n) {
        if (j + 1 < n && heap_[j + 1] < heap_[j]) j++;
        if (heap_[i] < heap_[j]) break;
        SwapNode_(i, j);
        i = j;
        j = i * 2 + 1;
    }
    return i > index;
}

void Timer::SwapNode_(size_t i, size_t j) {
    assert(i >= 0 && i < heap_.size()
           && j >= 0 && j < heap_.size());
    ref_[heap_[i].id] = j;
    ref_[heap_[j].id] = i;
    std::swap(heap_[i], heap_[j]);
}
