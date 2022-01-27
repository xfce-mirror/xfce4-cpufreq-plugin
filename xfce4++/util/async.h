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

#ifndef _XFCE4PP_UTIL_ASYNC_H_
#define _XFCE4PP_UTIL_ASYNC_H_

#include <functional>
#include "memory.h"

namespace xfce4 {

struct LaunchConfig {
    /*
     * Start the task even if the previously started task didn't finish yet?
     *
     * If the OS thread is busy and start_if_busy==true, then the argument 'task'
     * passed to 'start(config, task)' is discarded and 'start(config, task)' returns
     * immediately without calling/evaluating 'task'.
     */
    bool start_if_busy = true;
};

struct TaskQueue {
    typedef std::function<void()> Task;

    virtual ~TaskQueue();

    /*
     * Launches the specified task in this queue.
     *
     * In case the current thread is the GUI thread, then the task
     * might be able to run without interfering with the GUI thread.
     */
    virtual void start(LaunchConfig config, const Task &task) = 0;
};

/*
 * A queue that runs tasks in a single separate OS thread.
 * The single thread is different from the main GUI thread.
 */
extern const Ptr<TaskQueue> singleThreadQueue;

} /* namespace xfce4 */

#endif /* _XFCE4PP_UTIL_ASYNC_H_ */
