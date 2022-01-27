/*
 *  This file is part of Xfce (https://gitlab.xfce.org).
 *
 *  Copyright (c) 2022 Jan Ziak <0xe2.0x9a.0x9b@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "async.h"

#include <algorithm>
#include <condition_variable>
#include <list>
#include <mutex>
#include <thread>
#include <unistd.h>

namespace xfce4 {

TaskQueue::~TaskQueue() {}

struct SingleThreadQueue final : TaskQueue {
    struct Data {
        std::condition_variable cond_var;
        std::mutex mutex;
        std::list<Task> queue;
        bool stop = false;
    };
    Ptr<Data> data = make<Data>();
    std::thread *thread = nullptr;

    ~SingleThreadQueue();

    void start(const LaunchConfig config, const Task &task) override;
};

const Ptr<TaskQueue> singleThreadQueue = make<SingleThreadQueue>();

SingleThreadQueue::~SingleThreadQueue() {
    data->mutex.lock();
    if(thread) {
        data->stop = true;
        data->mutex.unlock();
        data->cond_var.notify_one();
        thread->join();
        delete thread;
    }
    else {
        data->mutex.unlock();
    }
}

void SingleThreadQueue::start(const LaunchConfig config, const Task &task) {
    // Previously queued tasks
    while(true) {
        data->mutex.lock();
        if(!data->queue.empty()) {
            data->mutex.unlock();
            if(config.start_if_busy) {
                // Wait for the thread to empty the queue
                usleep(100*1000);
            }
            else {
                // Discard the task
                return;
            }
        }
        else {
            data->mutex.unlock();
            break;
        }
    }

    // Add task to the queue
    data->mutex.lock();
    data->queue.push_back(task);
    data->mutex.unlock();
    data->cond_var.notify_one();

    data->mutex.lock();
    if(!thread) {
        thread = new std::thread([data = this->data]() {
            while(true) {
                std::unique_lock<std::mutex> lock(data->mutex);
                data->cond_var.wait(lock, [data]() {
                    return !data->queue.empty() || data->stop;
                });
                if(data->stop) {
                    lock.unlock();
                    break;
                }
                else {
                    auto f = std::move(data->queue.front());
                    data->queue.pop_front();
                    lock.unlock();
                    f();
                }
            }
        });
    }
    data->mutex.unlock();
}

} /* namespace xfce4 */
