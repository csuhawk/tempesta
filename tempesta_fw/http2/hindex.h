/**
 *		Tempesta FW
 *
 * HPACK static and dynamic tables for header fields.
 *
 * Copyright (C) 2017 Tempesta Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef HPACK_INDEX_H
#define HPACK_INDEX_H

#include "common.h"
#include "../str.h"
#include "errors.h"
#include "buffers.h"
#include "hpack.h"

ufast
hpack_add (HPack  * __restrict hp,
	   TfwStr * __restrict name,
	   TfwStr * __restrict value);

ufast
hpack_add_index (HPack	* __restrict hp,
		 ufast		     index,
		 TfwStr * __restrict value);


#endif
