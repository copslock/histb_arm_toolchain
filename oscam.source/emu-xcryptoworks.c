#include "globals.h"
#if defined(MODULE_XCAS)
#include "oscam-client.h"
#include "oscam-ecm.h"
#include "oscam-net.h"
#include "oscam-chk.h"
#include "oscam-string.h"
#include "cscrypt/bn.h"
#include "module-xcas.h"
#if defined(__XCAS_CRYPTOWORKS__)
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
typedef struct
{
	uint16_t casysid;
	uint8_t  provf[4];
	uint8_t  systemkey[6];
} CTW_SYSKDATA;

static CTW_SYSKDATA 	ctw_systemkeys[] =
{
	// Digiturk Cryptoworks Eutelsat (7.0??E)
	{ 0x0D00,	{0xC0,0xC4,0xC8,0xCC,},		{0x01,0x56,0x12,0xE8,0xEE,0x33},},
	// 5??E : Kingston/Arquiva 12111H
	{ 0x0D00,	{0x20,0x30,0x10,0x00,},		{0x23,0x90,0x91,0x78,0x1C,0x7E},},

	{ 0x0D01,	{0x50,0x54,0x00,0x00,},		{0xE0,0xE6,0xB0,0xBA,0x31,0x25},},
 	//	13??E : E! Europe & Jetix 10723H & JSTV 1 & 2 12597V
	{ 0x0D01,	{0xE4,0xE8,0xEC,0xF0,},		{0xC2,0x3F,0xFA,0x92,0x19,0xAB},},
 	{ 0x0D02,	{0x88,0x8C,0x00,0x00,},		{0x8A,0xD8,0xBE,0x27,0x92,0x21},},
 	//	UXP TEST(ASTRA 10921)
// { 0x0D02,	{0xA0,0x00,0x00,0x00,},		{0xC8,0x79,0x1A,0x66,0xBE,0x69,},
 	{ 0x0D02,	{0xA0,0x00,0x00,0x00,},		{0x62,0x7A,0x47,0xEE,0x58,0xA7},},

 	//	23??E : Czech Link 12525V
	{ 0x0D03,	{0x04,0x08,0x00,0x00,},		{0x27,0xB4,0x88,0x34,0x8D,0x54},},
 	//	23??E : Slo-vak Link 12565H
	{ 0x0D03,	{0x20,0x00,0x00,0x00,},		{0x91,0x47,0x55,0x0F,0x41,0x9C},},
 	//	19??E : Turner 11778V
 	// 0x5C ???????????
//	{ 0x0D03,	{0x5C,0x00,0x00,0x00,},		{0x27,0xB4,0x88,0x34,0x8D,0x54},},
	{ 0x0D03,	{0x5C,0x00,0x00,0x00,},		{0xDE,0x20,0x34,0x14,0x54,0x55},},
	{ 0x0D03,	{0x60,0x00,0x00,0x00,},		{0xDE,0x20,0x34,0x14,0x54,0x55},},
 	//	BritishTelecom 13.0??E
	{ 0x0D03,	{0xA0,0xCC,0x00,0x00,},		{0xC8,0x79,0x1A,0x66,0xBE,0x69},},
 	//	19??E : XtraMusic
	{ 0x0D03,	{0xB0,0x00,0x00,0x00,},		{0x5E,0x13,0x97,0xA6,0x59,0x7E},},
	//	42??E & 7.0??E : Digiturk
	{ 0x0D03,	{0xC0,0xC4,0x00,0x00,},		{0x64,0x2F,0x15,0xA3,0xBC,0x63},},

	//	15??W : DMC 11124H & 11150H.
	{ 0x0D04,	{0x04,0x08,0x0C,0x10,},		{0x28,0xD6,0xF3,0xBF,0x1B,0x76},},
	//	ORF/Easy TV
	{ 0x0D05,	{0x04,0x08,0x0C,0x10,},		{0x09,0x87,0xCF,0x2A,0x61,0x1D},},

	{ 0x0D08,	{0x20,0x30,0x00,0x00,},		{0x23,0x90,0x91,0x78,0x1C,0x7E},},
	{ 0x0D22,	{0x06,0x00,0x00,0x00,},		{0x09,0x87,0xCF,0x2A,0x61,0x1D},},

	//	ORF
	{ 0x0D95,	{0x04,0x08,0x00,0x00,},		{0x34,0x65,0x29,0x96,0x7A,0xDF},},
};
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
typedef struct
{
	uint16_t casysid;
	uint16_t provf;
	uint8_t  ab0key[16];
	uint8_t  ab1key[16];
	uint8_t  abRsa	[64];
} CTW_KEYDATA;

