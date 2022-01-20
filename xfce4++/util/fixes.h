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

#ifndef _XFCE4PP_UTIL_FIXES_H_
#define _XFCE4PP_UTIL_FIXES_H_

#if defined G_BEGIN_DECLS || defined LIBXFCE4UTIL_MAJOR_VERSION
#error "Please include xfce4++/util/fixes.h before any other include directives"
#endif

/* Before extern "C" */
#include <glib.h>

/*
 * Adjust for missing G_BEGIN_DECLS/G_END_DECLS directives in C files
 */
#define LIBXFCE4UTIL_INSIDE_LIBXFCE4UTIL_H
extern "C" {
    #include <libxfce4util/libxfce4util-config.h>
    #if LIBXFCE4UTIL_CHECK_VERSION(4, 17, 0)
        #include <libxfce4util/xfce-gio-extensions.h>
    #endif
    #include <libxfce4util/xfce-i18n.h>
}
#undef LIBXFCE4UTIL_INSIDE_LIBXFCE4UTIL_H

#endif /* _XFCE4PP_UTIL_FIXES_H_ */
