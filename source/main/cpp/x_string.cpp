
//==============================================================================
// INCLUDES
//==============================================================================
#include "xstring\x_string.h"
#include "xbase\x_memory.h"
#include "xbase\x_runes.h"

//==============================================================================
// xCore namespace
//==============================================================================
namespace xcore
{
	static s32 ascii_nr_chars(uchar const* src, uchar const* src_end)
	{
		return 0;
	}

	static void ascii_to_utf32(uchar const* src, uchar const* src_end, uchar32* dst, uchar32 const* dst_end)
	{

	}

	static s32 utf8_nr_chars(uchar8 const* src, uchar8 const* src_end)
	{
		return 0;
	}

	static void utf8_to_utf32(uchar8 const* src, uchar8 const* src_end, uchar32* dst, uchar32 const* dst_end)
	{

	}

	static s32 utf16_nr_chars(uchar16 const* src, uchar16 const* src_end)
	{
		return 0;
	}

	static void utf16_to_utf32(uchar16 const* src, uchar16 const* src_end, uchar32* dst, uchar32 const* dst_end)
	{

	}


	xstring::xstring(void)
		: m_slice()
	{
	}

	xstring::xstring(const char* str)
		: m_slice()
	{
		s32 const len = ascii_nr_chars(str, nullptr);
		m_slice = s_slice.construct(len, sizeof(uchar32));
	}

	xstring::xstring(xalloc* _allocator, s32 _len)
		: m_slice(_allocator, _len, sizeof(uchar32))
	{
	}

	xstring::xstring(const xstring& other)
		: m_slice(other.m_slice.obtain())
	{
	}


	//------------------------------------------------------------------------------

	xstring::xstring(const xstring& left, const xstring& right)
	{
		s32 const len = left.size() + right.size();
		m_slice = left.m_slice.construct(len, sizeof(uchar32));
		void* ldst = m_slice.get_at(0);
		void const* lsrc = left.m_slice.get_at(0);
		xmemcpy(ldst, lsrc, left.size());
		void* rdst = m_slice.get_at(left.size());
		void const* rsrc = right.m_slice.get_at(0);
		xmemcpy(rdst, rsrc, right.size());
	}


	bool			xstring::is_empty() const
	{
		return m_slice.size() == 0; 
	}

	s32				xstring::size() const
	{
		return m_slice.size(); 
	}


	xstring			xstring::operator () (s32 to) const
	{
		xstring str;
		str.m_slice = m_slice.view(0, to);
		return str;
	}

	xstring			xstring::operator () (s32 from, s32 to) const
	{
		xstring str;
		str.m_slice = m_slice.view(from, to);
		return str;
	}


	uchar32 		xstring::operator [] (s32 index)
	{
		uchar32* str = (uchar32 *)m_slice.get_at(index);
		if (str == nullptr)
			return '\0';
		return *str;
	}

	uchar32 		xstring::operator [] (s32 index) const
	{
		uchar32 const* str = (uchar32 const*)m_slice.get_at(index);
		if (str == nullptr)
			return '\0';
		return *str;
	}




