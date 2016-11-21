/* indicator_a11y.h
 *
 * Copyright (C) 2012 Paddubsky A.V. <pan.pav.7c5@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _INDICATOR_A11Y_H_INCLUDED_
#define _INDICATOR_A11Y_H_INCLUDED_

#include <gtk/gtk.h>

/* Functions */

void init_a11y_indicator               (void);
void a11y_close                        (void);

void a11y_toggle_osk                   (void);
void a11y_toggle_font                  (void);
void a11y_toggle_dpi                   (void);
void a11y_toggle_contrast              (void);

#endif // _INDICATOR_A11Y_H_INCLUDED_
