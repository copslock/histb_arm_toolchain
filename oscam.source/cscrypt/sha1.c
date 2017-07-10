#ifndef WITH_LIBCRYPTO
//FIXME Not checked on threadsafety yet; after checking please remove this line
/*
SHA-1 in C
By Steve Reid <sreid@sea-to-sky.net>
100% Public Domain

-----------------
Modified 7/98
By James H. Brown <jbrown@burgoyne.com>
Still 100% Public Domain

Corrected a problem which generated improper hash values on 16 bit machines
Routine SHA1Update changed from
    void SHA1Update(SHA_CTX* context, unsigned char* data, unsigned int
len)
to
    void SHA1Update(SHA_CTX* context, unsigned char* data, unsigned
long len)

The 'len' parameter was declared an int which works fine on 32 bit machines.
However, on 16 bit machines an int is too small for the shifts being done
against
it.  This caused the hash function to generate incorrect values if len was
greater than 8191 (8K - 1) due to the 'len << 3' on line 3 of SHA1Update().

Since the file IO in main() reads 16K at a time, any file 8K or larger would
be guaranteed to generate the wrong hash (e.g. Test Vector #3, a million
"a"s).

I also changed the declaration of variables i & j in SHA1Update to
unsigned long from unsigned int for the same reason.

These changes should make no difference to any 32 bit implementations since
an
int and a long are the same size in those environments.

--
I also corrected a few compiler warnings generated by Borland C.
1. Added #include <process.h> for exit() prototype
2. Removed unused variable 'j' in SHA1Final
3. Changed exit(0) to return(0) at end of main.

ALL changes I made can be located by searching for comments containing 'JHB'
-----------------
Modified 8/98
By Steve Reid <sreid@sea-to-sky.net>
Still 100% public domain

1- Removed #include <process.h> and used return() instead of exit()
2- Fixed overwriting of finalcount in SHA1Final() (discovered by Chris Hall)
3- Changed email address from steve@edmweb.com to sreid@sea-to-sky.net

-----------------
Modified 4/01
By Saul Kravitz <Saul.Kravitz@celera.com>
Still 100% PD
Modified to run on Compaq Alpha hardware.

-----------------
Modified 07/2002
By Ralph Giles <giles@ghostscript.com>
Still 100% public domain
modified for use with stdint types, autoconf
code cleanup, removed attribution comments
switched SHA1Final() argument order for consistency
use SHA1_ prefix for public api
move public api to sha1.h
*/

/*
Test Vectors (from FIPS PUB 180-1)
"abc"
  A9993E36 4706816A BA3E2571 7850C26C 9CD0D89D
"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
  84983E44 1C3BD26E BAAE4AA1 F95129E5 E54670F1
A million repetitions of "a"
  34AA973C D4C4DAA4 F61EEB2B DBAD2731 6534016F
*/

/* #define SHA1HANDSOFF  */

#include <stdio.h>
#include <string.h>
// sky(n)
#if defined(WITH_HISILICON)
#include <endian.h>
#endif
#include "sha1.h"

#define rol(value,bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

/* blk0() and blk() perform the initial expand. */
/* I got the idea of expanding during the round function from SSLeay */
/* FIXME: can we do this in an endian-proof way? */
#if __BYTE_ORDER == __BIG_ENDIAN
#define blk0(i) 	block->l[i]
#else
#define blk0(i) 	(block->l[i] = (rol(block->l[i],24)&0xFF00FF00) \
				    	|(rol(block->l[i],8)&0x00FF00FF))
#endif
#define blk(i) 	(block->l[i&15] = rol(block->l[(i+13)&15]^block->l[(i+8)&15] \
				    	^block->l[(i+2)&15]^block->l[i&15],1))

/* (R0+R1), R2, R3, R4 are the different operations used in SHA1 */
#define R0(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk0(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R1(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R2(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0x6ED9EBA1+rol(v,5);w=rol(w,30);
#define R3(v,w,x,y,z,i) z+=(((w|x)&y)|(w&x))+blk(i)+0x8F1BBCDC+rol(v,5);w=rol(w,30);
#define R4(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0xCA62C1D6+rol(v,5);w=rol(w,30);


