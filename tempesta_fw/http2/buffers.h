/**
 *		Tempesta FW
 *
 * HTTP/2 bufferization helpers for fragment-based parser.
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

#ifndef HTTP2_BUFFERS_H
#define HTTP2_BUFFERS_H

#include "common.h"
#include "../pool.h"
#include "../str.h"
#include "errors.h"

/* -------------------------------------------------- */
/* Input buffer (used by parser to handle fragments): */
/* -------------------------------------------------- */

typedef struct {
	uwide offset;  /* Offset in the current fragment.    */
	uwide n;       /* Total number of unparsed octets    */
		       /* in the string.		     */
	uwide current; /* Index of the current TfwStr chunk. */
	uwide tail;    /* Length of the unparsed tail in     */
		       /* the current fragment. 	     */
	const TfwStr * str; /* Source string.		     */
} HTTP2Input;

/* Initialize input buffer from the TfwStr: */

void
buffer_from_tfwstr (HTTP2Input * __restrict p,
		    const TfwStr * __restrict str);

/* Get the current pointer, total length of unparsed part */
/* of the string ("n") and length of the current fragment ("m"): */

const uchar *
buffer_get (HTTP2Input * __restrict p,
	    uwide      * __restrict n,
	    uwide      * __restrict m);

/* Get the next pointer and length of the next fragment: */

const uchar *
buffer_next (HTTP2Input * __restrict p,
	     uwide	* __restrict m);

/* Close current parser iteration. There "m" is the length */
/* of unparsed tail of the current fragment: */

void
buffer_close (HTTP2Input * __restrict p,
	      uwide		      m);

/* --------------------------------------------- */
/* Output buffer (used to write decoded stings): */
/* --------------------------------------------- */

typedef struct HTTP2Block {
	struct HTTP2Block * next;     /* Next allocated block. */
	uint16		    n;	      /* Block length. */
	uchar		    data [1]; /* Decoded data. */
} HTTP2Block;

typedef struct {
	HTTP2Block * first;   /* First allocated block. */
	HTTP2Block * last;    /* Last allocated block. */
	HTTP2Block * current; /* Block where current string is started. */
	uint16	     offset;  /* Offset of the current string in the first */
			      /* block where it started. */
	uint16	     tail;    /* Length of the unused tail of the current block. */
	uint32	     count;   /* Total number of chunks in the current string. */
	uwide	     total;   /* Total length of the current string. */
	TfwStr	   * str;     /* Last decored string. */
	TfwPool    * pool;    /* Memory allocation pool. */
} HTTP2Output;

/* Allocate new output buffer: */

void
buffer_new (HTTP2Output * __restrict p,
	    TfwPool	* __restrict pool);

/* Add new block to the output buffer. Returns the NULL */
/* and zero length ("n") if unable to allocate memory: */

uchar *
buffer_expand (HTTP2Output * __restrict p,
	       ufast	   * __restrict n);

/* Open output buffer before decoding the new string. */
/* There "n" is the length of available space in the buffer: */

uchar *
buffer_open (HTTP2Output * __restrict p,
	     ufast	 * __restrict n);

/* Emit the new string. Returns error code if unable */
/* to allocate memory: */

ufast
buffer_emit (HTTP2Output * __restrict p,
	     ufast		      n);

#endif
