
//==============================================================================
// INCLUDES
//==============================================================================
#include "xstring/x_string.h"
#include "xbase/x_memory.h"
#include "xbase/x_runes.h"
#include "xbase/x_printf.h"

//==============================================================================
// xCore namespace
//==============================================================================
namespace xcore
{
static s32 WriteCodeUnit(uchar32 cdpt, uchar8 *&pDst) noexcept
{
	if (cdpt <= 0x7F)
	{
		*pDst++ = (uchar8)(cdpt);
		return 1;
	}
	else if (cdpt <= 0x7FF)
	{
		*pDst++ = (uchar8)(0xC0 | ((cdpt >> 6) & 0x1F));
		*pDst++ = (uchar8)(0x80 | (cdpt & 0x3F));
		return 2;
	}
	else if (cdpt <= 0xFFFF)
	{
		*pDst++ = (uchar8)(0xE0 | ((cdpt >> 12) & 0x0F));
		*pDst++ = (uchar8)(0x80 | ((cdpt >> 6) & 0x3F));
		*pDst++ = (uchar8)(0x80 | (cdpt & 0x3F));
		return 3;
	}
	else if (cdpt <= 0x10FFFF)
	{
		*pDst++ = (uchar8)(0xF0 | ((cdpt >> 18) & 0x07));
		*pDst++ = (uchar8)(0x80 | ((cdpt >> 12) & 0x3F));
		*pDst++ = (uchar8)(0x80 | ((cdpt >> 6) & 0x3F));
		*pDst++ = (uchar8)(0x80 | (cdpt & 0x3F));
		return 4;
	}
	return 0;
}

static s32 WriteCodeUnit(uchar32 cdpt, uchar16 *&pDst) noexcept
{
	if (cdpt < 0x10000)
	{
		*pDst++ = (uchar16)cdpt;
		return 1;
	}
	else
	{
		*pDst++ = (uchar16)(0xD7C0 + (cdpt >> 10));
		*pDst++ = (uchar16)(0xDC00 + (cdpt & 0x3FF));
		return 2;
	}
}

bool ReadCodePoint(uchar8 const *&pSrc, uchar8 const *pSrcEnd, uchar32 &cdpt) noexcept
{
	static const uchar32 REPLACEMENT_CHAR = '?';

	if (((unsigned char)*str) < 0x80)
	{
		cdpt = (unsigned char)*str++;
	}
	else if (((unsigned char)*str) < 0xC0)
	{
		++str;
		cdpt = REPLACEMENT_CHAR;
		return false;
	}
	else if (((unsigned char)*str) < 0xE0)
	{
		cdpt = (((unsigned char)*str++) & 0x1F) << 6;
		if (((unsigned char)*str) < 0x80 || ((unsigned char)*str) >= 0xC0)
		{
			cdpt = REPLACEMENT_CHAR;
			return false;
		}
		cdpt += (((unsigned char)*str++) & 0x3F);
	}
	else if (((unsigned char)*str) < 0xF0)
	{
		cdpt = (((unsigned char)*str++) & 0x0F) << 12;
		if (((unsigned char)*str) < 0x80 || ((unsigned char)*str) >= 0xC0)
		{
			cdpt = REPLACEMENT_CHAR;
			return false;
		}
		cdpt += (((unsigned char)*str++) & 0x3F) << 6;
		if (((unsigned char)*str) < 0x80 || ((unsigned char)*str) >= 0xC0)
		{
			cdpt = REPLACEMENT_CHAR;
			return false;
		}
		cdpt += (((unsigned char)*str++) & 0x3F);
	}
	else if (((unsigned char)*str) < 0xF8)
	{
		cdpt = (((unsigned char)*str++) & 0x07) << 18;
		if (((unsigned char)*str) < 0x80 || ((unsigned char)*str) >= 0xC0)
		{
			cdpt = REPLACEMENT_CHAR;
			return false;
		}
		cdpt += (((unsigned char)*str++) & 0x3F) << 12;
		if (((unsigned char)*str) < 0x80 || ((unsigned char)*str) >= 0xC0)
		{
			cdpt = REPLACEMENT_CHAR;
			return false;
		}
		cdpt += (((unsigned char)*str++) & 0x3F) << 6;
		if (((unsigned char)*str) < 0x80 || ((unsigned char)*str) >= 0xC0)
		{
			cdpt = REPLACEMENT_CHAR;
			return false;
		}
		cdpt += (((unsigned char)*str++) & 0x3F);
	}
	else
	{
		++str;
		cdpt = REPLACEMENT_CHAR;
		return false;
	}
	return true;
}

bool ReadCodePoint(uchar16 const *&pSrc, uchar16 const *pSrcEnd, uchar32 &cdpt) noexcept
{
	static const uchar32 REPLACEMENT_CHAR = '?';

	if (*pSrc < 0xD800 || *pSrc >= 0xE000)
	{
		cdpt = *pSrc++;
		return true;
	}
	if (*pSrc >= 0xDC00)
	{
		++pSrc;
		cdpt = REPLACEMENT_CHAR;
		return true;
	}
	else if ((pSrc + 1) < pSrcEnd)
	{
		uchar32 const res = 0x10000 + ((*pSrc++ - 0xD800) << 10);
		if (*pSrc >= 0xDC00 && *pSrc < 0xE000)
		{
			cdpt = res + (*pSrc++ - 0xDC00);
			return true;
		}
		cdpt = REPLACEMENT_CHAR;
	}
	return false;
}

static s32 ascii_nr_chars(uchar const *src, uchar const *&src_end)
{
	s32 c = 0;
	if (src_end == nullptr)
	{
		while (*src != '\0')
		{
			src++;
			c++;
		}
		src_end = src;
	}
	else
	{
		c = src_end - src;
	}
	return c;
}

static void ascii_to_utf32(uchar const *src, uchar const *src_end, uchar32 *dst, uchar32 const *dst_end)
{
	while (src < src_end && dst < dst_end)
	{
		uchar32 c = *src++;
		*dst++ = c;
	}
	return;
}

static s32 utf8_nr_chars(uchar8 const *src, uchar8 const *src_end)
{
	s32 len = 0;
	while (src < src_end)
	{
		uchar32 c;
		if (!ReadCodePoint(src, src_end, c))
		{
			return -len;
		}
		++len;
	}
	return len;
}

static void utf8_to_utf32(uchar8 const *src, uchar8 const *src_end, uchar32 *dst, uchar32 const *dst_end)
{
	while (src < src_end && dst < dst_end)
	{
		uchar32 c;
		if (!ReadCodePoint(src, src_end, c))
		{
			return;
		}
		*dst++ = c;
	}
	return;
}

static s32 utf16_nr_chars(uchar16 const *src, uchar16 const *src_end)
{
	s32 len = 0;
	while (src < src_end)
	{
		uchar32 c;
		if (!ReadCodePoint(src, src_end, c))
		{
			return -len;
		}
		++len;
	}
	return len;
}

static void utf16_to_utf32(uchar16 const *src, uchar16 const *src_end, uchar32 *dst, uchar32 const *dst_end)
{
	while (src < src_end && dst < dst_end)
	{
		uchar32 c;
		if (!ReadCodePoint(src, src_end, c))
		{
			return;
		}
		*dst++ = c;
	}
	return;
}

xstring::xstring(void)
	: m_slice()
{
}

xstring::xstring(const char *str)
	: m_slice()
{
	const char *end = nullptr;
	s32 const len = ascii_nr_chars(str, end);
	m_slice = s_slice.construct(len, sizeof(uchar32));
	ascii_to_utf32(str, end, (uchar32 *)m_slice.begin(), (uchar32 const *)m_slice.end());
}

xstring::xstring(xalloc *_allocator, s32 _len)
	: m_slice(_allocator, _len, sizeof(uchar32))
{
}

xstring::xstring(const xstring &other)
	: m_slice(other.m_slice.obtain())
{
}

xstring::xstring(const xstring &left, const xstring &right)
{
	s32 const len = left.size() + right.size();
	m_slice = left.m_slice.construct(len, sizeof(uchar32));
	void *ldst = m_slice.get_at(0);
	void const *lsrc = left.m_slice.get_at(0);
	xmemcpy(ldst, lsrc, left.size());
	void *rdst = m_slice.get_at(left.size());
	void const *rsrc = right.m_slice.get_at(0);
	xmemcpy(rdst, rsrc, right.size());
}

//------------------------------------------------------------------------------

bool xstring::is_empty() const
{
	return m_slice.size() == 0;
}

s32 xstring::size() const
{
	return m_slice.size();
}

xstring xstring::operator()(s32 to) const
{
	xstring str;
	str.m_slice = m_slice.view(0, to);
	return str;
}

xstring xstring::operator()(s32 from, s32 to) const
{
	xstring str;
	str.m_slice = m_slice.view(from, to);
	return str;
}

uchar32 xstring::operator[](s32 index)
{
	uchar32 *str = (uchar32 *)m_slice.get_at(index);
	if (str == nullptr)
		return '\0';
	return *str;
}

uchar32 xstring::operator[](s32 index) const
{
	uchar32 const *str = (uchar32 const *)m_slice.get_at(index);
	if (str == nullptr)
		return '\0';
	return *str;
}

//------------------------------------------------------------------------------
xstring selectUntil(const xstring &str, const xstring &find)
{
	s32 src_pos = 0;
	uchar32 const *src = (uchar32 const *)str.m_slice.begin();
	uchar32 const *end = (uchar32 const *)str.m_slice.end();
	while (src < end)
	{
		uchar32 const *ssrc = src;
		uchar32 const *fsrc = (uchar32 const *)find.m_slice.begin();
		uchar32 const *fdst = (uchar32 const *)find.m_slice.end();
		bool found_match = true;
		while (ssrc < end && fsrc < fend && *ssrc == *fsrc)
		{
			++ssrc;
			++fsrc;
		}
		if (fsrc == fend)
		{
			return str.m_slice.view(0, src_pos);
		}
		++src_pos;
		++src;
	}
	return xstring();
}

xstring selectUntilIncluded(const xstring &inStr, const xstring &inFind)
{
	s32 src_pos = 0;
	uchar32 const *src = (uchar32 const *)str.m_slice.begin();
	uchar32 const *end = (uchar32 const *)str.m_slice.end();
	while (src < end)
	{
		uchar32 const *ssrc = src;
		uchar32 const *fsrc = (uchar32 const *)find.m_slice.begin();
		uchar32 const *fdst = (uchar32 const *)find.m_slice.end();
		bool found_match = true;
		while (ssrc < end && fsrc < fend && *ssrc == *fsrc)
		{
			++ssrc;
			++fsrc;
		}
		if (fsrc == fend)
		{
			return str.m_slice.view(0, src_pos + find.m_slice.size());
		}
		++src_pos;
		++src;
	}
	return xstring();
}

bool narrowIfDelimited(xstring &str, uchar32 leftdel, uchar32 rightdel)
{
	uchar32 const *src = (uchar32 const *)str.m_slice.begin();
	uchar32 const *end = (uchar32 const *)str.m_slice.end() - 1;
	if (src < end && *src == lefdel)
	{
		if (*dst == rightdel)
		{
			str = str(1, str.size() - 2);
			return true;
		}
	}
	return false;
}

bool isUpper(const xstring &str)
{
	uchar32 const *src = (uchar32 const *)str.m_slice.begin();
	uchar32 const *dst = (uchar32 const *)str.m_slice.end();
	while (src < dst)
	{
		if (is_lower(*src))
		{
			return false;
		}
		++src;
	}
	return true;
}

bool isLower(const xstring &str)
{
	uchar32 const *src = (uchar32 const *)str.m_slice.begin();
	uchar32 const *dst = (uchar32 const *)str.m_slice.end();
	while (src < dst)
	{
		if (is_upper(*src))
		{
			return false;
		}
		++src;
	}
	return true;
}

bool isCapitalized(const xstring &str)
{
	uchar32 const *src = (uchar32 const *)str.m_slice.begin();
	uchar32 const *dst = (uchar32 const *)str.m_slice.end();
	if (src < dst)
	{
		if (is_upper(*src))
		{
			++src;
			while (src < dst && !is_space(*src))
			{
				if (is_lower(*src))
				{
					return false;
				}
				++src;
			}
			return true;
		}
	}
	return false;
}

bool isQuoted(const xstring &str)
{
	uchar32 const *src = (uchar32 const *)str.m_slice.begin();
	uchar32 const *dst = (uchar32 const *)str.m_slice.end() - 1;
	if (src < dst)
	{
		uchar32 lefdel = *src;
		if (*dst == rightdel)
		{
			return true;
		}
	}
	return false;
}

bool isQuoted(const xstring &str, rune inQuote)
{
	return isDelimited(str, inQuote, inQuote);
}

bool isDelimited(const xstring &str, rune inLeft, rune inRight)
{
	uchar32 const *src = (uchar32 const *)str.m_slice.begin();
	uchar32 const *dst = (uchar32 const *)str.m_slice.end() - 1;
	if (src < dst)
	{
		if (*src == inLeft && *dst == inRight)
		{
			return true;
		}
	}
	return false;
}

rune firstChar(const xstring &str)
{
	uchar32 const *src = (uchar32 const *)str.m_slice.begin();
	uchar32 const *dst = (uchar32 const *)str.m_slice.end() - 1;
	if (src < dst)
	{
		return *src;
	}
	return '\0';
}

rune lastChar(const xstring &str)
{
	uchar32 const *src = (uchar32 const *)str.m_slice.begin();
	uchar32 const *dst = (uchar32 const *)str.m_slice.end() - 1;
	if (src < dst)
	{
		return *dst;
	}
	return '\0';
}

bool startsWith(const xstring &str, xstring const &inStartStr)
{
	uchar32 const *src = (uchar32 const *)str.m_slice.begin();
	uchar32 const *end = (uchar32 const *)str.m_slice.end();
	if (src < end)
	{
		uchar32 const *fsrc = (uchar32 const *)inStartStr.m_slice.begin();
		uchar32 const *fdst = (uchar32 const *)inStartStr.m_slice.end();
		while (ssrc < end && fsrc < fend && *ssrc == *fsrc)
		{
			++ssrc;
			++fsrc;
		}
		if (fsrc == fend)
		{
			return true;
		}
	}
	return false;
}

bool endsWith(const xstring &str, xstring const &inEndStr)
{
	uchar32 const *beg = (uchar32 const *)str.m_slice.begin();
	uchar32 const *src = (uchar32 const *)str.m_slice.end() - inEndStr.size();
	uchar32 const *end = (uchar32 const *)str.m_slice.end();
	if (src >= beg && src < end)
	{
		uchar32 const *fsrc = (uchar32 const *)inEndStr.m_slice.begin();
		uchar32 const *fdst = (uchar32 const *)inEndStr.m_slice.end();
		while (src < end && fsrc < fend && *src == *fsrc)
		{
			++src;
			++fsrc;
		}
		if (fsrc == fend)
		{
			return true;
		}
	}
	return false;
}

xstring find(const xstring &str, rune find)
{
	s32 pos = 0;
	uchar32 const *src = (uchar32 const *)str.m_slice.begin();
	uchar32 const *end = (uchar32 const *)str.m_slice.end();
	while (src < end)
	{
		if (*src == find)
		{
			return str.m_slice.view(pos, pos + 1);
		}
		++pos;
		++src;
	}
	return xstring();
}

xstring find(const xstring &str, const xstring &find)
{
	s32 pos = 0;
	uchar32 const *src = (uchar32 const *)str.m_slice.begin();
	uchar32 const *end = (uchar32 const *)str.m_slice.end();
	while (src < end)
	{
		uchar32 const *ssrc = src;
		uchar32 const *fsrc = (uchar32 const *)find.m_slice.begin();
		uchar32 const *fdst = (uchar32 const *)find.m_slice.end();
		while (ssrc < end && fsrc < fend && *ssrc == *fsrc)
		{
			++ssrc;
			++fsrc;
		}
		if (fsrc == fend)
		{
			return str.m_slice.view(pos, pos + find.size());
		}
		++pos;
		++src;
	}
	return xstring();
}

xstring rfind(const xstring &str, const xstring &find)
{
	s32 pos = str.size() - 1;
	uchar32 const *src = (uchar32 const *)str.m_slice.begin();
	uchar32 const *iter = (uchar32 const *)str.m_slice.end() - 1;
	uchar32 const *end = (uchar32 const *)str.m_slice.end();
	while (iter >= src)
	{
		uchar32 const *ssrc = iter;
		uchar32 const *fsrc = (uchar32 const *)find.m_slice.begin();
		uchar32 const *fdst = (uchar32 const *)find.m_slice.end();
		while (ssrc < end && fsrc < fend && *ssrc == *fsrc)
		{
			++ssrc;
			++fsrc;
		}
		if (fsrc == fend)
		{
			return str.m_slice.view(pos, pos + find.size());
		}
		--pos;
		--iter;
	}
	return xstring();
}

xstring findOneOf(const xstring &str, const xstring &find)
{
	s32 pos = 0;
	uchar32 const *src = (uchar32 const *)str.m_slice.begin();
	uchar32 const *end = (uchar32 const *)str.m_slice.end();
	while (src < end)
	{
		uchar32 const *fsrc = (uchar32 const *)find.m_slice.begin();
		uchar32 const *fdst = (uchar32 const *)find.m_slice.end();
		while (fsrc < fend)
		{
			if (*src == *fsrc)
			{
				return str.m_slice.view(pos, pos + 1);
			}
			++fsrc;
		}
		++pos;
		++src;
	}
	return xstring();
}

xstring rfindOneOf(const xstring &str, const xstring &find)
{
	s32 pos = str.size() - 1;
	uchar32 const *src = (uchar32 const *)str.m_slice.begin();
	uchar32 const *iter = (uchar32 const *)str.m_slice.end() - 1;
	uchar32 const *end = (uchar32 const *)str.m_slice.end();
	while (iter >= src)
	{
		uchar32 const *fsrc = (uchar32 const *)find.m_slice.begin();
		uchar32 const *fdst = (uchar32 const *)find.m_slice.end();
		while (fsrc < fend)
		{
			if (*iter == *fsrc)
			{
				return str.m_slice.view(pos, pos + 1);
			}
			++fsrc;
		}
		--pos;
		--iter;
	}
	return xstring();
}

s32 compare(const xstring &lhs, const xstring &rhs)
{
	if (lhs.size() < rhs.size())
		return -1;
	if (lhs.size() > rhs.size())
		return 1;

	uchar32 const *lsrc = (uchar32 const *)lhs.m_slice.begin();
	uchar32 const *lend = (uchar32 const *)lhs.m_slice.end();
	uchar32 const *rsrc = (uchar32 const *)rhs.m_slice.begin();
	uchar32 const *rend = (uchar32 const *)rhs.m_slice.end();
	while (lsrc < lend && rsrc < rend)
	{
		if (*src < *dst)
		{
			return -1;
		}
		else if (*src > *dst)
		{
			return 1;
		}
		++lsrc;
		++rsrc;
	}
	return 0;
}

bool isEqual(const xstring &lhs, const xstring &rhs)
{
	return compare(lhs, rhs) == 0;
}

bool contains(const xstring &str, const xstring &contains)
{
	uchar32 const *src = (uchar32 const *)str.m_slice.begin();
	uchar32 const *end = (uchar32 const *)str.m_slice.end();
	while (src < end)
	{
		uchar32 const *ssrc = src;
		uchar32 const *fsrc = (uchar32 const *)contains.m_slice.begin();
		uchar32 const *fdst = (uchar32 const *)contains.m_slice.end();
		while (ssrc < end && fsrc < fend && *ssrc == *fsrc)
		{
			++ssrc;
			++fsrc;
		}
		if (fsrc == fend)
		{
			return true;
		}
		++src;
	}
	return false;
}

bool contains(const xstring &str, rune contains)
{
	uchar32 const *src = (uchar32 const *)str.m_slice.begin();
	uchar32 const *end = (uchar32 const *)str.m_slice.end();
	while (src < end)
	{
		if (*src == contains)
		{
			return true;
		}
		++src;
	}
	return false;
}

void repeat(xstring &str, xstring const &inStr, s32 inTimes)
{
	s32 const current_len = size();
	s32 const added_len = inStr.size() * inTimes;
	str.m_slice.resize(size() + inTimes * inStr.size());
	for (s32 i = 0; i < inTimes; ++i)
	{
		concatenate(str, inStr);
	}
}

s32 format(xstring &str, xstring const &format, const x_va_list &args)
{
	utf32::runes dst(str.m_slice.begin(), str.m_slice.begin(), str.m_slice.end());
	utf32::crunes fmt(format.m_slice.begin(), format.m_slice.end());
	utf32::vsprintf(dst, fmt, args);
	return 0;
}

s32 formatAdd(xstring &str, xstring const &format, const x_va_list &args)
{
	utf32::runes dst(str.m_slice.begin(), str.m_slice.end(), str.m_slice.eos());
	utf32::crunes fmt(format.m_slice.begin(), format.m_slice.end());
	utf32::vsprintf(dst, fmt, args);
	return 0;
}

void replace(xstring &inStr, xstring const &inReplace)
{
	s32 current_len = inStr.size();
	s32 replace_len = inReplace.size();
	if (current_len < replace_len)
	{
		// Grow the place
	}
	else if (current_len > replace_len)
	{
		// Shrink the place
	}
	else
	{
		// The current and replacement string are of the same length
	}
}

s32 replaceAnyWith(xstring &str, xstring const &any, rune inWith)
{
	s32 c = 0;
	uchar32 *dst = (uchar32 *)str.m_slice.begin();
	uchar32 const *end = (uchar32 const *)str.m_slice.end();
	while (dst < end)
	{
		uchar32 const *asrc = (uchar32 *)any.m_slice.begin();
		uchar32 const *aend = (uchar32 const *)any.m_slice.end();
		while (asrc < aend)
		{
			if (*dst == *asrc)
			{
				++c;
				*dst = inWith;
				break;
			}
		}
		++dst;
	}
	return c;
}

// Growing and Shrinking
//
// Things to consider:
// - We are the only reference to this string, we can resize without problems
// - We are not the only reference to this string, resizing means that others
//   will end up with wrong <from,to> selections
// - Also the reference we hold can point to not only a small but also a very
//   large backed buffer, this would mean that a resize would have a big
//   performance hit.
//
// However we still leave all of the above to the 'user' since they should be
// aware of how this string is working.

void insert(xstring &str, xstring const &insert)
{
	str.m_slice.insert(insert.size());
	xmemcpy(str.m_slice.begin(), insert.m_slice.begin(), insert.size());
}

void remove(xstring &remove)
{
	str.m_slice.remove(insert.size());
}

void find_remove(xstring &str, const xstring &remove)
{
	xstring found = str.find(remove);
	if (found.is_empty())
		return;
	s32 const strlen = str.size();
	found.m_slice.remove(remove.size());
	str = str(0, strlen - remove.size());
}

void remove_charset(xstring &str, const xstring &charset)
{
	// Remove any of the characters in @charset from @str
}

void upper(xstring &str)
{
	uchar32 *dst = (uchar32 *)str.m_slice.begin();
	uchar32 const *end = (uchar32 const *)str.m_slice.end();
	while (dst < end)
	{
		*dst = utf32::to_upper(*dst);
		++dst;
	}
}

void lower(xstring &str)
{
	uchar32 *dst = (uchar32 *)str.m_slice.begin();
	uchar32 const *end = (uchar32 const *)str.m_slice.end();
	while (dst < end)
	{
		*dst = utf32::to_lower(*dst);
		++dst;
	}
}

void capitalize(xstring &str)
{
	// Standard separator is ' '
	uchar32 *dst = (uchar32 *)str.m_slice.begin();
	uchar32 const *end = (uchar32 const *)str.m_slice.end();
	bool prev_is_space = true;
	while (dst < end)
	{
		uchar32 c = *dst;
		if (is_alpha(c))
		{
			if (prev_is_space)
			{
				c = to_upper(c);
			}
			else
			{
				c = to_lower(c);
			}
		}
		else
		{
			prev_is_space = is_space(c);
		}
		*dst++ = c;
	}
}

void capitalize(xstring &str, xstring const &separators)
{
	uchar32 *dst = (uchar32 *)str.m_slice.begin();
	uchar32 const *end = (uchar32 const *)str.m_slice.end();
	bool prev_is_space = false;
	while (dst < end)
	{
		uchar32 c = *dst;
		if (is_alpha(c))
		{
			if (prev_is_space)
			{
				c = to_upper(c);
			}
			else
			{
				c = to_lower(c);
			}
		}
		else
		{
			prev_is_space = false;
			uchar32 *sepsrc = (uchar32 *)separators.m_slice.begin();
			uchar32 const *sepend = (uchar32 const *)separators.m_slice.end();
			while (sepsrc < sepend)
			{
				if (*sepsrc == c)
				{
					prev_is_space = true;
					break;
				}
			}
		}
		*dst++ = c;
	}
}

// Trim does nothing more than narrowing the <from, to>, nothing is actually removed
// from the actual underlying string data.
void trim(xstring &str)
{
	trimLeft(str);
	trimRight(str);
}

void trimLeft(xstring &str)
{
	s32 from = 0;
	s32 const to = str.size();
	uchar32 const *src = (uchar32 const *)str.m_slice.begin();
	uchar32 const *end = (uchar32 const *)str.m_slice.end();
	while (src < end && (*src == ' ' || *src == '\t' || *src == '\r' || *src == '\n'))
	{
		++src;
		++from;
	}
	str = str(from, to);
}

void trimRight(xstring &str)
{
	s32 const from = 0;
	s32 to = str.size();
	uchar32 const *end = (uchar32 const *)str.m_slice.begin();
	uchar32 const *src = (uchar32 const *)str.m_slice.end() - 1;
	while (src > end && (*src == ' ' || *src == '\t' || *src == '\r' || *src == '\n'))
	{
		--src;
		--to;
	}
	str = str(from, to);
}

void trim(xstring &str, rune r)
{
	trimLeft(str, r);
	trimRight(str, r);
}

void trimLeft(xstring &str, rune r)
{
	s32 from = 0;
	s32 const to = str.size();
	uchar32 const *src = (uchar32 const *)str.m_slice.begin();
	uchar32 const *end = (uchar32 const *)str.m_slice.end();
	while (src < end && (*src == r))
	{
		++src;
		++from;
	}
	str = str(from, to);
}

void trimRight(xstring &str, rune r)
{
	s32 const from = 0;
	s32 to = str.size();
	uchar32 const *end = (uchar32 const *)str.m_slice.begin();
	uchar32 const *src = (uchar32 const *)str.m_slice.end() - 1;
	while (src > end && (*src == r))
	{
		--src;
		--to;
	}
	str = str(from, to);
}

void trim(xstring &str, xstring const &set)
{
	trimLeft(str, set);
	trimRight(str, set);
}

void trimLeft(xstring &str, xstring const &set)
{
	s32 from = 0;
	s32 const to = str.size();
	uchar32 const *src = (uchar32 const *)str.m_slice.begin();
	uchar32 const *end = (uchar32 const *)str.m_slice.end();
	while (src < end)
	{
		if (!contains(set, *src))
		{
			break;
		}
		++src;
		++from;
	}
	str = str(from, to);
}

void trimRight(xstring &str, xstring const &set)
{
	s32 const from = 0;
	s32 to = str.size();
	uchar32 const *end = (uchar32 const *)str.m_slice.begin();
	uchar32 const *src = (uchar32 const *)str.m_slice.end() - 1;
	while (src > end)
	{
		if (!contains(set, *src))
		{
			break;
		}
		--src;
		--to;
	}
	str = str(from, to);
}

void trimQuotes(xstring &str)
{
	trimDelimiters(str, '"', '"');
}

void trimQuotes(xstring &str, rune quote)
{
	trimDelimiters(str, quote, quote);
}

void trimDelimiters(xstring &str, rune left, rune right)
{
	trimLeft(str, left);
	trimRight(str, right);
}

void reverse(xstring &str)
{
	uchar32 *src = (uchar32 *)str.m_slice.begin();
	uchar32 *end = (uchar32 *)str.m_slice.end() - 1;
	while (src <= end)
	{
		uchar32 c = *src;
		*src = *end;
		*end = c;
		--end;
		++src;
	}
}

bool splitOn(xstring &str, rune inChar, xstring &outLeft, xstring &outRight)
{
	outLeft = find(str, inChar);
	if (outLeft.is_empty())
		return false;
	outRight = str(outLeft.size(), str.size());
	trimRight(outLeft, inChar);
	return true;
}

bool splitOn(xstring &str, xstring &inStr, xstring &outLeft, xstring &outRight)
{
	outLeft = selectUntil(str, inChar);
	if (outLeft.is_empty())
		return false;
	outRight = str(outLeft.size() + inStr.size(), str.size());
	return true;
}

xstring copy(const xstring &str)
{
	return xstring(str);
}

void concatenate(xstring &left, const xstring &right)
{
	s32 const llen = left.size();
	s32 const rlen = right.size();
	left.m_slice.resize(llen + rlen);
	void *rdst = left.m_slice.get_at(llen);
	void const *rsrc = right.m_slice.begin();
	xmemcpy(rdst, rsrc, rlen);
}
} // namespace xcore

//------------------------------------------------------------------------------
static void user_case_for_string()
{
	xstring str;
	xstring substr = str(0, 10);
}
}
; // namespace xcore
