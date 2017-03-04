#include <stdio.h>
#include "common.h"
#include "../pool.h"
#include "../str.h"
#include "errors.h"
#include "buffers.h"

#define Debug_Buffers 0

/* -------------------------------------------------- */
/* Input buffer (used by parser to handle fragments): */
/* -------------------------------------------------- */

/* Initialize input buffer from the TfwStr: */

void
buffer_from_tfwstr (HTTP2Input * __restrict p,
		    const TfwStr * __restrict str)
{
	p->offset = 0;
	p->n = str->len;
	p->current = 0;
	p->tail = 0; /* p->tail is initialized here only to avoid */
		     /* partial writing into cache line, really */
		     /* it does not used by buffer_get() call. */
	p->str = str;
}

/* Get pointer to and length of the current fragment ("m"): */

const uchar *
buffer_get (HTTP2Input * __restrict p,
	    uwide      * __restrict m)
{
	const uwide offset = p->offset;
	const TfwStr * __restrict fp = p->str;
	if (TFW_STR_PLAIN(fp)) {
		#if Debug_Buffers
			puts("Plain string...");
		#endif
		* m = p->n;
	}
	else {
		uwide tail;
		fp = __TFW_STR_CH(fp, p->current);
		tail = fp->len - offset;
		p->tail = tail;
		* m = tail;
	}
	#if Debug_Buffers
		printf("Open at: %u, current: %u bytes...\n", offset, * m);
	#endif
	return (const uchar *) fp->ptr + offset;
}

/* Get the next pointer and length of the next fragment: */

const uchar *
buffer_next (HTTP2Input * __restrict p,
	     uwide	* __restrict m)
{
	const uwide current = p->current + 1;
	TfwStr * __restrict fp = __TFW_STR_CH(p->str, current);
	const uwide length = fp->len;
	p->offset = 0;
	p->n -= p->tail;
	p->current = current;
	p->tail = length;
	* m = length;
	#if Debug_Buffers
		printf("Next fragment: %u bytes...\n", length);
	#endif
	return (const uchar *) fp->ptr;
}

/* Close current parser iteration. There "m" is the length */
/* of unparsed tail of the current fragment: */

void
buffer_close (HTTP2Input * __restrict p,
	      uwide		      m)
{
	const uwide tail = p->tail;
	if (m) {
		const uwide bias = tail - m;
		if (bias) {
			p->offset += bias;
			p->n -= bias;
			#if Debug_Buffers
				printf("Shift forward to %u bytes...\n", bias);
			#endif
		}
	}
	else {
		p->offset = 0;
		p->n -= tail;
		p->current++;
		#if Debug_Buffers
			printf("Consumed %u bytes...\n", tail);
		#endif
	}
}

/* --------------------------------------------- */
/* Output buffer (used to write decoded stings): */
/* --------------------------------------------- */

/* Allocate new output buffer: */

void
buffer_new (HTTP2Output * __restrict p,
	    TfwPool	* __restrict pool)
{
     /* Many fields initialized here only to avoid */
     /* partial writing into cache line: */
	p->last = NULL;
	p->first = NULL;
	p->current = NULL;
	p->offset = 0;
	p->tail = 0;
	p->count = 0;
	p->total = 0;
	p->str = NULL;
	p->pool = pool;
}

/* Add new block to the output buffer. Returns the NULL */
/* and zero length ("n") if unable to allocate memory: */

#ifndef offsetof
   #define offsetof(x, y) ((uwide) &((x *) 0)->y))
#endif

#define Page_Size 4096
#define Unused_Space (Page_Size - offsetof(HTTP2Block, data))