#ifdef VERBOSE  /* SAK */
void SHAPrintContext(SHA_CTX *context, char *msg) {
	printf("%s (%d,%d) %x %x %x %x %x\n",
			msg,
			context->count[0], context->count[1],
			context->state[0],
			context->state[1],
			context->state[2],
			context->state[3],
			context->state[4]);
}
#endif /* VERBOSE */

/* Hash a single 512-bit block. This is the core of the algorithm. */
void SHA1_Transform(uint32_t state[5], const uint8_t buffer[64])
{
	uint32_t a, b, c, d, e;
	typedef union {
		uint8_t  c[64];
		uint32_t l[16];
	} CHAR64LONG16;
	CHAR64LONG16 *block;

#ifdef SHA1HANDSOFF
	static uint8_t workspace[64];
	block = (CHAR64LONG16*)workspace;
	memcpy(block, buffer, 64);
#else
	block = (CHAR64LONG16*)buffer;
#endif

	/* Copy context->state[] to working vars */
	a = state[0];
	b = state[1];
	c = state[2];
	d = state[3];
	e = state[4];

	/* 4 rounds of 20 operations each. Loop unrolled. */
	R0(a,b,c,d,e, 0); R0(e,a,b,c,d, 1); R0(d,e,a,b,c, 2); R0(c,d,e,a,b, 3);
	R0(b,c,d,e,a, 4); R0(a,b,c,d,e, 5); R0(e,a,b,c,d, 6); R0(d,e,a,b,c, 7);
	R0(c,d,e,a,b, 8); R0(b,c,d,e,a, 9); R0(a,b,c,d,e,10); R0(e,a,b,c,d,11);
	R0(d,e,a,b,c,12); R0(c,d,e,a,b,13); R0(b,c,d,e,a,14); R0(a,b,c,d,e,15);
	R1(e,a,b,c,d,16); R1(d,e,a,b,c,17); R1(c,d,e,a,b,18); R1(b,c,d,e,a,19);
	R2(a,b,c,d,e,20); R2(e,a,b,c,d,21); R2(d,e,a,b,c,22); R2(c,d,e,a,b,23);
	R2(b,c,d,e,a,24); R2(a,b,c,d,e,25); R2(e,a,b,c,d,26); R2(d,e,a,b,c,27);
	R2(c,d,e,a,b,28); R2(b,c,d,e,a,29); R2(a,b,c,d,e,30); R2(e,a,b,c,d,31);
	R2(d,e,a,b,c,32); R2(c,d,e,a,b,33); R2(b,c,d,e,a,34); R2(a,b,c,d,e,35);
	R2(e,a,b,c,d,36); R2(d,e,a,b,c,37); R2(c,d,e,a,b,38); R2(b,c,d,e,a,39);
	R3(a,b,c,d,e,40); R3(e,a,b,c,d,41); R3(d,e,a,b,c,42); R3(c,d,e,a,b,43);
	R3(b,c,d,e,a,44); R3(a,b,c,d,e,45); R3(e,a,b,c,d,46); R3(d,e,a,b,c,47);
	R3(c,d,e,a,b,48); R3(b,c,d,e,a,49); R3(a,b,c,d,e,50); R3(e,a,b,c,d,51);
	R3(d,e,a,b,c,52); R3(c,d,e,a,b,53); R3(b,c,d,e,a,54); R3(a,b,c,d,e,55);
	R3(e,a,b,c,d,56); R3(d,e,a,b,c,57); R3(c,d,e,a,b,58); R3(b,c,d,e,a,59);
	R4(a,b,c,d,e,60); R4(e,a,b,c,d,61); R4(d,e,a,b,c,62); R4(c,d,e,a,b,63);
	R4(b,c,d,e,a,64); R4(a,b,c,d,e,65); R4(e,a,b,c,d,66); R4(d,e,a,b,c,67);
	R4(c,d,e,a,b,68); R4(b,c,d,e,a,69); R4(a,b,c,d,e,70); R4(e,a,b,c,d,71);
	R4(d,e,a,b,c,72); R4(c,d,e,a,b,73); R4(b,c,d,e,a,74); R4(a,b,c,d,e,75);
	R4(e,a,b,c,d,76); R4(d,e,a,b,c,77); R4(c,d,e,a,b,78); R4(b,c,d,e,a,79);

	/* Add the working vars back into context.state[] */
	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;
	state[4] += e;

	/* Wipe variables */
	a = b = c = d = e = 0;
}