	//------------------------------------------------------------------------------
	xstring			xstring::selectUntil(const xstring& str, const xstring& find)
	{
		s32 src_pos = 0;
		uchar32 const* src = (uchar32 const*)str.m_slice.begin();
		uchar32 const* end = (uchar32 const*)str.m_slice.end();
		while (src < end)
		{
			uchar32 const* ssrc = src;
			uchar32 const* fsrc = (uchar32 const*)find.m_slice.begin();
			uchar32 const* fdst = (uchar32 const*)find.m_slice.end();
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

	xstring			xstring::selectUntilIncluded(const xstring& inStr, const xstring& inFind)
	{
		s32 src_pos = 0;
		uchar32 const* src = (uchar32 const*)str.m_slice.begin();
		uchar32 const* end = (uchar32 const*)str.m_slice.end();
		while (src < end)
		{
			uchar32 const* ssrc = src;
			uchar32 const* fsrc = (uchar32 const*)find.m_slice.begin();
			uchar32 const* fdst = (uchar32 const*)find.m_slice.end();
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

	bool			xstring::narrowIfDelimited(xstring& str, uchar32 leftdel, uchar32 rightdel)
	{
		uchar32 const* src = (uchar32 const*)str.m_slice.begin();
		uchar32 const* end = (uchar32 const*)str.m_slice.end() - 1;
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


	bool			xstring::isUpper(const xstring& str)
	{
		uchar32 const* src = (uchar32 const*)str.m_slice.begin();
		uchar32 const* dst = (uchar32 const*)str.m_slice.end();
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

	bool			xstring::isLower(const xstring& str)
	{
		uchar32 const* src = (uchar32 const*)str.m_slice.begin();
		uchar32 const* dst = (uchar32 const*)str.m_slice.end();
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

	bool			xstring::isCapitalized(const xstring& str)
	{
		uchar32 const* src = (uchar32 const*)str.m_slice.begin();
		uchar32 const* dst = (uchar32 const*)str.m_slice.end();
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

	bool			xstring::isQuoted(const xstring& str)
	{
		uchar32 const* src = (uchar32 const*)str.m_slice.begin();
		uchar32 const* dst = (uchar32 const*)str.m_slice.end() - 1;
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

	bool			xstring::isQuoted(const xstring& str, rune inQuote)
	{
		return isDelimited(str, inQuote, inQuote);
	}

	bool			xstring::isDelimited(const xstring& str, rune inLeft, rune inRight)
	{
		uchar32 const* src = (uchar32 const*)str.m_slice.begin();
		uchar32 const* dst = (uchar32 const*)str.m_slice.end() - 1;
		if (src < dst)
		{
			if (*src == inLeft && *dst == inRight)
			{
				return true;
			}
		}
		return false;
	}


	rune			xstring::firstChar(const xstring& str)
	{
		uchar32 const* src = (uchar32 const*)str.m_slice.begin();
		uchar32 const* dst = (uchar32 const*)str.m_slice.end() - 1;
		if (src < dst)
		{
			return *src;
		}
		return '\0';
	}

	rune			xstring::lastChar(const xstring& str)
	{
		uchar32 const* src = (uchar32 const*)str.m_slice.begin();
		uchar32 const* dst = (uchar32 const*)str.m_slice.end() - 1;
		if (src < dst)
		{
			return *dst;
		}
		return '\0';
	}


	bool			xstring::startsWith(const xstring& str, xstring const& inStartStr)
	{
		uchar32 const* src = (uchar32 const*)str.m_slice.begin();
		uchar32 const* end = (uchar32 const*)str.m_slice.end();
		if (src < end)
		{
			uchar32 const* fsrc = (uchar32 const*)inStartStr.m_slice.begin();
			uchar32 const* fdst = (uchar32 const*)inStartStr.m_slice.end();
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

	bool			xstring::endsWith(const xstring& str, xstring const& inEndStr)
	{
		uchar32 const* beg = (uchar32 const*)str.m_slice.begin();
		uchar32 const* src = (uchar32 const*)str.m_slice.end() - inEndStr.size();
		uchar32 const* end = (uchar32 const*)str.m_slice.end();
		if (src >= beg && src < end)
		{
			uchar32 const* fsrc = (uchar32 const*)inEndStr.m_slice.begin();
			uchar32 const* fdst = (uchar32 const*)inEndStr.m_slice.end();
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

	xstring			xstring::find(const xstring& str, const xstring& find)
	{
		s32 pos = 0;
		uchar32 const* src = (uchar32 const*)str.m_slice.begin();
		uchar32 const* end = (uchar32 const*)str.m_slice.end();
		while (src < end)
		{
			uchar32 const* ssrc = src;
			uchar32 const* fsrc = (uchar32 const*)find.m_slice.begin();
			uchar32 const* fdst = (uchar32 const*)find.m_slice.end();
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

	xstring			xstring::rfind(const xstring& str, const xstring& find)
	{
		s32 pos = str.size() - 1;
		uchar32 const* src  = (uchar32 const*)str.m_slice.begin();
		uchar32 const* iter = (uchar32 const*)str.m_slice.end() - 1;
		uchar32 const* end  = (uchar32 const*)str.m_slice.end();
		while (iter >= src)
		{
			uchar32 const* ssrc = iter;
			uchar32 const* fsrc = (uchar32 const*)find.m_slice.begin();
			uchar32 const* fdst = (uchar32 const*)find.m_slice.end();
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

	xstring			xstring::findOneOf(const xstring& str, const xstring& find)
	{
		s32 pos = 0;
		uchar32 const* src = (uchar32 const*)str.m_slice.begin();
		uchar32 const* end = (uchar32 const*)str.m_slice.end();
		while (src < end)
		{
			uchar32 const* fsrc = (uchar32 const*)find.m_slice.begin();
			uchar32 const* fdst = (uchar32 const*)find.m_slice.end();
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

	xstring			xstring::rfindOneOf(const xstring& str, const xstring& find)
	{
		s32 pos = str.size() - 1;
		uchar32 const* src  = (uchar32 const*)str.m_slice.begin();
		uchar32 const* iter = (uchar32 const*)str.m_slice.end() - 1;
		uchar32 const* end  = (uchar32 const*)str.m_slice.end();
		while (iter >= src)
		{
			uchar32 const* fsrc = (uchar32 const*)find.m_slice.begin();
			uchar32 const* fdst = (uchar32 const*)find.m_slice.end();
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

	s32				xstring::compare(const xstring& lhs, const xstring& rhs)
	{
		if (lhs.size() < rhs.size())
			return -1;
		if (lhs.size() > rhs.size())
			return 1;

		uchar32 const* lsrc = (uchar32 const*)lhs.m_slice.begin();
		uchar32 const* lend = (uchar32 const*)lhs.m_slice.end();
		uchar32 const* rsrc = (uchar32 const*)rhs.m_slice.begin();
		uchar32 const* rend = (uchar32 const*)rhs.m_slice.end();
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

	bool			xstring::isEqual(const xstring& lhs, const xstring& rhs)
	{
		return compare(lhs, rhs) == 0;
	}

	bool			xstring::contains(const xstring& str, const xstring& contains)
	{
		s32 pos = 0;
		uchar32 const* src = (uchar32 const*)str.m_slice.begin();
		uchar32 const* end = (uchar32 const*)str.m_slice.end();
		while (src < end)
		{
			uchar32 const* ssrc = src;
			uchar32 const* fsrc = (uchar32 const*)contains.m_slice.begin();
			uchar32 const* fdst = (uchar32 const*)contains.m_slice.end();
			while (ssrc < end && fsrc < fend && *ssrc == *fsrc)
			{
				++ssrc;
				++fsrc;
			}
			if (fsrc == fend)
			{
				return true;
			}
			++pos;
			++src;
		}
		return false;
	}





	//------------------------------------------------------------------------------
	static void user_case_for_string()
	{
		xstring str;
		xstring substr = str(0, 10);
	}

};
