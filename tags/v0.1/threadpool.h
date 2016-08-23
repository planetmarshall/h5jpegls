#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <iostream>
#include <map>
#include <chrono>
using std::map;
using std::vector;

class Worker_buffer {
  public:
    Worker_buffer(unsigned char* data=0, size_t size=0)
    : data(data), size(size) {}
    
    unsigned char* data;
    size_t size;
    
};

class ThreadPool {
public:
    ThreadPool(size_t);
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type>;
    ~ThreadPool();
    
    inline unsigned char* get_buffer(int buffer_id, size_t size) {
        int ti = tid[std::this_thread::get_id()];
        if (size > buffers[ti][buffer_id].size) {
            if (buffers[ti][buffer_id].data) {
                free(buffers[ti][buffer_id].data);
            }
            buffers[ti][buffer_id] =
                Worker_buffer((unsigned char*)aligned_alloc(16, size), size);
        }
        return buffers[ti][buffer_id].data;
    }
    
    inline unsigned char* get_global_buffer(int buffer_id, size_t size) {
        std::lock_guard<std::mutex> lock(gb_mutex);
        auto it = global_buffers.find(buffer_id);
        if (it == global_buffers.end()) {
            global_buffers[buffer_id] = Worker_buffer((unsigned char*)aligned_alloc(16, size), size);
            return global_buffers[buffer_id].data;
        } else {
            if (size > it->second.size) {
                free(it->second.data);
                it->second = Worker_buffer((unsigned char*)aligned_alloc(16, size), size);
            }
            return it->second.data;
        }
    }
    
    inline void lock_buffers(void) {
        sleeper_mutex.lock();
        buffers_toggled = true;
    }
    
    inline void unlock_buffers(void) {
        buffers_toggled = true;
        sleeper_mutex.unlock();
        sleeper_condition.notify_one();
    }
    
private:
    // need to keep track of threads so we can join them
    std::vector< std::thread > workers;
    // the task queue
    std::queue< std::function<void()> > tasks;
    
    // synchronization
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
    
    map< std::thread::id, int > tid;
    vector < vector<Worker_buffer> > buffers;
    
    map< int, Worker_buffer > global_buffers;
    std::mutex gb_mutex;
    
    std::thread sleeper;
    std::mutex sleeper_mutex;
    std::condition_variable sleeper_condition;
    bool buffers_toggled;
};

extern ThreadPool* filter_pool;
 
// the constructor just launches some amount of workers
inline ThreadPool::ThreadPool(size_t threads)
    :   stop(false), buffers_toggled(false)
{
    buffers = vector< vector<Worker_buffer> >(threads);
    for(size_t i = 0;i<threads;++i) {
        workers.emplace_back(
            [this]
            {
                for(;;)
                {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock,
                            [this]{ return this->stop || !this->tasks.empty(); });
                        if(this->stop && this->tasks.empty())
                            return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }

                    task();
                }
            }
        );
        
        tid[workers[i].get_id()] = i;
        buffers[i] = vector<Worker_buffer>(2);
    }
    
    sleeper = std::thread( [this] {
        while (true) {
            std::unique_lock<std::mutex> lock(sleeper_mutex);
            buffers_toggled = false;
            sleeper_condition.wait_for(lock, std::chrono::milliseconds(100), [this] {
                return this->stop;
            });
            if (stop) return;
            if (buffers_toggled) continue;

            if (global_buffers.size()) {
                for (auto& bo: buffers) {
                    for (auto& bi: bo) {
                        if (bi.data) {
                            free(bi.data);
                            bi.size = 0;
                            bi.data = 0;
                        }
                    }
                }
                
                for (auto& b: global_buffers) {
                    if (b.second.data) {
                        free(b.second.data);
                        b.second.data = 0;
                    }
                }
                global_buffers.clear();
            }
        }    
    });
    
    sched_param sch_params;
    sch_params.sched_priority = sched_get_priority_min(SCHED_IDLE);
    if (pthread_setschedparam(sleeper.native_handle(), SCHED_IDLE, &sch_params)) {
        printf("Error while trying to demote sleeper thread\n");
    }
}

// add new work item to the pool
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) 
    -> std::future<typename std::result_of<F(Args...)>::type>
{
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared< std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        // don't allow enqueueing after stopping the pool
        if(stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");

        tasks.emplace([task](){ (*task)(); });
    }
    condition.notify_one();
    return res;
}

// the destructor joins all threads
inline ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    sleeper_condition.notify_all();
    for(std::thread &worker: workers) {
        worker.join();
    }
    sleeper.join();
}

#endif