uchar *
buffer_expand (HTTP2Output * __restrict p,
	       ufast	   * __restrict n)
{
	HTTP2Block * __restrict block = tfw_pool_alloc(p->pool, Page_Size);
	if (block) {
		HTTP2Block * __restrict last = p->last;
		block->next = NULL;
		block->n = Page_Size;
		* n = Unused_Space;
		p->last = block;
		if (last) {
			const ufast tail = p->tail;
			last->next = block;
			if (tail) {
				p->tail = Unused_Space;
				p->count++;
				p->total += tail;
				#if Debug_Buffers
					printf("New fragment added to string: %u bytes...\n", tail);
				#endif
			}
			else {
				p->current = block;
				p->offset = 0;
				p->tail = Unused_Space;
				#if Debug_Buffers
					puts("New string started at offset 0...");
				#endif
			}
		}
		else {
			p->first = block;
			p->current = block;
			p->offset = 0; /* p->offset is initialized here */
				       /* only to avoid partial writing */
				       /* into cache line. */
			p->tail = Unused_Space;
			#if Debug_Buffers
				puts("Initial block allocated...");
			#endif
		}
		#if Debug_Buffers
			printf("New offset = %u\n", p->offset);
			printf("New block allocated: %u unused bytes...\n", p->tail);
		#endif
		return block->data;
	}
	else {
		#if Debug_Buffers
			puts("Unable to allocate memory block...");
		#endif
		* n = 0;
		return NULL;
	}
}

/* Open output buffer before decoding the new string. */
/* There "n" is the length of available space in the buffer: */

uchar *
buffer_open (HTTP2Output * __restrict p,
	     ufast	 * __restrict n)
{
	const ufast tail = p->tail;
	* n = tail;
	if (tail) {
		HTTP2Block * __restrict last = p->last;
		p->current = last;
		#if Debug_Buffers
			printf("Open output buffer with %u bytes (offset = %u)...\n", tail, p->offset);
		#endif
		return last->data + p->offset;
	}
	else {
		#if Debug_Buffers
			puts("Return empty output buffer...");
		#endif
		return NULL;
	}
}

/* Emit the new string. Returns error code if unable */
/* to allocate memory: */

ufast
buffer_emit (HTTP2Output * __restrict p,
	     ufast		      n)
{
	const ufast offset = p->offset;
	const ufast tail = p->tail - (ufast) n;
	ufast count = p->count;
	const uwide total = p->total + tail;
	#if Debug_Buffers
		printf("In emit, total length: %u, last delta: %u...\n", total, tail);
		printf("Offset = %u\n", offset);
	#endif
	if (total) {
		TfwPool * __restrict pool = p->pool;
		HTTP2Block * __restrict current = p->current;
		TfwStr * __restrict str;
		p->offset = count ? tail : tail + offset;
		p->tail = n;
		p->count = 0;
		p->total = 0;
		str = tfw_pool_alloc(pool, sizeof(TfwStr));
		p->str = str;
		if (count == 0) {
			str->ptr = current->data + offset;
			str->len = total;
			str->skb = NULL;
			str->eolen = 0;
			str->flags = 0;
		}
		else {
			TfwStr * __restrict fp =
				tfw_pool_alloc(pool, ++count * sizeof(TfwStr));
			str->ptr = fp;
			str->len = total;
			str->skb = NULL;
			str->eolen = 0;
			str->flags = count << TFW_STR_CN_SHIFT;
			fp->ptr = current->data + offset;
			fp->len = current->n - offsetof(HTTP2Block, data) - offset;
			fp->skb = NULL;
			fp->eolen = 0;
			fp->flags = 0;
			#if Debug_Buffers
				printf("Fragment: %u bytes...\n", fp->len);
			#endif
			fp++;
			count -= 2;
			while (count) {
				current = current->next;
				fp->ptr = current->data;
				fp->len = current->n - offsetof(HTTP2Block, data);
				fp->skb = NULL;
				fp->eolen = 0;
				fp->flags = 0;
				#if Debug_Buffers
					printf("Fragment: %u bytes...\n", fp->len);
				#endif
				fp++;
				count--;
			}
			current = current->next;
			fp->ptr = current->data;
			fp->len = tail;
			fp->skb = NULL;
			fp->eolen = 0;
			fp->flags = 0;
			#if Debug_Buffers
				printf("Fragment: %u bytes...\n", fp->len);
			#endif
		}
	}
	return 0;
}
