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

#ifndef _XFCE4PP_UTIL_OPTIONAL_H_
#define _XFCE4PP_UTIL_OPTIONAL_H_

#include <glib.h>
#include <new>
#include <utility>

namespace xfce4 {

/*
 * A value that might or might not exist.
 *
 * Notes:
 *  - std::optional is a C++17 feature, not a C++11 feature
 *  - std::optional defines operator bool(), which is error-prone in case T is an integral type
 */
template<typename T>
struct Optional {
    Optional()               { exists = false; }
    Optional(const T &value) { exists = true; new (valuePtr()) T(value); }
    Optional(T &&value)      { exists = true; new (valuePtr()) T(std::move(value)); }

    Optional(const Optional &o) { exists = false; *this = o; }
    Optional(Optional &&o)      { exists = false; *this = std::move(o); }

    ~Optional() {
        if(exists)
            valuePtr()->~T();
    }

    Optional& operator=(const Optional &o) {
        if(G_LIKELY(this != &o)) {
            if(exists) {
                if(o.exists) {
                    *valuePtr() = *o.constValuePtr();
                }
                else {
                    exists = false;
                    valuePtr()->~T();
                }
            }
            else {
                if(o.exists) {
                    exists = true;
                    new (valuePtr()) T(*o.constValuePtr());
                }
            }
        }
        return *this;
    }

    Optional& operator=(Optional &&o) {
        if(G_LIKELY(this != &o)) {
            if(exists) {
                if(o.exists) {
                    *valuePtr() = std::move(*o.valuePtr());
                    o.valuePtr()->~T();
                    o.exists = false;
                }
                else {
                    exists = false;
                    valuePtr()->~T();
                }
            }
            else {
                if(o.exists) {
                    exists = true;
                    new (valuePtr()) T(std::move(*o.valuePtr()));
                    o.valuePtr()->~T();
                    o.exists = false;
                }
            }
        }
        return *this;
    }

    operator bool() const = delete;

    bool     has_value() const { return exists; }
    const T& value() const     { g_assert(exists); return *constValuePtr(); }
    T&       value()           { g_assert(exists); return *valuePtr(); }

private:
    bool exists;
    char bytes[sizeof(T)] __attribute__((aligned(__alignof__(T))));

    const T* constValuePtr() const { return reinterpret_cast<const T*>(&bytes[0]); }
    T*       valuePtr()            { return reinterpret_cast<T*>(&bytes[0]); }
};

} /* namespace xfce4 */

#endif /* _XFCE4PP_UTIL_OPTIONAL_H_ */
