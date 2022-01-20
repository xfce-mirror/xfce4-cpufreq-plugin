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

#ifndef _XFCE4PP_UTIL_STRINGUTILS_H_
#define _XFCE4PP_UTIL_STRINGUTILS_H_

#include <glib.h>
#include <string>
#include "optional.h"

namespace xfce4 {

/*
 * Number parsing functions. If parsing fails, an Optional without a value is returned.
 *
 * The string can contain just the number, with an optional prefix and suffix consisting from whitespace characters.
 */
Optional<gdouble> parse_double(const std::string &s);
Optional<gfloat>  parse_float (const std::string &s);
Optional<glong>   parse_long  (const std::string &s, unsigned base = 0);
Optional<gulong>  parse_ulong (const std::string &s, unsigned base = 0);

/*
 * Number parsing functions which move an indirect string pointer
 * to the character which follows the parsed number.
 * If parsing fails then the indirect string pointer is not modified.
 *
 * These functions return zero in case of an error.
 */
glong  parse_long (gchar **s, unsigned base = 0, bool *error = nullptr);
gulong parse_ulong(gchar **s, unsigned base = 0, bool *error = nullptr);

bool        ends_with  (const std::string &s, const char        *suffix);
bool        ends_with  (const std::string &s, const std::string &suffix);
std::string sprintf    (const char *fmt, ...) G_GNUC_PRINTF(1, 2);
bool        starts_with(const std::string &s, const char        *prefix);
bool        starts_with(const std::string &s, const std::string &prefix);
std::string trim       (const std::string &s);
std::string trim_left  (const std::string &s);
std::string trim_right (const std::string &s);

} /* namespace xfce4 */

#endif /* _XFCE4PP_UTIL_STRINGUTILS_H_ */
