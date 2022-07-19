#include "Thread/ThreadPool.h"
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <sstream>
#include <ostream>

using tp = std::chrono::system_clock::time_point;
using td = std::chrono::system_clock::duration;
using std::cout;
using std::endl;

ThreadPool tpool{8};

int main() {
    cout << "RUN THREADPOOL FUNCTION TEST" << endl;
    std::unordered_map<std::thread::id, int> cnt;
    std::mutex mtx;
    tp start_time = std::chrono::system_clock::now();
    for (int i = 0; i < 1e6; i++) {
        tpool.addTask([&] {
            std::unique_lock<std::mutex> lck(mtx);
            cnt[std::this_thread::get_id()] ++;
            // cout << i << " ";
        });
    }
    tpool.stop();
    tp end_time = std::chrono::system_clock::now();
    td duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    int total = 0;
    for(auto [k, v] : cnt) {
        cout << "Thread " << k << " executed " << v << " tasks." << endl;
        total += v;
    }
    cout << "total tasks : " << total << ", Cost time : " << duration.count() << "ms" << endl;
    return 0;
}