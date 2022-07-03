#include "Thread/ThreadPool.h"
#include <chrono>
#include <iostream>

using tp = std::chrono::system_clock::time_point;
using td = std::chrono::system_clock::duration;
using std::cout;
using std::endl;

ThreadPool tpool;

int main() {
    cout << "RUN THREADPOOL FUNCTION TEST" << endl;
    
    return 0;
}