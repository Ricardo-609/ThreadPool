#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

class ThreadPool {
public:
    ThreadPool(size_t threads);
    ~ThreadPool();

    template<typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args)
        ->std::future<typename std::result_of<F(Args...)>::type>;

private:
    std::vector<std::thread> _workers;
    std::queue<std::function<void()>> _tasks;

    std::mutex _queueMutex;
    std::condition_variable _condition;
    bool _stop;
};

inline ThreadPool::ThreadPool(size_t threads) : _stop(false) {
    //设置线程任务
    for (size_t i = 0; i < threads; ++i) {
        //每个线程的任务：1.从任务队列中获取任务（需要加锁）；2.执行任务
        _workers.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->_queueMutex);
                    //等待唤醒，条件是停止或任务队列中有任务
                    this->_condition.wait(lock,
                                        [this]{return this->_stop || !this->_tasks.empty();});
                    if (this->_stop && this->_tasks.empty())    return;
                    task = std::move(this->_tasks.front());
                    this->_tasks.pop();
                }
                task();
            }
        });
    }
}

template<typename F, typename... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args)
    ->std::future<typename std::result_of<F(Args...)>::type>
{
    using return_type = typename std::result_of<F(Args...)>::type;

    //将需要执行的任务函数打包（bind)，转换为参数列表为空的函数对象
    auto task = std::make_shared<std::packaged_task<return_type()>> (
        std::bind(std::forward<F>(f), std::forward<Args...>(args)...)
    );

    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(_queueMutex);

        if (_stop) throw std::runtime_error("enqueue on stopped ThreadPool");

        //最妙的地方，利用lambda函数包装线程函数，使其符合function<void()>的形式，并且返回值可以通过future获取
        _tasks.emplace([task] {
            (*task)();
        });
    }

    _condition.notify_all();
    return res;
}
inline ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(_queueMutex);
        _stop = true;
    }

    _condition.notify_all();
    for (std::thread& worker : _workers) {
        worker.join();
    }
}