/* SHA1Init - Initialize new context */
void SHA1_Init(SHA_CTX* context)
{
	/* SHA1 initialization constants */
	context->state[0] = 0x67452301;
	context->state[1] = 0xEFCDAB89;
	context->state[2] = 0x98BADCFE;
	context->state[3] = 0x10325476;
	context->state[4] = 0xC3D2E1F0;
	context->count[0] = context->count[1] = 0;
}


/* Run your data through this. */
void SHA1_Update(SHA_CTX* context, const uint8_t* data, const size_t len)
{
	size_t i, j;

#ifdef VERBOSE
	SHAPrintContext(context, "before");
#endif

	j = (context->count[0] >> 3) & 63;
	if ((context->count[0] += len << 3) < (len << 3)) context->count[1]++;
	context->count[1] += (len >> 29);
	if ((j + len) > 63) {
		memcpy(&context->buffer[j], data, (i = 64-j));
		SHA1_Transform(context->state, context->buffer);
		for ( ; i + 63 < len; i += 64) {
			SHA1_Transform(context->state, data + i);
		}
		j = 0;
	}
	else i = 0;
	memcpy(&context->buffer[j], &data[i], len - i);

#ifdef VERBOSE
	SHAPrintContext(context, "after ");
#endif
}


/* Add padding and return the message digest. */
void SHA1_Final(uint8_t digest[SHA_DIGEST_LENGTH], SHA_CTX* context)
{
	uint32_t i;
	uint8_t  finalcount[8];

	for (i = 0; i < 8; i++) {
		finalcount[i] = (unsigned char)((context->count[(i >= 4 ? 0 : 1)]
							>> ((3-(i & 3)) * 8) ) & 255);  /* Endian independent */
	}
	SHA1_Update(context, (uint8_t *)"\200", 1);
	while ((context->count[0] & 504) != 448) {
		SHA1_Update(context, (uint8_t *)"\0", 1);
	}
	SHA1_Update(context, finalcount, 8);  /* Should cause a SHA1_Transform() */
	for (i = 0; i < SHA_DIGEST_LENGTH; i++) {
		digest[i] = (uint8_t)
						((context->state[i>>2] >> ((3-(i & 3)) * 8) ) & 255);
	}

	/* Wipe variables */
	i = 0;
	memset(context->buffer, 0, 64);
	memset(context->state, 0, 20);
	memset(context->count, 0, 8);
	memset(finalcount, 0, 8);   /* SWR */

#ifdef SHA1HANDSOFF  /* make SHA1Transform overwrite its own static vars */
	SHA1_Transform(context->state, context->buffer);
#endif
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
// sky(n)
#define _sha_t32_(x)			((x) & 0xffffffff)
// rotate left
#define _sha_role_(x, n)	_sha_t32_(((x) << (n)) | ((x) >> (32 - (n))))
// convert DWord to little endian format (flop bytes)
#define _sha_fole_(x)		((((x) >> 24) & 0xFF) | (((x) >> 8) & 0xFF00) | (((x) & 0xFF) << 24) | (((x) & 0xFF00) << 8))
/* The SHS f()-functions */
#define _sha_f1_(x,y,z)		( ( x & y ) | ( ~(x) & z ) )		 	/* rounds  0-19 */
#define _sha_f2_(x,y,z)		( x ^ y ^ z )			  					/* rounds 20-39 */
#define _sha_f3_(x,y,z)		( ( x & y ) | ( x & z ) | ( y & z ) ) 	/* rounds 40-59 */
#define _sha_f4_(x,y,z)		( x ^ y ^ z )			  				/* rounds 60-79 */
#define _sha_swap32_(x)	 ((((x)&0xff000000)>>24) | (((x)&0x00ff0000)>> 8) | \
                   		  (((x)&0x0000ff00)<< 8) | (((x)&0x000000ff)<< 24))
#define _sha_swap16_(x)   (((x)&0x00ff)<<8 | ((x)&0xff00)>>8)

// The SHS Mysterious Constants
static 	uint32_t sha_Magic[4] = {0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6};


