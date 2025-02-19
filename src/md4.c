/*
 * This is an OpenSSL-compatible implementation of the RSA Data Security, Inc.
 * MD4 Message-Digest Algorithm (RFC 1320).
 *
 * Homepage:
 * https://openwall.info/wiki/people/solar/software/public-domain-source-code/md4
 *
 * Author:
 * Alexander Peslyak, better known as Solar Designer <solar at openwall.com>
 *
 * This software was written by Alexander Peslyak in 2001.  No copyright is
 * claimed, and the software is hereby placed in the public domain.
 * In case this attempt to disclaim copyright and place the software in the
 * public domain is deemed null and void, then the software is
 * Copyright (c) 2001 Alexander Peslyak and it is hereby released to the
 * general public under the following terms:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted.
 *
 * There's ABSOLUTELY NO WARRANTY, express or implied.
 *
 * (This is a heavily cut-down "BSD license".)
 *
 * This differs from Colin Plumb's older public domain implementation in that
 * no exactly 32-bit integer data type is required (any 32-bit or wider
 * unsigned integer data type will do), there's no compile-time endianness
 * configuration, and the function prototypes match OpenSSL's.  No code from
 * Colin Plumb's implementation has been reused; this comment merely compares
 * the properties of the two independent implementations.
 *
 * The primary goals of this implementation are portability and ease of use.
 * It is meant to be fast, but not as fast as possible.  Some known
 * optimizations are not included to reduce source code size and avoid
 * compile-time configuration.
 */

/*
 * The code below has been modified in many ways for use specifically in John
 * the Ripper jumbo.  If you want to reuse it outside of that project, please
 * obtain the original from the homepage URL above.
 */

#include "md4.h"

#if !HAVE_LIBCRYPTO
#include <string.h>

/*
 * The basic MD4 functions.
 *
 * F and G are optimized compared to their RFC 1320 definitions, with the
 * optimization for F borrowed from Colin Plumb's MD5 implementation.
 */
#define F(x, y, z)			((z) ^ ((x) & ((y) ^ (z))))
#if 0 /* Wei Dai's trick, but we let the compiler cache/reuse or not */
#define G(x, y, z)			(y ^ ((x ^ y) & (y ^ z)))
#elif 0
#define G(x, y, z)			((x & y) ^ (x & z) ^ (y & z))
#else
#define G(x, y, z)			(((x) & ((y) | (z))) | ((y) & (z)))
#endif
#define H(x, y, z)			(((x) ^ (y)) ^ (z))
#define H2(x, y, z)			((x) ^ ((y) ^ (z)))

/*
 * The MD4 transformation for all three rounds.
 */
#define STEP(f, a, b, c, d, x, s) \
	(a) += f((b), (c), (d)) + (x); \
	(a) = (((a) << (s)) | (((a) & 0xffffffff) >> (32 - (s))));

/*
 * SET reads 4 input bytes in little-endian byte order and stores them in a
 * properly aligned word in host byte order.
 *
 * The check for little-endian architectures that tolerate unaligned memory
 * accesses is just an optimization.  Nothing will break if it fails to detect
 * a suitable architecture.
 *
 * Unfortunately, this optimization may be a C strict aliasing rules violation
 * if the caller's data buffer has effective type that cannot be aliased by
 * MD4_u32plus.  In practice, this problem may occur if these MD4 routines are
 * inlined into a calling function, or with future and dangerously advanced
 * link-time optimizations.  For the time being, keeping these MD4 routines in
 * their own translation unit avoids the problem.
 */
#if ARCH_LITTLE_ENDIAN && ARCH_ALLOWS_UNALIGNED
#define SET(n) \
	(*(MD4_u32plus *)&ptr[(n) * 4])
#define GET(n) \
	SET(n)
#else
#define SET(n) \
	(ctx->block[(n)] = \
	(MD4_u32plus)ptr[(n) * 4] | \
	((MD4_u32plus)ptr[(n) * 4 + 1] << 8) | \
	((MD4_u32plus)ptr[(n) * 4 + 2] << 16) | \
	((MD4_u32plus)ptr[(n) * 4 + 3] << 24))