static CTW_KEYDATA 	ctw_keys[] =
{
	{	// Digiturk Cryptoworks Eutelsat (7.0??E)
		 0x0D00,   0xC8,
		{0x22,0xEC,0xB8,0xB2,0x43,0x85,0xC3,0xB2,0x94,0x1E,0xF7,0xEC,0xC2,0xB5,0x4A,0x09,},
		{0xC0,0xC6,0x86,0x55,0xB6,0x33,0x04,0x08,0x5B,0x5F,0x72,0x2D,0xD4,0x71,0xFE,0x27,},
		{0x9D,0x5F,0x83,0xA2,0xD8,0x4C,0x03,0xF1,0x00,0xAA,0xF7,0x24,0x98,0xAD,0x74,0xB8,
		 0x99,0x95,0x79,0x4D,0x8F,0x28,0x82,0xDD,0x5F,0xDE,0x26,0xEB,0xB1,0x0A,0x50,0x4E,
		 0x78,0x76,0x5E,0x4D,0x59,0x5E,0xD4,0xD7,0x20,0x2E,0x0E,0x00,0x2F,0x41,0x02,0x8B,
		 0xCD,0xAF,0x45,0x69,0xFE,0x27,0xB5,0x3F,0x0A,0xCF,0x2D,0x90,0x63,0x96,0x00,0x80,},
	},
	{	// Czech Link
		 0x0D03,   0x04,
	//	{0x1B,0x94,0xCA,0xED,0x06,0x00,0x01,0x54,0x1B,0x94,0xCA,0xED,0x06,0x00,0x01,0x54,},
		{0x72,0x55,0x91,0xA1,0x9F,0x0C,0x8F,0x38,0x97,0x37,0x78,0xFF,0x7E,0x57,0x7C,0x04,},
		{0x15,0x44,0x2A,0xF4,0xF4,0x5D,0xA4,0x5A,0x02,0xCF,0xFE,0x37,0xCE,0xC2,0xB6,0x07,},
		{0x09,0x05,0x8B,0x5B,0xD4,0x4D,0xE7,0xDB,0x83,0x6C,0x65,0xE7,0xC1,0xA6,0x0E,0xAE,
		 0xBF,0xD7,0x94,0xD8,0x89,0x74,0xE1,0x1F,0xB8,0x2F,0x46,0x6A,0x41,0xE6,0xC5,0x33,
		 0x5B,0x33,0x8F,0xDF,0x11,0x46,0xC5,0xA5,0xBE,0x90,0xF8,0xA9,0x75,0x8B,0x63,0x77,
		 0x6B,0x55,0x0E,0x9A,0x4F,0x39,0x1E,0xFF,0xCC,0x63,0xA9,0xC8,0x29,0x4E,0x00,0x80,},
	},
	//-------------------------------------------------------------------------------------
	{	 0x0D03,   0x20,
		{0xD7,0x40,0x79,0xB6,0x32,0x34,0x9E,0x76,0xCA,0x84,0x03,0x21,0xAB,0x92,0x35,0x33,},
		{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,},
		{0x59,0xD7,0x52,0x4B,0x66,0xCB,0xE0,0x5F,0x2A,0x55,0x0B,0xA8,0xD1,0xE4,0x63,0x5B,
		 0xFB,0xF8,0xCE,0xC3,0x60,0x98,0x87,0xE1,0x21,0x22,0xF6,0x6E,0xCD,0xD1,0xAA,0x1B,
		 0xAD,0x5F,0x70,0x6A,0x2E,0xF1,0xE5,0xBB,0x6C,0x78,0x01,0xFC,0x62,0x4B,0x29,0xDC,
		 0x64,0x4C,0xB0,0x75,0x4D,0xB7,0xF5,0x90,0xAD,0xA3,0x20,0x1D,0x9F,0x64,0x00,0x80,},
	},
	//-------------------------------------------------------------------------------------
	{	// turner B0 tonytr 2007-09-05 22:01:03
		 0x0D03,   0x5C,
		{0x9A,0x0A,0x10,0x9C,0x23,0xA5,0xC2,0x17,0x5B,0x1F,0x87,0x03,0x38,0xCD,0xCE,0xC4,},
		{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,},
		{0x7D,0xD0,0xBF,0x66,0x92,0xE9,0xD4,0x5B,0xFA,0x69,0x97,0xA6,0x59,0x2E,0x90,0xAE,
		 0xD2,0xF9,0xA6,0x62,0x4B,0xA9,0x1D,0x1B,0xF8,0xA4,0xDC,0x16,0xEF,0x18,0x7E,0x02,
		 0xB6,0x66,0xF1,0x77,0xA0,0x23,0x23,0xDC,0x97,0x66,0x5F,0x3C,0x61,0x97,0x0B,0xCE,
		 0x0A,0xEF,0x63,0xE7,0x53,0x9B,0x36,0xAB,0x9B,0xF5,0x8B,0x20,0x4B,0x5D,0x00,0x80,},
	},
	//-------------------------------------------------------------------------------------
	{	//	19??E : XtraMusic
		 0x0D03,   0xB0,
		{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,},
		{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,},
		{0x39,0xFA,0x13,0x22,0x59,0xC6,0x4C,0x7C,0xF5,0x5D,0x3D,0x6F,0xE8,0x32,0x0B,0x64,
		 0x4F,0xE8,0x2B,0x96,0x68,0x1D,0x54,0x61,0x21,0xA0,0xD2,0xB6,0xD0,0x51,0xE5,0x68,
		 0x05,0xE0,0x6E,0xB0,0x67,0x80,0x2A,0xCE,0xD1,0x72,0xF3,0xF8,0x7E,0x0D,0xE9,0x1B,
		 0x9B,0x96,0xE8,0x46,0x38,0x30,0x15,0xC9,0xD4,0xD5,0x84,0x42,0xF5,0x9D,0x00,0x80,},
	},
	//-------------------------------------------------------------------------------------
	{	//	42??E & 7.0??E : Digiturk
		 0x0D03,   0xC0,
		{0xB7,0x52,0xC3,0xBE,0x0B,0xDE,0xEB,0x8A,0x32,0xA3,0xE7,0x3E,0x63,0x58,0xF0,0x44,},
		{0x28,0xE3,0xAF,0x36,0xBB,0xC2,0xBC,0x92,0x40,0x7D,0xAA,0xC2,0x38,0xD1,0x2A,0xDF,},
		{0x21,0x4D,0xA3,0x1E,0x63,0x5D,0x37,0xC9,0xA3,0x16,0x40,0xAE,0xC8,0xD2,0x14,0x5F,
		 0x3E,0x5A,0xE1,0x8F,0x1E,0xDF,0x73,0x68,0x35,0xAB,0x60,0x42,0x33,0x33,0xAB,0x8C,
		 0xEC,0x8B,0x6D,0x47,0x40,0x08,0xFB,0x50,0x31,0x20,0x0E,0x0F,0x68,0x73,0xFE,0x58,
		 0x87,0xF1,0x95,0xC1,0xE8,0x86,0x6D,0xB2,0x81,0xF8,0x49,0xE1,0x46,0xBA,0x00,0x80,},
	},
	//-------------------------------------------------------------------------------------
	{	//	42??E & 7.0??E : Digiturk
		 0x0D03,   0xC4,
		{0x00,0xB5,0x34,0xFF,0x03,0x2F,0x8A,0x5E,0xAC,0x0A,0xE6,0x3E,0x99,0x70,0xEB,0x74,},
		{0xBC,0xF2,0x15,0x87,0x22,0x0E,0x44,0xE3,0xCF,0x51,0x60,0x43,0x33,0xE2,0x5F,0x54,},
		{0x05,0x48,0x03,0xCE,0x3D,0x7F,0x3A,0x6D,0x98,0xE1,0xAB,0x3E,0x94,0x7F,0x67,0x49,
		 0x87,0x79,0x9B,0x17,0x26,0x23,0x8C,0xCD,0x6F,0x16,0x12,0x5C,0xB3,0x93,0xA6,0x56,
		 0x8A,0x11,0xFC,0xA4,0xD3,0x06,0x56,0x94,0x87,0x76,0x1D,0xEF,0xD6,0x44,0xC8,0x3B,
		 0x4C,0x36,0x81,0x46,0x0D,0x7C,0x3E,0x74,0x7A,0x01,0xAC,0x70,0x04,0xC7,0x00,0x80,},
	},
	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------
	{	//	(ORF 19E)
		 0x0D05,   0x04,
	//	{0x07,0x16,0x5D,0xF9,0xAC,0x22,0x6C,0x9A,0x38,0x46,0x0C,0x26,0xA2,0xD8,0x4F,0xBF,},
		{0xA9,0xD7,0x32,0xF5,0xE7,0x38,0xE4,0x8A,0x91,0xC8,0x63,0x8B,0x12,0x74,0x66,0x61,},
		{0x2D,0xF2,0x5D,0xC7,0x8D,0x09,0x88,0x60,0xB8,0xC6,0x28,0x11,0x40,0x22,0x4E,0xBD,},
		//	EMM_G:D0B2E2DCC581F4DE71BD69845F67229B, 35F16B7AE5B2458A81D7BA6C1A86AA62
		{0xCD,0x88,0xAC,0x49,0x94,0x57,0x7E,0x06,0x71,0xA8,0x8D,0x65,0xC4,0x24,0x76,0xB1,
		 0x62,0xED,0xD8,0x43,0x30,0x38,0x21,0x42,0x9B,0x14,0xB0,0x5C,0xD1,0x2F,0xC9,0x2C,
		 0xD0,0xA8,0xD3,0xDC,0xEB,0xD2,0xD2,0x67,0xD8,0x1B,0x0A,0x1A,0xD0,0x8E,0xF1,0x80,
		 0xED,0x8A,0x4D,0x59,0xFF,0x4D,0x9C,0x61,0x93,0x05,0x09,0x5A,0xE0,0xD1,0x00,0x80,},
	},
	//-------------------------------------------------------------------------------------
	{	//	Alpen TVX (19E)
		 0x0D05,   0x08,
		{0xC3,0x5C,0xCB,0xE9,0x54,0x29,0x81,0xD8,0x97,0x9C,0x6D,0x23,0xE3,0x8C,0x71,0xE0,},
		{0x62,0x47,0x2D,0xA7,0x3D,0xAF,0x2D,0x39,0xD8,0x78,0xB4,0x6C,0x32,0x97,0xE1,0x9D,},
		{0xE9,0x4B,0x79,0x5C,0xC4,0x93,0xA9,0x8A,0x85,0x59,0x69,0x9E,0x3F,0xAF,0x1E,0xF2,
		 0x09,0xD3,0x0D,0xDE,0xFA,0xDB,0x4B,0x3B,0x58,0x87,0x7B,0x8C,0x5F,0xFA,0x41,0x1D,
		 0x88,0x39,0x08,0xC4,0xAD,0x1C,0xC0,0xA3,0xAF,0xB8,0xFE,0x58,0x90,0xE2,0x01,0xAD,
		 0xF0,0x02,0x61,0x39,0x38,0xBB,0x82,0x0B,0x24,0xDF,0x29,0xF6,0xF0,0x6C,0x00,0x80},
	},
	//-------------------------------------------------------------------------------------
	{	//	EasyTv/ORF (19E)
		 0x0D05,   0x0C,
		{0x65,0xD4,0xE6,0xD5,0x37,0xE3,0xAD,0x18,0x4D,0xF4,0xD5,0x79,0x6A,0x8F,0x8A,0xED,},
		{0x7C,0x07,0xC8,0x5B,0xC1,0x26,0x22,0xB9,0x42,0x17,0x17,0x94,0x51,0x61,0x3C,0x88,},
		//	EMM_G:F5B7E5E82B600D8216D353EDFB189B13
		{0x9D,0x96,0x5D,0x72,0x43,0x2A,0xA5,0x11,0x86,0xBD,0x0C,0x98,0xCB,0xFE,0xB7,0x49,
		 0xD0,0x78,0x8C,0x6F,0x37,0x93,0x3D,0xB2,0xFA,0xED,0xEF,0xCB,0x47,0x3E,0x3D,0xF3,
		 0x09,0x34,0xDA,0x04,0x44,0x73,0x94,0xF9,0x55,0xD1,0xCA,0x47,0x3B,0x25,0x01,0xC7,
		 0xAC,0x0A,0x7E,0xC0,0xF4,0x3A,0xFD,0xB7,0xEF,0xC2,0x0B,0x17,0xE1,0xF4,0x00,0x80,},
	},
	//-------------------------------------------------------------------------------------
	{	//	Sky Austria (19E)
		 0x0D05,   0x10,
	//	{0x2D,0xF2,0x5D,0xC7,0x8D,0x09,0x88,0x60,0xB8,0xC6,0x28,0x11,0x40,0x22,0x4E,0xBD,},
	//	{0xB5,0x7B,0xBB,0x96,0x26,0xDA,0x9F,0xFB,0x21,0x8C,0x42,0x43,0x6E,0xD0,0xF1,0x8F,},
		{0x00,0x00,0x6A,0xB3,0x25,0xE6,0xAB,0xF4,0xDA,0x47,0x28,0x0F,0x35,0x29,0x4B,0x5C,},
	//	{0xC2,0x29,0xA4,0x03,0x61,0x00,0xE8,0x1A,0x42,0xBA,0xC3,0xC7,0x0E,0xBB,0xC8,0x52,},
		{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x7E,0x4D,0xAC,0x06,0x5B,0xA6,0x86,},
		{0x59,0x72,0x0F,0x3F,0x1B,0x74,0xA4,0xDB,0xF7,0x26,0x44,0x6C,0xCB,0xEE,0xAB,0x49,
		 0xA9,0x6E,0xF2,0xD2,0x08,0x40,0xA2,0x1F,0x1C,0x0B,0xD7,0x29,0x13,0xE6,0x3C,0x97,
		 0xEA,0x3B,0xE2,0xF0,0x4D,0x3D,0x27,0xDA,0x1F,0xBF,0xC5,0xBD,0xAD,0x5D,0x98,0x49,
		 0x50,0x1F,0x83,0xD3,0xC2,0x0C,0xF9,0x84,0xC0,0x1F,0x72,0xE0,0x38,0xA4,0x00,0x80,},
	},
	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------
	{	// Arena
		 0x0D22,   0x04,
		{0x65,0xD4,0xE6,0xD5,0x37,0xE3,0xAD,0x18,0x4D,0xF4,0xD5,0x79,0x6A,0x8F,0x8A,0xED,},
		{0x7C,0x07,0xC8,0x5B,0xC1,0x26,0x22,0xB9,0x42,0x17,0x17,0x94,0x51,0x61,0x3C,0x88,},
		{0x9D,0x96,0x5D,0x72,0x43,0x2A,0xA5,0x11,0x86,0xBD,0x0C,0x98,0xCB,0xFE,0xB7,0x49,
		 0xD0,0x78,0x8C,0x6F,0x37,0x93,0x3D,0xB2,0xFA,0xED,0xEF,0xCB,0x47,0x3E,0x3D,0xF3,
		 0x09,0x34,0xDA,0x04,0x44,0x73,0x94,0xF9,0x55,0xD1,0xCA,0x47,0x3B,0x25,0x01,0xC7,
		 0xAC,0x0A,0x7E,0xC0,0xF4,0x3A,0xFD,0xB7,0xEF,0xC2,0x0B,0x17,0xE1,0xF4,0x00,0x80,},
	},
	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------
};