static uint32_t
SHA1_sekaFs(int iround, uint32_t b, uint32_t c, uint32_t d)
{
   if (iround < 20)  return (uint32_t)_sha_f1_(b,c,d);
   else if (iround < 40) return (uint32_t)_sha_f2_(b,c,d);
   else if (iround < 60) return (uint32_t)_sha_f3_(b,c,d);
   else return (uint32_t)_sha_f4_(b,c,d);
}

void
SHA1_sekaDes(uint8_t *src_p, int msg_len, uint8_t *digest, uint8_t shapad, uint8_t padend)
{
	uint32_t state[5]; 		// SHA-1 state buffer
	uint8_t 	databuf[256];
	uint32_t	W[80];
	uint32_t a,b,c,d,e,t; 	// copies of state buffer
	int i,j,blocks;

	// SHA1 initialization constants
	state[0] = 0x67452301;
	state[1] = 0xEFCDAB89;
	state[2] = 0x98BADCFE;
	state[3] = 0x10325476;
	state[4] = 0xC3D2E1F0;
	// count the number of 512 bit blocks in message

	memcpy(databuf, src_p, msg_len);
	if (!(shapad|padend))
	{
		blocks = (msg_len + 63) >> 6;
		for (i=msg_len;i<64*blocks;i++)
		{
			databuf[i] = 0;
		}
	}
	else
	{
		blocks = (msg_len + 3 + 63) >> 6;
		databuf[(64*blocks)-2]	= 0;
		databuf[(64*blocks)-1]	= msg_len;
		databuf[msg_len] = padend;
		for (i=msg_len+1;i<(64*blocks)-2;i++)
		{
			databuf[i] = shapad;
		}
	}

	for (i=0;i<blocks;i++)
	{
		memcpy(W, databuf+(64*i), 64);
		for (j= 0;j<16;j++) W[j] = _sha_fole_(W[j]);

		// expand data buffer
		for (j=16;j<80;j++) W[j] = _sha_role_(W[j-3] ^ W[j-8] ^ W[j-14] ^ W[j-16], 1);
		// Copy state[] to work vars
		a = state[0];
		b = state[1];
		c = state[2];
		d = state[3];
		e = state[4];
		// run core function
		for (j=0; j<80;j++)
		{
			t = (uint32_t)(_sha_role_(a,5) + SHA1_sekaFs(j,b,c,d) + e + W[j] + sha_Magic[j/20]);
			e = d;
			d = c;
			c = _sha_role_(b,30);
			b = a;
			a = t;
		}
		// Update state buffer
		state[0] += a;
		state[1] += b;
		state[2] += c;
		state[3] += d;
		state[4] += e;
	}
	// convert state src_p to byte format
	state[0] = _sha_fole_(state[0]);
	state[1] = _sha_fole_(state[1]);
	state[2] = _sha_fole_(state[2]);
	state[3] = _sha_fole_(state[3]);
	state[4] = _sha_fole_(state[4]);

	memcpy(digest, state, 20);
}



void
SHA1_sekaSignature(uint8_t *data, int n, uint8_t *signature, int PadMode, uint8_t Pad, uint8_t End)
{
	SHA_CTX ctx;
	uint16_t s;
	uint8_t SHA_pad;
	uint8_t SHA_end;
	int l;

	SHA1_Init(&ctx);
	SHA1_Update(&ctx,data,n);
	if (PadMode)
	{
		SHA_end = End;
		SHA1_Update(&ctx,&SHA_end,1);
		l=(n&63)+1;
		SHA_pad = Pad;
		if (l>62)
		{
			for(; l<64; l++) SHA1_Update(&ctx,&SHA_pad,1);
			l=0;
		}
		for(; l<62; l++) SHA1_Update(&ctx,&SHA_pad,1);
		s=_sha_swap16_(n);
		SHA1_Update(&ctx, (uint8_t*)&s, 2);
	}
	else
	{
		SHA_pad=0;
		for(; n&63; n++) SHA1_Update(&ctx,&SHA_pad,1);
	}
	*((unsigned int *)(signature   ))=_sha_swap32_(ctx.state[0]);
	*((unsigned int *)(signature+ 4))=_sha_swap32_(ctx.state[1]);
	*((unsigned int *)(signature+ 8))=_sha_swap32_(ctx.state[2]);
	*((unsigned int *)(signature+12))=_sha_swap32_(ctx.state[3]);
	*((unsigned int *)(signature+16))=_sha_swap32_(ctx.state[4]);
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
#endif