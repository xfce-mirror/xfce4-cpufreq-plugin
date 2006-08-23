/*  xfce4-cpu-freq-plugin - panel plugin for cpu informations
 *
 *  Copyright (c) 2006 Thomas Schreck <shrek@xfce.org>
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef XFCE4_CPUFREQ_LINUX_H
#define XFCE4_CPUFREQ_LINUX_H

G_BEGIN_DECLS

gboolean
cpufreq_cpu_set_freq (guint cpu_number, guint *freq);

gboolean
cpufreq_cpu_set_governor (guint cpu_number, gchar *governor);

gboolean
cpufreq_update_cpus (gpointer data);

gboolean
cpufreq_linux_init (void);

G_END_DECLS

#endif /* XFCE4_CPUFREQ_LINUX_H */