static int16_t		er_KFOUND;
static uint32_t	er_KPPID;
static uint8_t		er_KSYSKEY[6];
static uint8_t		er_KNR;
static uint8_t		er_KKEY[16];
static BIGNUM 		er_RN85;
static BIGNUM 		er_RExp;
// reader_common.c
extern int32_t rdr_RSA_decrypt(struct s_reader *reader, uint8_t *out, uint8_t *in, int32_t n, BIGNUM *exp, BIGNUM *mod, int32_t LE);
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//======================================================================
//======================================================================
//======================================================================
//======================================================================
//======================================================================
//======================================================================
//======================================================================
//======================================================================
// common Cryptoworks DES modes
#define DESCTWLEFT		0x0
#define DESCTWRIGHT		0x1
#define DESMOD1			0x2
#define DESN				0x4
#define DESMOD3			0x8

// common Cryptoworks DES modes
#define DES2DECRYPT		(DESN|DESMOD3|DESCTWLEFT)
#define DES2ENCRYPT		(DESN|DESMOD1|DESCTWRIGHT)
#define DES2N				(DESN)

#define _ror_1_(value) 	(((value>>1)&0x7fffffff)) | ((value&0x10)<<27);
#define _rol_1_(value) 	(((value<<1)&0xfffffffe)) | ((value&0x08000000)>>27);
#define _lwz_(rD,d,buf) (rD=buf[d+0]<<24|buf[d+1]<<16|buf[d+2]<<8|buf[d+3])


