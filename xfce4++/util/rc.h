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

#ifndef _XFCE4PP_UTIL_RC_H_
#define _XFCE4PP_UTIL_RC_H_

#ifndef _XFCE4PP_UTIL_FIXES_H_
#error "Please include xfce4++/util/fixes.h before any other include directives"
#endif

#include <glib.h>
#include <libxfce4util/libxfce4util.h>
#include <string>
#include "memory.h"

namespace xfce4 {

struct Rc final {
    typedef std::string string;

    XfceRc *rc;

    static Ptr0<Rc> simple_open(const string &filename, bool readonly);

    Rc(XfceRc *rc);
    ~Rc();

    void close();

    void delete_entry(const char   *key, bool global);
    void delete_entry(const string &key, bool global);

    bool         read_bool_entry (const char   *key, bool          fallback       ) const G_GNUC_WARN_UNUSED_RESULT;
    bool         read_bool_entry (const string &key, bool          fallback       ) const G_GNUC_WARN_UNUSED_RESULT;
    Ptr0<string> read_entry      (const char   *key, const char   *fallback_orNull) const G_GNUC_WARN_UNUSED_RESULT;
    Ptr0<string> read_entry      (const string &key, const char   *fallback_orNull) const G_GNUC_WARN_UNUSED_RESULT;
    string       read_entry      (const char   *key, const string &fallback       ) const G_GNUC_WARN_UNUSED_RESULT;
    string       read_entry      (const string &key, const string &fallback       ) const G_GNUC_WARN_UNUSED_RESULT;
    float        read_float_entry(const char   *key, float         fallback       ) const G_GNUC_WARN_UNUSED_RESULT;
    float        read_float_entry(const string &key, float         fallback       ) const G_GNUC_WARN_UNUSED_RESULT;
    gint         read_int_entry  (const char   *key, gint          fallback       ) const G_GNUC_WARN_UNUSED_RESULT;
    gint         read_int_entry  (const string &key, gint          fallback       ) const G_GNUC_WARN_UNUSED_RESULT;

    bool has_group(const char   *group) const;
    bool has_group(const string &group) const;
    void set_group(const char   *group);
    void set_group(const string &group);

    void write_bool_entry (const char   *key, bool          value);
    void write_bool_entry (const string &key, bool          value);
    void write_entry      (const char   *key, const char   *value);
    void write_entry      (const char   *key, const string &value);
    void write_entry      (const string &key, const char   *value);
    void write_entry      (const string &key, const string &value);
    void write_float_entry(const char   *key, float         value);
    void write_float_entry(const string &key, float         value);
    void write_int_entry  (const char   *key, gint          value);
    void write_int_entry  (const string &key, gint          value);

    /* Write an entry if it differs from the default value.
     * Delete an entry if it equals to the default value. */
    void write_default_bool_entry (const char   *key, bool          value, bool          default_value);
    void write_default_bool_entry (const string &key, bool          value, bool          default_value);
    void write_default_entry      (const char   *key, const char   *value, const char   *default_value);
    void write_default_entry      (const char   *key, const string &value, const string &default_value);
    void write_default_entry      (const string &key, const char   *value, const char   *default_value);
    void write_default_entry      (const string &key, const string &value, const string &default_value);
    void write_default_float_entry(const char   *key, float         value, float         default_value, float epsilon);
    void write_default_float_entry(const string &key, float         value, float         default_value, float epsilon);
    void write_default_int_entry  (const char   *key, gint          value, gint          default_value);
    void write_default_int_entry  (const string &key, gint          value, gint          default_value);
};

} /* namespace xfce4 */

#endif /* _XFCE4PP_UTIL_RC_H_ */