#define GET(n) \
	(ctx->block[(n)])
#endif

/*
 * This processes one or more 64-byte data blocks, but does NOT update the bit
 * counters.  There are no alignment requirements.
 */
static const void *body(MD4_CTX *ctx, const void *data, unsigned long size)
{
	const unsigned char *ptr;
	MD4_u32plus a, b, c, d;
	MD4_u32plus saved_a, saved_b, saved_c, saved_d;
	const MD4_u32plus ac1 = 0x5a827999, ac2 = 0x6ed9eba1;

	ptr = (const unsigned char *)data;

	a = ctx->A;
	b = ctx->B;
	c = ctx->C;
	d = ctx->D;

	do {
		saved_a = a;
		saved_b = b;
		saved_c = c;
		saved_d = d;

/* Round 1 */
		STEP(F, a, b, c, d, SET(0), 3)
		STEP(F, d, a, b, c, SET(1), 7)
		STEP(F, c, d, a, b, SET(2), 11)
		STEP(F, b, c, d, a, SET(3), 19)
		STEP(F, a, b, c, d, SET(4), 3)
		STEP(F, d, a, b, c, SET(5), 7)
		STEP(F, c, d, a, b, SET(6), 11)
		STEP(F, b, c, d, a, SET(7), 19)
		STEP(F, a, b, c, d, SET(8), 3)
		STEP(F, d, a, b, c, SET(9), 7)
		STEP(F, c, d, a, b, SET(10), 11)
		STEP(F, b, c, d, a, SET(11), 19)
		STEP(F, a, b, c, d, SET(12), 3)
		STEP(F, d, a, b, c, SET(13), 7)
		STEP(F, c, d, a, b, SET(14), 11)
		STEP(F, b, c, d, a, SET(15), 19)

/* Round 2 */
		STEP(G, a, b, c, d, GET(0) + ac1, 3)
		STEP(G, d, a, b, c, GET(4) + ac1, 5)
		STEP(G, c, d, a, b, GET(8) + ac1, 9)
		STEP(G, b, c, d, a, GET(12) + ac1, 13)
		STEP(G, a, b, c, d, GET(1) + ac1, 3)
		STEP(G, d, a, b, c, GET(5) + ac1, 5)
		STEP(G, c, d, a, b, GET(9) + ac1, 9)
		STEP(G, b, c, d, a, GET(13) + ac1, 13)
		STEP(G, a, b, c, d, GET(2) + ac1, 3)
		STEP(G, d, a, b, c, GET(6) + ac1, 5)
		STEP(G, c, d, a, b, GET(10) + ac1, 9)
		STEP(G, b, c, d, a, GET(14) + ac1, 13)
		STEP(G, a, b, c, d, GET(3) + ac1, 3)
		STEP(G, d, a, b, c, GET(7) + ac1, 5)
		STEP(G, c, d, a, b, GET(11) + ac1, 9)
		STEP(G, b, c, d, a, GET(15) + ac1, 13)

/* Round 3 */
		STEP(H, a, b, c, d, GET(0) + ac2, 3)
		STEP(H2, d, a, b, c, GET(8) + ac2, 9)
		STEP(H, c, d, a, b, GET(4) + ac2, 11)
		STEP(H2, b, c, d, a, GET(12) + ac2, 15)
		STEP(H, a, b, c, d, GET(2) + ac2, 3)
		STEP(H2, d, a, b, c, GET(10) + ac2, 9)
		STEP(H, c, d, a, b, GET(6) + ac2, 11)
		STEP(H2, b, c, d, a, GET(14) + ac2, 15)
		STEP(H, a, b, c, d, GET(1) + ac2, 3)
		STEP(H2, d, a, b, c, GET(9) + ac2, 9)
		STEP(H, c, d, a, b, GET(5) + ac2, 11)
		STEP(H2, b, c, d, a, GET(13) + ac2, 15)
		STEP(H, a, b, c, d, GET(3) + ac2, 3)
		STEP(H2, d, a, b, c, GET(11) + ac2, 9)
		STEP(H, c, d, a, b, GET(7) + ac2, 11)
		STEP(H2, b, c, d, a, GET(15) + ac2, 15)

		a += saved_a;
		b += saved_b;
		c += saved_c;
		d += saved_d;

		ptr += 64;
	} while (size -= 64);

	ctx->A = a;
	ctx->B = b;
	ctx->C = c;
	ctx->D = d;

	return ptr;
}