static uint8_t 	tab_SBOXES[4][0x40] =
{
	{0xD8,0xD7,0x83,0x3D,0x1C,0x8A,0xF0,0xCF,0x72,0x4C,0x4D,0xF2,0xED,0x33,0x16,0xE0,
	 0x8F,0x28,0x7C,0x82,0x62,0x37,0xAF,0x59,0xB7,0xE0,0x00,0x3F,0x09,0x4D,0xF3,0x94,
	 0x16,0xA5,0x58,0x83,0xF2,0x4F,0x67,0x30,0x49,0x72,0xBF,0xCD,0xBE,0x98,0x81,0x7F,
	 0xA5,0xDA,0xA7,0x7F,0x89,0xC8,0x78,0xA7,0x8C,0x05,0x72,0x84,0x52,0x72,0x4D,0x38,},
	{0xD8,0x35,0x06,0xAB,0xEC,0x40,0x79,0x34,0x17,0xFE,0xEA,0x47,0xA3,0x8F,0xD5,0x48,
	 0x0A,0xBC,0xD5,0x40,0x23,0xD7,0x9F,0xBB,0x7C,0x81,0xA1,0x7A,0x14,0x69,0x6A,0x96,
	 0x47,0xDA,0x7B,0xE8,0xA1,0xBF,0x98,0x46,0xB8,0x41,0x45,0x9E,0x5E,0x20,0xB2,0x35,
	 0xE4,0x2F,0x9A,0xB5,0xDE,0x01,0x65,0xF8,0x0F,0xB2,0xD2,0x45,0x21,0x4E,0x2D,0xDB,},
	{0xDB,0x59,0xF4,0xEA,0x95,0x8E,0x25,0xD5,0x26,0xF2,0xDA,0x1A,0x4B,0xA8,0x08,0x25,
	 0x46,0x16,0x6B,0xBF,0xAB,0xE0,0xD4,0x1B,0x89,0x05,0x34,0xE5,0x74,0x7B,0xBB,0x44,
	 0xA9,0xC6,0x18,0xBD,0xE6,0x01,0x69,0x5A,0x99,0xE0,0x87,0x61,0x56,0x35,0x76,0x8E,
	 0xF7,0xE8,0x84,0x13,0x04,0x7B,0x9B,0xA6,0x7A,0x1F,0x6B,0x5C,0xA9,0x86,0x54,0xF9,},
	{0xBC,0xC1,0x41,0xFE,0x42,0xFB,0x3F,0x10,0xB5,0x1C,0xA6,0xC9,0xCF,0x26,0xD1,0x3F,
	 0x02,0x3D,0x19,0x20,0xC1,0xA8,0xBC,0xCF,0x7E,0x92,0x4B,0x67,0xBC,0x47,0x62,0xD0,
	 0x60,0x9A,0x9E,0x45,0x79,0x21,0x89,0xA9,0xC3,0x64,0x74,0x9A,0xBC,0xDB,0x43,0x66,
	 0xDF,0xE3,0x21,0xBE,0x1E,0x16,0x73,0x5D,0xA2,0xCD,0x8C,0x30,0x67,0x34,0x9C,0xCB,},
};
static uint8_t 	tab_sb[4][8] =
{
	{0,0x40,4,0x80,0x21,0x10,2,8},
	{0x80,8,1,0x40,4,0x20,0x10,2},
	{0x82,0x40,1,0x10,0,0x20,4,8},
	{2,0x10,4,0x40,0x80,8,1,0x20},
};
static uint8_t 	tab_1002D85C[0x2C]=
{
	1,0,3,1,2,2,3,2,1,3,1,1,3,0,1,2,3,1,3,2,2,0,
	7,6,5,4,7,6,5,7,6,5,6,7,5,7,5,7,6,6,7,5,4,4
};
static uint8_t 	tab_1002D888[0x2C]=
{
	0x80,0x08,0x10,0x02,0x08,0x40,0x01,0x20,0x40,0x80,0x04,
	0x10,0x04,0x01,0x01,0x02,0x20,0x20,0x02,0x01,0x80,0x04,
	0x02,0x02,0x08,0x02,0x10,0x80,0x01,0x20,0x08,0x80,0x01,
	0x08,0x40,0x01,0x02,0x80,0x10,0x40,0x40,0x10,0x08,0x01
};
static uint8_t		tab_1002D8B4[ 4]={2,0x10,4,4};
static uint8_t		tab_1002D8E0[24]={1,1,2,2,2,2,2,2,1,2,2,2,2,2,2,1,1};
static uint8_t		tab_1002D8C0[24]={4,5,6,7,7,6,5,4,7,4,5,6,4,7,6,5,0,2,4,6,7,5,3,1};

//======================================================================
//======================================================================
//======================================================================
//======================================================================
//======================================================================
//======================================================================
//======================================================================
//======================================================================
static int
xcryptoworks_FindSystemkey(struct s_reader *reader, uint16_t casysid, uint8_t provf, uint8_t *systemkey)
{
	CTW_SYSKDATA *expi;
	uint32_t	ppid;
	int cMax;
	int i, found;

	// select suitable systemkey BYTES
	ppid = (casysid<<8) | (provf & 0xff);
	if (er_KPPID == ppid)
	{
		memcpy(systemkey, er_KSYSKEY, 6);
		MYEMU_TRACE("xcryptoworks:sys:%06X:%02X..%02X\n", ppid, systemkey[0], systemkey[5]);
		return 1;
	}

	found = XEMUKEY_SpecialSearch(reader, CASS_CRYPTOWORKS, ppid, "06", systemkey, 6);
	if ( found && cs_Iszero(systemkey,6)) found = 0;
	if (!found)
	{
		expi = (CTW_SYSKDATA *)ctw_systemkeys;
		cMax = sizeof(ctw_systemkeys)/sizeof(CTW_SYSKDATA);
		for (i=0; i<cMax; i++)
		{
			if (expi->casysid == casysid)
			{
				if (expi->provf[0] == provf) break;
				if (expi->provf[1] == provf) break;
				if (expi->provf[2] == provf) break;
				if (expi->provf[3] == provf) break;
			}
			expi++;
		}

		if (i==cMax)
		{
			MYEMU_TRACE("xcryptoworks:systemkey{%06x} not found\n", ppid);
			return 0;	// prov id not found
		}
		memcpy(systemkey, expi->systemkey, 6);
	}
	memcpy(er_KSYSKEY, systemkey, 6);
	MYEMU_TRACE("xcryptoworks:sys:%06X:%02X..%02X\n", ppid, systemkey[0], systemkey[5]);
	return 1;
}


