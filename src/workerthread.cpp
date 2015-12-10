#include "workerthread.hpp"

WorkerThread::WorkerThread():
    isStop(false)
{
}

void WorkerThread::start()
{
    isStop = false;

    uv_async_init(uv_default_loop(), &async, (uv_async_cb)processResults);
    async.data = this;

    thread = std::thread(&WorkerThread::process, this);
}

void WorkerThread::exec(std::function<void ()>&& function,
                        std::function<void ()>&& result)
{
    if (isStop)
    {
        return;
    }

    {
        std::lock_guard<std::mutex> functionsLock(functionsMutex);
        functions.emplace_back([this, function = std::move(function), result = std::move(result)]()
        {
            function();

            if (result)
            {
                std::lock_guard<std::mutex> resultsLock(resultsMutex);
                results.emplace_back(std::move(result));
            }

            uv_async_send(&async);
        });
    }

    signal.notify_one();
}

void WorkerThread::stop(std::function<void ()>&& result)
{
    exec([]()
    {
    },
    [this, result = std::move(result)]()
    {
        uv_close((uv_handle_t*)&async, NULL);

        if (result)
        {
            result();
        }
    });

    isStop = true;
    signal.notify_one();
    thread.join();
}

void WorkerThread::process()
{
    std::unique_lock<std::mutex> lock(functionsMutex);
    while (!isStop || !functions.empty())
    {
        if (!functions.empty())
        {
            std::function<void()> function(std::move(functions.front()));
            functions.pop_front();

            lock.unlock();
            function();
            lock.lock();
        }
        else
        {
            signal.wait(lock);
        }
    }
}

void WorkerThread::processResults(uv_async_t* handle)
{
    WorkerThread* workerThread = static_cast<WorkerThread*>(handle->data);

    std::unique_lock<std::mutex> lock(workerThread->resultsMutex);
    while (!workerThread->results.empty())
    {
        std::function<void()> result(std::move(workerThread->results.front()));
        workerThread->results.pop_front();

        lock.unlock();
        result();
        lock.lock();
    }
}
