#ifndef __SRC_THREAD_THREADGROUP_H_
#define __SRC_THREAD_THREADGROUP_H_

#include <memory>
#include <stdexcept>
#include <thread>
#include <unordered_map>
class threadGroup {
private:
    std::thread::id _thread_id;
    std::unordered_map<std::thread::id, std::shared_ptr<std::thread>> _threads;
    threadGroup(threadGroup const &);
    threadGroup &operator=(threadGroup const &);

public:
    threadGroup() = default;;
    
    ~threadGroup() {
        _threads.clear();
    }

    bool is_this_thread_in() {
        auto thread_id = std::this_thread::get_id();
        if(_thread_id == thread_id) {
            return true;
        }
        return _threads.find(thread_id) != _threads.end();
    }

    bool is_thread_in(std::thread *t) {
        if(!t) {
            return false;
        }
        auto it = _threads.find(t ->get_id());
        return it != _threads.end();
    }

    template<typename F>
    std::thread *create_thread(F &&tfunc) {
        auto new_th = std::make_shared<std::thread>(tfunc);
        _thread_id = new_th -> get_id();
        _threads[_thread_id] = new_th;
        return new_th.get();
    }

    void remove_thread(std::thread *t) {
        auto it = _threads.find(t ->get_id());
        if(it != _threads.end()) {
            _threads.erase(it);
        }
    }

    void join_all() {
        if(is_this_thread_in()) {
            throw std::runtime_error("thereadGroup : trying joining itself");
        }
        for(auto& it : _threads) {
            if(it.second -> joinable()) {
                it.second -> join();
            }
        }
        _threads.clear();
    }

    size_t size(){
        return _threads.size();
    }
};

#endif /* __SRC_THREAD_THREADGROUP_H_ */