static uint8_t *
xcryptoworks_FindRsakey(struct s_reader *reader, uint16_t casysid, uint8_t provf)
{
	static uint8_t seRsa[64];
	static uint8_t	seExp[1] = {0x2};
	CTW_KEYDATA *provi;
	uint32_t	ppid;
	int cMax;
	int i, found;

	ppid = (casysid<<8) | (provf & 0xff);
	if (er_KPPID == ppid)
	{
		MYEMU_TRACE("xcryptoworks:rsa:%06X:%02X..%02X\n", ppid, seRsa[0],  seRsa[63]);
		return (seRsa);
	}

	// select suitable RSA
	found = XEMUKEY_SpecialSearch(reader, CASS_CRYPTOWORKS, ppid, "B0", seRsa, 64);
	if ( found && cs_Iszero(seRsa,64)) found = 0;
	if (!found)
	{
		provi = (CTW_KEYDATA *)ctw_keys;
		cMax  = sizeof(ctw_keys)/sizeof(CTW_KEYDATA);
		for (i=0; i<cMax; i++)
		{
			if ((provi->casysid == casysid) && (provi->provf == provf)) break;
			provi++;
		}
		if (i==cMax)
		{
			MYEMU_TRACE("xcryptoworks:rsakey{%06x} not found\n", ppid);
			return 0;	// RSA not found
		}
		memcpy(seRsa, provi->abRsa, 64);
	}
	BN_bin2bn(seExp,  1, &er_RExp);
	BN_bin2bn(seRsa, 64, &er_RN85);
	MYEMU_TRACE("xcryptoworks:rsa:%06X:%02X..%02X\n", ppid, seRsa[0],  seRsa[63]);
	return (seRsa);
}



static int
xcryptoworks_FindDeskey(struct s_reader *reader, uint16_t casysid, uint8_t provf, uint8_t keyNr, uint8_t *cDeskey)
{
	CTW_KEYDATA *provi;
	uint32_t ppid;
	int cMax;
	int i, found;

	ppid = (casysid<<8) | (provf & 0xff);
	if (er_KPPID == ppid && er_KNR == keyNr)
	{
		memcpy(cDeskey, er_KKEY, 0x10);
		MYEMU_TRACE("xcryptoworks:des:%06X:%02X..%02X(%2X)\n", ppid, cDeskey[0], cDeskey[15], keyNr);
		return 1;
	}

	found = XEMUKEY_Searchkey(reader, CASS_CRYPTOWORKS, ppid, keyNr, cDeskey, 16);
	if ( found && cs_Iszero(cDeskey,16)) found = 0;
	if (!found)
	{
		provi = (CTW_KEYDATA *)ctw_keys;
		cMax  =	sizeof(ctw_keys)/sizeof(CTW_KEYDATA);
		for (i=0; i<cMax; i++)
		{
			if ((provi->casysid == casysid) && (provi->provf == provf)) break;
			provi++;
		}
		if (i==cMax)
		{
			MYEMU_TRACE("xcryptoworks:deskey{%06x} not found\n", ppid);
			return 0;	// RSA not found
		}
		memcpy(cDeskey, (keyNr & 0x1) ? provi->ab1key : provi->ab0key, 0x10);
	}
	er_KNR = keyNr;
	er_KPPID = ppid;
	er_KFOUND = 1;
	memcpy(er_KKEY, cDeskey, 0x10);
	MYEMU_TRACE("xcryptoworks:des:%06X:%02X..%02X(%2X)\n", ppid, cDeskey[0], cDeskey[15], keyNr);
	return 1;
}

//======================================================================
//======================================================================
//======================================================================
//======================================================================
//======================================================================
static void
xcryptoworks_DesPrepare(uint8_t *desKey)
{
	uint32_t r10,r11,temp, r0;
	uint8_t tempbuff[8];
	int i,j,k;

	memset(tempbuff,0,8);
	desKey[7]=6;

	for (i=6;i>=0;i--)
	{
		k=desKey[7];
		for (j=8;j>0;j--)
		{
			r0  = tempbuff[k];
			r0  = r0<<1;
			r11 = r0|1;
			if (!(desKey[i]&1)) r11=r0;
			desKey[i]=desKey[i]>>1;
			tempbuff[k]=r11;
			k--;
			if (k<0) k=6;
		}
		desKey[7]=k;
	}

	tempbuff[7]=(tempbuff[3]<<4)&0xF0;
	temp=tempbuff[6];
	tempbuff[6]=tempbuff[4];
	tempbuff[4]=temp;

	memcpy(desKey,tempbuff,8);

	_lwz_(r10,4,desKey);
	_lwz_(r11,0,desKey);
	for (i=0;i<4;i++) { r11 = _ror_1_(r11); r10 = _ror_1_(r10); }
	i2be(r11,desKey  ,4);
	i2be(r10,desKey+4,4);
}


static void
xcryptoworks_DesRound(uint8_t *buff, uint32_t left, uint32_t right)
{
	int r0,r3,r5,r6,r9,r10,r11,r26,r27,r30,r31;
	uint8_t tempbuff38[8], key[8];

	i2be(left, key  ,4);
	i2be(right,key+4,4);

	// step1 create tempbuff
	for (r10=0;r10<2;r10++)
	{
		for (r31=0;r31<4;r31++)
		{
			r9 = tab_1002D8C0[(4*r10)+r31];
			r0 = tab_1002D8C0[(4*r10)+r31+8];
			if (r10!=0) r0 = ((buff[r9]<<1)&0x3E) | ((buff[r0]&0x80)>>7);
			else
			{
				r6 = buff[r0]&1;
				r0 = (buff[r9]>>1) | 0x80;
				if (r6==0) r0 = buff[r9]>>1;
			}
			tempbuff38[tab_1002D8C0[(4*r10)+r31+0x10]]=r0&0xFF;
		}
	}
	// step2
	r26=1;	r27=0;
	for (r31=0; r31<8; r31++)
	{
		r26^=1;
		r30 =(r26^1)+6;

		if (r31!=0) r11 = r31-1;
		else r11 = 1;

		if (r26!=0) r9  = ((uint32_t)(-(key[r11] & tab_1002D8B4[(r31-1)>>1]))) >> 31;
		else r9  = tempbuff38[r11];

		for (r3=1; r30>r3; r3++)
		{
			r6  = (r9<<1)&0xFE;
			r5  = (r9>>1)&0x7F;
			r10 = tab_1002D888[r27];
			r0  = key[tab_1002D85C[r27]];

			if (r26!=0)
			{
				r9 = (r6|1) & 0xFF;
				if ((r0&r10)==0) r9=r6&0xFF;
			}
			else
			{
				r9 = r5|0x80;
				if ((r0&r10)==0) r9=r5&0xFF;
			}
			r27++;
		}

		if (r31!=0) r11++;
		else        r11=0;
		if (r26!=0) tempbuff38[r11] ^= r9;
		else        tempbuff38[r11]  = (tempbuff38[r11]^r9)/4;
	}

	for (r31=0; r31<8; r31++)
	{
		for (r0=0; r0<4; r0++) buff[r0] ^= (tab_sb[r0][r31] & tab_SBOXES[r0][tempbuff38[r31]]);
	}

	memcpy(tempbuff38,buff,4);
	memcpy(buff,buff+4,4);
	memcpy(buff+4,tempbuff38,4);

	_lwz_(right,4,key);
	_lwz_(left,0,key);
}

