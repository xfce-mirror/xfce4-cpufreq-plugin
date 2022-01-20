/*
 *  This file is part of Xfce (https://gitlab.xfce.org).
 *
 *  Copyright (c) 2021 Jan Ziak <0xe2.0x9a.0x9b@xfce.org>
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

#ifndef _XFCE4PP_UTIL_MEMORY_H_
#define _XFCE4PP_UTIL_MEMORY_H_

#include <cstdlib>
#include <glib.h>
#include <memory>
#include <utility>

namespace xfce4 {

template<typename T> struct Ptr0;

/* A pointer that cannot be null */
template<typename T>
struct Ptr final {
    std::shared_ptr<T> ptr;

    template<typename... Args>
    static Ptr<T> make(Args&&... args) {
        return Ptr<T>(std::make_shared<T>(std::forward<Args>(args)...));
    }

    template<typename U> Ptr(const Ptr<U> &p) : ptr(p.ptr) {}

    T* operator->() const { return ptr.get(); }

    template<typename U> bool operator<(const Ptr<U> &p) const { return ptr < p.ptr; }

private:
    friend Ptr0<T>;
    Ptr(const std::shared_ptr<T> &p) : ptr(p) {}
    Ptr(std::shared_ptr<T> &&p) : ptr(std::move(p)) {}
};

/* A pointer that can be null */
template<typename T>
struct Ptr0 final : std::shared_ptr<T> {
    Ptr0() : std::shared_ptr<T>(nullptr) {}
    Ptr0(std::nullptr_t) : std::shared_ptr<T>(nullptr) {}
    template<typename U> Ptr0(const std::shared_ptr<U> &p) : std::shared_ptr<T>(p) {}
    template<typename U> Ptr0(std::shared_ptr<U> &&p) : std::shared_ptr<T>(std::move(p)) {}
    template<typename U> Ptr0(const Ptr<U> &p) : std::shared_ptr<T>(p.ptr) {}

    Ptr<T> toPtr() const {
        if(*this) {
            return Ptr<T>(*this);
        }
        else {
            g_error("null pointer");
            std::abort();
        }
    }
};

/* Allocates a new instance of T on the heap, passing args to T's constructor */
template<typename T, typename... Args>
inline Ptr<T> make(Args&&... args) {
    return Ptr<T>::template make<Args...>(std::forward<Args>(args)...);
}

/* Calls malloc_trim(), if malloc_trim() is available */
bool trim_memory(size_t pad = 0);

} /* namespace xfce4 */

#endif /* _XFCE4PP_UTIL_MEMORY_H_ */