void MD4_Init(MD4_CTX *ctx)
{
	ctx->A = 0x67452301;
	ctx->B = 0xefcdab89;
	ctx->C = 0x98badcfe;
	ctx->D = 0x10325476;

	ctx->lo = 0;
	ctx->hi = 0;
}

void MD4_Update(MD4_CTX *ctx, const void *data, unsigned long size)
{
	MD4_u32plus saved_lo;
	unsigned long used, available;

	saved_lo = ctx->lo;
	if ((ctx->lo = (saved_lo + size) & 0x1fffffff) < saved_lo)
		ctx->hi++;
	ctx->hi += size >> 29;

	used = saved_lo & 0x3f;

	if (used) {
		available = 64 - used;

		if (size < available) {
			memcpy(&ctx->buffer[used], data, size);
			return;
		}

		memcpy(&ctx->buffer[used], data, available);
		data = (const unsigned char *)data + available;
		size -= available;
		body(ctx, ctx->buffer, 64);
	}

	if (size >= 64) {
		data = body(ctx, data, size & ~(unsigned long)0x3f);
		size &= 0x3f;
	}

	memcpy(ctx->buffer, data, size);
}

#define OUT(dst, src) \
	(dst)[0] = (unsigned char)(src); \
	(dst)[1] = (unsigned char)((src) >> 8); \
	(dst)[2] = (unsigned char)((src) >> 16); \
	(dst)[3] = (unsigned char)((src) >> 24);

void MD4_Final(unsigned char *result, MD4_CTX *ctx)
{
	unsigned long used, available;

	used = ctx->lo & 0x3f;

	ctx->buffer[used++] = 0x80;

	available = 64 - used;

	if (available < 8) {
		memset(&ctx->buffer[used], 0, available);
		body(ctx, ctx->buffer, 64);
		used = 0;
		available = 64;
	}

	memset(&ctx->buffer[used], 0, available - 8);

	ctx->lo <<= 3;
	OUT(&ctx->buffer[56], ctx->lo)
	OUT(&ctx->buffer[60], ctx->hi)

	body(ctx, ctx->buffer, 64);

	OUT(&result[0], ctx->A)
	OUT(&result[4], ctx->B)
	OUT(&result[8], ctx->C)
	OUT(&result[12], ctx->D)

#if 0
	memset(ctx, 0, sizeof(*ctx));
#endif
}

#endif /* !HAVE_LIBCRYPTO */

#undef INIT_A
#undef INIT_B
#undef INIT_C
#undef INIT_D
#undef SQRT_3
#define INIT_A 0x67452301
#define INIT_B 0xefcdab89
#define INIT_C 0x98badcfe
#define INIT_D 0x10325476
#define SQRT_3 0x6ed9eba1

void md4_reverse(uint32_t *hash)
{
	hash[0] -= INIT_A;
	hash[1] -= INIT_B;
	hash[2] -= INIT_C;
	hash[3] -= INIT_D;
	hash[1]  = (hash[1] >> 15) | (hash[1] << 17);
	hash[1] -= SQRT_3 + (hash[2] ^ hash[3] ^ hash[0]);
	hash[1]  = (hash[1] >> 15) | (hash[1] << 17);
	hash[1] -= SQRT_3;
}

void md4_unreverse(uint32_t *hash)
{
	hash[1] += SQRT_3;
	hash[1]  = (hash[1] >> 17) | (hash[1] << 15);
	hash[1] += SQRT_3 + (hash[2] ^ hash[3] ^ hash[0]);
	hash[1]  = (hash[1] >> 17) | (hash[1] << 15);
	hash[3] += INIT_D;
	hash[2] += INIT_C;
	hash[1] += INIT_B;
	hash[0] += INIT_A;
}