static void
xcryptoworks_DesProcess(uint8_t *key, int mode, uint8_t *CW)
{
	uint32_t left, right, iteration;
	int i, j, x;

	// split OpKey on 2 half
	_lwz_(left, 0,key);
	_lwz_(right,4,key);

	iteration = 16;
	if ((mode&DESMOD1) || (mode&DESMOD3))	iteration = 2;

	if (mode&DESMOD3)
	{
		for (i=2;i>0;i--) { left = _rol_1_(left); right = _rol_1_(right); }
	}

	if (mode&DESN)
	{
		for (j=iteration-1; j>=0; j--)
		{
			x=j;
			if (mode&DESCTWRIGHT)
			{
				if (mode&DESMOD1) x=0;
				for (i=0; i<tab_1002D8E0[x+1]; i++) {	left = _rol_1_(left); right = _rol_1_(right); }
			}
			xcryptoworks_DesRound(CW, left, right);
			if (!(mode&DESCTWRIGHT))
			{
				if (mode&DESMOD3) x=0;
				for (i=0; i<tab_1002D8E0[x]; i++) { left = _ror_1_(left); right = _ror_1_(right); }
			}
		}
	}
}

static void
xcryptoworks_DesAlgo(uint8_t *CW, uint8_t *key, int odd, int desmode)
{
	uint8_t keytemp70[0x16];
	uint8_t desKey[8], desKey2[8];
	int r31, dmode;

	memcpy(keytemp70,key,0x16);
	memset(keytemp70,0,0x10);
	memcpy(keytemp70,key+0x10,6);

	odd = (~odd)&7;
	dmode = desmode&1;

	for (r31=8; r31>odd; r31--) keytemp70[r31-2] = key[r31];
	xcryptoworks_DesPrepare(keytemp70);
	memcpy(desKey, keytemp70, 8);
	memcpy(desKey2, key+9, 8);
	xcryptoworks_DesPrepare(desKey2);
	xcryptoworks_DesProcess(desKey,  DES2ENCRYPT, CW);	// DES  2 rounds encrypt
	xcryptoworks_DesProcess(desKey2, DES2N|dmode, CW);	// DES 16 round decrypt
	xcryptoworks_DesProcess(desKey,  DES2DECRYPT, CW);	// DES  2 rounds decrypt
	memcpy(desKey,CW,4);
	memcpy(CW,CW+4,4);
	memcpy(CW+4,desKey,4);

	if (desmode&2)
	{
		memcpy(desKey,key,8);
		memcpy(key,key+8,8);
		memcpy(key+8,desKey,8);
	}
}


static int
xcryptoworks_n85Decrypt(struct s_reader *reader,
	uint8_t *Encr_p,
	uint8_t *cDeskey,
	int odd,
	int casysid,
	int provf,
	uint8_t *Decr_p)
{
	static uint8_t	sePermute[16]= {0xE,3,5,8,9,4,2,0xF,0,0xD,0xB,6,7,0xA,0xC,1};
	uint8_t var1A0[16], var280[0x80], var300[0x40];
	int posCryptKey;
	int i, r5, r14, nanoLenCopy, nanoLen;

	nanoLen = Encr_p[1];
	nanoLenCopy = nanoLen;
	posCryptKey = nanoLen - 6;

	memcpy(var300, Encr_p + posCryptKey, 8);	// copy last 8 bytes (cryptKey)
	xcryptoworks_DesAlgo(var300, cDeskey, odd, 1);
	var300[0] |= 0x80;
	if ((odd & 0x18) < 0x18) var300[0] = (uint8_t)-1;
	if  (odd & 0x8)          var300[1] = (uint8_t)-1;
	for (r14=0; nanoLenCopy>r14; r14+=0x20)
	{
		for (i=0; i<8; i++)
		{
			var1A0[i*2] = (sePermute[var300[i]&0xF] | ((sePermute[var300[i]>>4]) << 4));
			var1A0[(i*2)+1] = var300[i];
		}

		for (i=0; i<4; i++) memcpy(var300+(i*0x10), var1A0, 0x10);
		var300[0x10] =  var300[0] ^ 1;
		var300[0x1F] = (var300[0xF]<<4) | 6;
		var300[0x20] = (var300[0x20] & 0x7F) | 0x40;
		b2Ro(var300,0x20);
		b2Ro(var300+0x20, 0x20);

      rdr_RSA_decrypt(reader, var300, var300, 0x40, &er_RExp, &er_RN85, 1);

		b2Ro(var300, 8);
		r5 = nanoLen-8;
		if (nanoLen>0x20) r5=0x20;
		memcpy(var280+r14, var300+8, r5);
		b2Ro(var280+r14, r5);

		nanoLen-=0x20;
	}

	// nano85_end XORING
	for (i=0; i<(nanoLenCopy); i++) Decr_p[i] = Encr_p[i+2] ^ var280[i];

	return 1;
}
//======================================================================
//======================================================================
//======================================================================
//======================================================================
//======================================================================
//======================================================================
//======================================================================
//======================================================================
/*
   	0x81,0x70,0x81,0x81,0xFF,0x00,0x00,0x7C,
   	0x80,0x60,0x57,0x75,0x75,0x9C,0x14,0xCA,0xCC,0x6B,0xFB,0x71,0x26,0xA1,0x65,0x86,0x78,0xDA,
   			  0x95,0xDC,0x2B,0x72,0xD2,0x83,0xE7,0xF7,0xDB,0x04,0xC6,0xFD,0x63,0xA0,0xE1,0xFE,
   			  0x33,0xD2,0xBA,0x06,0x40,0x2E,0xFF,0x91,0xA0,0xA8,0x78,0x8E,0x3D,0x00,0x8F,0x52,
   			  0x42,0x01,0x68,0xD3,0xF1,0xCF,0x43,0x59,0xDF,0x0B,0xBB,0x16,0x80,0x37,0x17,0x6F,
   			  0xFE,0xDE,0xEB,0x0E,0x29,0x20,0x6E,0xDD,0xBB,0x69,0x30,0xDB,0x5A,0xA6,0x79,0xA0,
   			  0x21,0xE4,0xB2,0x8A,0x1F,0x4C,0xFE,0x33,0xA4,0x01,0x6F,0xE7,0x62,0xB1,0xAD,0x63,
   	0x83,0x01,0xC0,
   	0x84,0x02,0xC9,0x01,
   	0x8C,0x03,0x0D,0xB1,0x32,
    0x8E,0x02,0x20,0xB0,
    0xDF,0x08,0x40,0xA9,0xEA,0x1D,0x9E,0x34,0x11,0x63,

    0x81,0x70,0x6A,0x81,0xFF,0x28,0x00,0x65,
    0x83,0x01,0xC0,
    0x85,0x48,0xFA,0x54,0x73,0xE6,0x0D,0x3A,0x83,0x94,0x28,0x0E,0x8B,0x66,0x69,0xAB,0x85,0x9A,
    		  0x51,0xDD,0x29,0xE4,0xC3,0x2E,0xEA,0x53,0x57,0xB0,0xE2,0xF6,0xBD,0x8B,0xCE,0x7F,
    		  0x69,0x5C,0x31,0xB7,0xE7,0x51,0x5D,0x64,0x84,0x45,0x4C,0x04,0xF1,0xD6,0x0B,0x97,
    		  0xCB,0x9F,0xE2,0x6B,0xC4,0x03,0xC3,0x81,0x7A,0x9F,0x51,0x69,0x1F,0xF8,0x1D,0x6E,
    		  0xBC,0x73,0xBA,0x2A,0xA6,0x53,0x3C,0xFC,
    0x86,0x08,0xB2,0xCE,0xC0,0x2C,0x7A,0xC2,0x58,0x18,
    0x8E,0x02,0x20,0xB0,
    0xDF,0x08,0xEE,0x5D,0xB6, 0xCF,0x38,0x1E,0x1C,0x8B,
*/

