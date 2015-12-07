#ifndef WORKERTHREAD_H
#define WORKERTHREAD_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <list>
#include <mutex>
#include <thread>

#include <uv.h>

/**
 * Class to offload some work at separted thread with ability to communicate
 * with main thread.
 */
class WorkerThread
{

public:

    explicit WorkerThread();

    // Start the thread
    void start();

    // Execute function in the spawned thread and result in main thread
    void exec(std::function<void ()>&& function,
              std::function<void ()>&& result = std::function<void ()>());

    // Stop the thread and execute result in main thread when all is done
    void stop(std::function<void ()>&& result);

private:

    // Process functions list in spawned thread
    void process();

    // Process results list in main thread
    static void processResults(uv_async_t* handle);

private:

    // Thread id
    std::thread thread;

    // Should we stop?
    std::atomic<bool> isStop;

    // Functions list
    std::mutex functionsMutex;
    std::list<std::function<void ()>> functions;

    // Results list
    std::mutex resultsMutex;
    std::list<std::function<void ()>> results;

    // Worker thread conditional variable
    std::condition_variable signal;

    // Async to communicate with the main thread
    uv_async_t async;
};

#endif
