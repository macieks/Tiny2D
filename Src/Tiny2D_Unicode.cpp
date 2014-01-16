// -----------------------------------------------------------------------------

/*
 * Copyright 2001-2004 Unicode, Inc.
 * 
 * Disclaimer
 * 
 * This source code is provided as is by Unicode, Inc. No claims are
 * made as to fitness for any particular purpose. No warranties of any
 * kind are expressed or implied. The recipient agrees to determine
 * applicability of information provided. If this file has been
 * purchased on magnetic or optical media from Unicode, Inc., the
 * sole remedy for any claim will be exchange of defective media
 * within 90 days of receipt.
 * 
 * Limitations on Rights to Redistribute This Code
 * 
 * Unicode, Inc. hereby grants the right to freely use the information
 * supplied in this file in the creation of products supporting the
 * Unicode Standard, and to make copies of this file in any form
 * for internal or external distribution as long as this notice
 * remains attached.
 */

namespace Tiny2D
{

/* Some fundamental constants */
#define UNI_REPLACEMENT_CHAR (unsigned int) 0x0000FFFD
#define UNI_MAX_BMP (unsigned int) 0x0000FFFF
#define UNI_MAX_UTF16 (unsigned int) 0x0010FFFF
#define UNI_MAX_UTF32 (unsigned int) 0x7FFFFFFF
#define UNI_MAX_LEGAL_UTF32 (unsigned int) 0x0010FFFF

#define UNI_SUR_HIGH_START  (unsigned int) 0xD800
#define UNI_SUR_HIGH_END    (unsigned int) 0xDBFF
#define UNI_SUR_LOW_START   (unsigned int) 0xDC00
#define UNI_SUR_LOW_END     (unsigned int) 0xDFFF

/*
 * Index into the table below with the first byte of a UTF-8 sequence to
 * get the number of trailing bytes that are supposed to follow it.
 * Note that *legal* UTF-8 values can't have 4 or 5-bytes. The table is
 * left as-is for anyone who may want to do such conversion, which was
 * allowed in earlier algorithms.
 */
static const char trailingBytesForUTF8[256] =
{
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

/*
 * Magic values subtracted from a buffer value during unsigned char conversion.
 * This table contains as many values as there might be trailing bytes
 * in a UTF-8 sequence.
 */
static const unsigned int offsetsFromUTF8[6] =
{ 0x00000000UL, 0x00003080UL, 0x000E2080UL, 0x03C82080UL, 0xFA082080UL, 0x82082080UL };

/*
 * Once the bits are split out into bytes of UTF-8, this is a mask OR-ed
 * into the first byte, depending on how many bytes follow.  There are
 * as many entries in this table as there are UTF-8 sequence types.
 * (I.e., one byte sequence, two byte... etc.). Remember that sequencs
 * for *legal* UTF-8 will be 4 or fewer bytes total.
 */
static const unsigned char firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

/*
 * Utility routine to tell whether a sequence of bytes is legal UTF-8.
 * This must be called with the length pre-determined by the first byte.
 * If not calling this from ConvertUTF8to*, then the length can be set by:
 *  length = trailingBytesForUTF8[*source]+1;
 * and the sequence is illegal right away if there aren't that many bytes
 * available.
 * If presented with a length > 4, this returns false.  The Unicode
 * definition of UTF-8 goes up to 4-byte sequences.
 */

bool IsLegalUTF8Char(const unsigned char* source, unsigned int length)
{
    unsigned char a;
    const unsigned char *srcptr = source + length;
    switch (length)
	{
		default: return false;

		// Everything else falls through when "true"...
		case 4: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
		case 3: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
		case 2: if ((a = (*--srcptr)) > 0xBF) return false;

		switch (*source)
		{
			// no fall-through in this inner switch
			case 0xE0: if (a < 0xA0) return false; break;
			case 0xED: if (a > 0x9F) return false; break;
			case 0xF0: if (a < 0x90) return false; break;
			case 0xF4: if (a > 0x8F) return false; break;
			default:   if (a < 0x80) return false;
		}

		case 1: if (*source >= 0x80 && *source < 0xC2) return false;
    }
    if (*source > 0xF4) return false;
    return true;
}

bool UTF8ToUTF32(const unsigned char* src, unsigned int srcSize, unsigned int* dst, unsigned int& dstSize)
{
	const unsigned int* dstStart = dst;
	const unsigned int* dstEnd = dst + dstSize;
	const unsigned char* srcEnd = src + srcSize;
    while (src < srcEnd)
	{
		unsigned int ch = 0;
		const unsigned int extraBytesToRead = trailingBytesForUTF8[*src];
		if (extraBytesToRead >= srcSize)
			return false;

		// Do this check whether lenient or strict
		if (!IsLegalUTF8Char(src, extraBytesToRead + 1))
			return false;

		// The cases all fall through. See "Note A" below.
		switch (extraBytesToRead)
		{
			case 5: ch += *src++; ch <<= 6;
			case 4: ch += *src++; ch <<= 6;
			case 3: ch += *src++; ch <<= 6;
			case 2: ch += *src++; ch <<= 6;
			case 1: ch += *src++; ch <<= 6;
			case 0: ch += *src++;
		}
		ch -= offsetsFromUTF8[extraBytesToRead];

		if (dst >= dstEnd)
			return false;

		if (ch <= UNI_MAX_LEGAL_UTF32)
		{
			// UTF-16 surrogate values are illegal in UTF-32, and anything
			// over Plane 17 (> 0x10FFFF) is illegal.
			if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_LOW_END)
				*dst++ = UNI_REPLACEMENT_CHAR;
			else
				*dst++ = ch;
		} else // i.e., ch > UNI_MAX_LEGAL_UTF32
		*dst++ = UNI_REPLACEMENT_CHAR;
    }
	dstSize = (unsigned int) (dst - dstStart);
    return true;
}

};