#if 0
	#define	___TEST_ECM_CWS___	2
	static uint8_t	TEST_CWS_ECMs[][256] = {
			//	0x0D04
			{
				0x81,0x70,0x99,0x81,0xFF,0x00,0x00,0x94,
				0x80,0x60,
				     0x51,0x61,0x5A,0xDE,0x64,0x17,0x09,0xC9,0x5A,0xF0,0x66,0x8B,0xC0,0xBB,0xEF,0xF2,
				     0x46,0x49,0x1A,0xDC,0x7C,0xA5,0xEA,0x46,0x64,0x59,0x4B,0xE2,0x85,0xED,0x03,0xDF,
				     0xC6,0x05,0x0B,0x14,0x51,0x01,0xEA,0x73,0x37,0xC1,0xE5,0xAF,0x75,0x97,0x48,0x45,
				     0x83,0xAB,0x08,0x37,0xCF,0x69,0x07,0x1A,0x64,0x28,0x8E,0x8C,0x8E,0xD0,0x5E,0xAB,
				     0xCB,0x18,0x2B,0xD5,0x24,0xAE,0xB9,0xF9,0xB4,0xE1,0x09,0xD6,0xF6,0xD0,0x35,0x15,
				     0x56,0x14,0x2A,0x76,0x8B,0x2F,0x39,0x1D,0x34,0xFF,0x2C,0x47,0x4B,0xAD,0xA1,0x6D,
				0x83,0x01,0x04,
				0x84,0x02,0x04,0x01,
				0x8C,0x03,0x08,0x00,0x01,
				0x8E,0x02,0x2A,0x25,
				0x8F,0x01,0x01,
				0x91,0x01,0x01,
				0xDB,0x10,0x19,0xFC,0x6C,0xAC,0x85,0x89,0x7E,0xA5,0x25,0x95,0x98,0x1A,0x06,0xCA,0xDA,0x7D,
				0xDF,0x08,0x5A,0x02,0xD0,0x58,0x2B,0x8B,0x97,0x13,
			//	DCWs:60 6D 5D 2A B9 56 69 78 A3 2C 1B EA FA FF 71 6A
			},
			{
				0x81,0x70,0x99,0x81,0xFF,0x00,0x00,0x94,
				0x80,0x60,0xF9,0x38,0x36,0x6A,0xB0,0xE4,0x91,0x39,0xA3,0x18,0x89,0x2E,0x88,0xAB,0x29,0xFD,
						  0xBA,0xB9,0xFF,0x01,0xBA,0x43,0xAC,0x0A,0x59,0xF8,0x2D,0xAA,0xAD,0x9F,0x07,0x4C,
						  0xA6,0x82,0xB9,0x1D,0xF0,0x85,0x07,0x2E,0xF5,0x6B,0x0F,0xBF,0x8D,0x16,0x08,0x22,
						  0x5C,0x57,0xB4,0x8B,0x51,0xB3,0x91,0x88,0xF2,0x70,0x31,0x7D,0x21,0xA8,0x09,0x42,
						  0x41,0x59,0x6C,0xF2,0x21,0x8A,0x6E,0x40,0xF1,0xEE,0x95,0x53,0xCE,0x47,0xED,0x8F,
						  0x1C,0x8A,0xCB,0x67,0x89,0xA5,0x84,0x20,0x07,0xAC,0x2F,0xDD,0x85,0x22,0xDB,0x80,
				0x83,0x01,0x04,
				0x84,0x02,0x04,0x01,
				0x8C,0x03,0x08,0x00,0x01,
				0x8E,0x02,0x29,0x95,
				0x8F,0x01,0x01,
				0x91,0x01,0x01,
				0xDB,0x10,0x84,0x9C,0xE6,0xE4,0x3C,0xBA,0xF9,0xE2,0x41,0x56,0x7A,0x99,0xC1,0x39,0xA0,0x6D,
				0xDF,0x08,0x07,0x14,0xDC,0x33,0xC0,0xDE,0x45,0xA4,
			// cw: 49 4F 2F C7 8E 96 7E A2 3E 69 C4 6B 56 1A 84 F4
			},
			//	0x0D95
			{
				0x81,0x70,0xB5,0x81,0xFF,0x00,0x00,0xB0,
				0x80,0x9D,0x00,0x29,0x30,0x06,0x80,0x30,0x52,0x90,0x8F,0xDE,0xB2,0x86,0x88,0xD2,0x9C,0x7B,
						  0x04,0xC9,0x79,0x84,0x02,0x46,0x01,0x49,0xA1,0xD6,0x2E,0x3F,0x19,0xFC,0xC8,0x31,
						  0xAA,0x6E,0x0D,0xEC,0x4E,0xA7,0x2B,0xD5,0xF8,0xD8,0xE3,0xBD,0xE4,0xE3,0xE2,0x6B,
						  0x7C,0xFC,0x66,0x03,0x46,0xBD,0x00,0x2A,0x30,0x06,0x80,0x30,0xEA,0x1D,0xFC,0xD7,
						  0x9D,0x12,0xA3,0x73,0x64,0xA9,0x95,0xE2,0x9B,0x3E,0x98,0xF5,0xFF,0x8C,0x8A,0x53,
						  0x44,0x40,0xF8,0x48,0xE1,0x93,0xB7,0xDF,0x84,0xBB,0x6F,0x63,0x8D,0x29,0x4F,0x00,
						  0xC7,0x3C,0x26,0x23,0x91,0x80,0x5A,0xEF,0xD6,0xB0,0x67,0xF7,0xFF,0xFF,0xFF,0xFF,
						  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
						  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
						  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
				0x83,0x01,0x10,
				0x84,0x02,0x00,0x00,
				0xDF,0x08,0x20,0x8C,0x90,0x0E,0xDC,0x67,0xFD,0x30,
			//	cw: 93 89 50 6C C9 11 4F 29 36 2A 49 A9 63 88 DF CA
			},
	};
#endif	// #if 0


#define CTW_ALGO_TYPE  		5
#define CTW_DATA_START 		(CTW_ALGO_TYPE)
#define CTW_NANO_LEN   		7
#define CTW_NANO_START 		8
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
int
XCRYPTOWORKS_Process(struct s_reader *reader, ECM_REQUEST *er, uint8_t *cw)
{
	uint16_t casysid;
	uint8_t  *ECM_p;
	uint8_t 	n85Raws 	[256];
	uint8_t 	n85crpts [256];
	uint8_t 	cwplain 	[0x10];
	uint8_t 	systemkey[8];
	uint8_t 	cDeskey	[22];
	int keyNr = 0;
	int provf = 0, prov85f;
	int n83found = 0, n85found = 0, nDBfound = 0;
	int cwAlgo;
	int r29;
	int i,j;

	if (!er) return 0;
	if (!reader) return 0;
	casysid 	= er->caid;
	ECM_p 	= er->ecm;
	cwAlgo  	= ECM_p[CTW_ALGO_TYPE]; // 0x28 DT
//	casysid 	= 0x0D05;
//	ECM_p 	= TEST_CWS_ECMs[1];
//	myprdump("ECM_p", ECM_p, SCT_LEN(ECM_p));
	for (i=CTW_NANO_START; i<ECM_p[7];)
	{
		MYEMU_TRACE("xcryptoworks:NANO:%02X\n", ECM_p[i]);
		switch (ECM_p[i])
		{
			case 0x83:
				provf = ECM_p[i+2]&0xFC;
				keyNr = ECM_p[i+2]&3;
				n83found = i;
				break;

			case 0x84:
				if (n83found) break;
				provf = ECM_p[i+2]&0xFC;
				keyNr = ECM_p[i+2]&3;
				break;

			case 0x85:
				memset(n85Raws, 0x00, sizeof(n85Raws));
				memcpy(n85Raws, &ECM_p[i], ECM_p[i+1]+2);
				n85found = i;
				MYEMU_TRACE("xcryptoworks:nano85 found\n");
				break;

			case 0xDB:
				memcpy (cwplain,&ECM_p[i+2], 0x10);
				nDBfound = i;
				MYEMU_TRACE("xcryptoworks:nanoDB found\n");
				break;

			default:
				break;
		}
		i = i + ECM_p[i+1] + 2;
	}
	//------------------------------------------------------------------
	//------------------------------------------------------------------
	//------------------------------------------------------------------
	if (n85found)
	{
		if (!xcryptoworks_FindRsakey(reader, casysid, provf)) return 0;
	}
	// select suitable systemkey
	if (!xcryptoworks_FindSystemkey(reader, casysid, provf, systemkey)) return 0;	// prov id not found
	// select suitable provider
	if (!xcryptoworks_FindDeskey(reader, casysid, provf, keyNr, cDeskey)) return 0;
	//------------------------------------------------------------------
	//------------------------------------------------------------------
	//------------------------------------------------------------------
	//------------------------------------------------------------------
	if (n85found)
	{
//		MYEMU_TRACE("xcryptoworks:NANO 85 Provider ID: %04X:%02X,%02X\n", casysid, prov85f, cwAlgo));
		prov85f = provf;
		memcpy(cDeskey+0x10, systemkey, 6);
		if (!xcryptoworks_n85Decrypt(reader,
					n85Raws,
					cDeskey,
					cwAlgo,
					casysid,
					prov85f,
					n85crpts))
		{
			MYEMU_TRACE("xcryptoworks:nano85 decrypt fails.\n");
			return 0;
		}
//		myprdump("n85crpts", n85crpts, n85Raws[1]-8);
		n83found = 0;
		for (i=0; i<n85Raws[1]-8; )
		{
			switch (n85crpts[i])
			{
				case 0x83:
					prov85f = n85crpts[i+2] & 0xFC;
					keyNr   = n85crpts[i+2] & 3;
					n83found= 1;
					break;

				case 0x84:
					if (n83found) break;
					prov85f = n85crpts[i+2] & 0xFC;
					keyNr   = n85crpts[i+2] & 3;
					break;

				case 0x85:
					break;

				case 0xDB:
					memcpy (cwplain, &n85crpts[i+2], 0x10);
					nDBfound = 1;
					break;
				default:
					break;
			}
			i = i + n85crpts[i+1]+2;
		}
		if (provf != prov85f)
		{
			MYEMU_TRACE("xcryptoworks:provf(%02X)!=prov85f(%02X)\n", provf, prov85f);
			if (!xcryptoworks_FindSystemkey(reader, casysid, provf, systemkey)) return 0;	// prov id not found
		}
	}

	if (!nDBfound)
	{
		MYEMU_TRACE("xcryptoworks:nanoDB not found.\n");
		return 0;
	}
	//------------------------------------------------------------------
	//------------------------------------------------------------------
	//------------------------------------------------------------------
	memcpy(cDeskey + 0x10, systemkey, 6);
	//------------------------------------------------------------------
	//------------------------------------------------------------------
	//------------------------------------------------------------------
	// decrypt CW
	for (i=0; i<2; i++)
	{
		r29 = 0;
		if ((ECM_p[0x5]&7)==7)
		{
			for (j=0;j<3;i++)
			{
				r29^=1;
				xcryptoworks_DesAlgo(cwplain+(8*i),cDeskey,cwAlgo,r29|2);
			}
		}
		else
		{
			xcryptoworks_DesAlgo(cwplain+(8*i),cDeskey,cwAlgo,0);//normal DecryptCW
		}
	}

	memcpy(cw, &cwplain[0], 8);
	memcpy(cw+8, &cwplain[8], 8);
	MYEMU_TRACE("xcryptoworks:cw{%02X...%02X,%02X...%02X}\n", cw[0], cw[7], cw[8], cw[15]);
	return 1;
}


void
XCRYPTOWORKS_Cleanup(void)
{
	MYEMU_TRACE("xcryptoworks:clean\n");
	er_KFOUND = 0;
	er_KNR 	 = 0;
	er_KPPID	 = 0x0;
	memset(er_KKEY, 0x0, 16);
	memset(er_KSYSKEY, 0x0, 6);
}
#endif	// defined(__XCAS_CRYPTOWORKS__)
#endif	// defined(MODULE_XCAS)

