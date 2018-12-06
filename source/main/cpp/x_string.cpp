
//==============================================================================
// INCLUDES
//==============================================================================
#include "xstring/x_string.h"

#include "xbase/x_allocator.h"
#include "xbase/x_integer.h"
#include "xbase/x_memory.h"
#include "xbase/x_runes.h"
#include "xbase/x_printf.h"

namespace xcore
{
	//==============================================================================
	class xview
	{
	public:
		static inline uchar32* get_ptr_unsafe(xstring::view& str, s32 i) { return &str.m_runes.m_str[i]; }
		static inline uchar32  get_char_unsafe(xstring::view& str, s32 i) { return str.m_runes.m_str[i]; }
		static inline uchar32* set_char_unsafe(xstring::view& str, s32 i, uchar32 c) { str.m_runes.m_str[i] = c; }

		static void resize(xstring& str, s32 new_size)
		{
			if (str.size() < new_size)
			{
				utf32::runes nrunes = str.m_allocator->allocate(0, new_size);
				utf32::copy(str.m_runes, nrunes);
				str.release();
				str.m_runes = nrunes;
			}
		}

		static bool narrow_view(xstring::view& v, s32 move)
		{
			// if negative then narrow the left side
			if (move < 0)
			{
				move = -move;
				if (move <= v.m_size)
				{
					v.m_from = v.m_from + move;
					v.m_size = v.m_size - move;
					return true;
				}
			}
			else
			{
				if (move <= v.m_size)
				{
					v.m_size -= move;
					return true;
				}
			}
			return false;
		}

		static bool move_view(xstring::view& v, s32 move)
		{
			s32 from = v.m_from + move;

			// Check if the move doesn't result in a negative @from
			if (from < 0)
				return false;

			// Check if the movement doesn't invalidate this view!
			uchar32 const* end = v.m_runes.m_str + from + v.m_size;
			if (end > v.m_runes.m_end)
				return false;

			// Movement is ok, new view is valid
			return true;
		}

		static xstring::view select_before(const xstring::view& str, const xstring::view& selection)
		{
			xstring::view v(selection);
			s32			  selection_size = v.m_from;
			v.m_from					 = 0;
			v.m_size					 = selection_size;
			return v;
		}
		static xstring::view select_before_included(const xstring::view& str, const xstring::view& selection)
		{
			xstring::view v(selection);
			s32			  selection_size = v.m_from + v.m_size;
			v.m_from					 = 0;
			v.m_size					 = selection_size;
			return v;
		}

		static xstring::view default() { return xstring::view(); }
	};

	//==============================================================================
	static s32 WriteCodeUnit(uchar32 cdpt, uchar8*& pDst)
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

	static s32 WriteCodeUnit(uchar32 cdpt, uchar16*& pDst)
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

	bool ReadCodePoint(uchar8 const*& str, uchar8 const* pSrcEnd, uchar32& cdpt)
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

	bool ReadCodePoint(uchar16 const*& pSrc, uchar16 const* pSrcEnd, uchar32& cdpt)
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

	static s32 ascii_nr_chars(uchar const* src, uchar const*& src_end)
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
			c = (s32)(src_end - src);
		}
		return c;
	}

	static void ascii_to_utf32(uchar const* src, uchar const* src_end, uchar32* dst, uchar32 const* dst_end)
	{
		while (src < src_end && dst < dst_end)
		{
			uchar32 c = *src++;
			*dst++	= c;
		}
		return;
	}

	static s32 utf8_nr_chars(uchar8 const* src, uchar8 const* src_end)
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

	static void utf8_to_utf32(uchar8 const* src, uchar8 const* src_end, uchar32* dst, uchar32 const* dst_end)
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

	static s32 utf16_nr_chars(uchar16 const* src, uchar16 const* src_end)
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

	static void utf16_to_utf32(uchar16 const* src, uchar16 const* src_end, uchar32* dst, uchar32 const* dst_end)
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

	//------------------------------------------------------------------------------
	//------------------------------------------------------------------------------
	//------------------------------------------------------------------------------
	xstring::view::view() : m_alloc(nullptr), m_runes(), m_from(0), m_size(0), m_list(nullptr), m_next(nullptr), m_prev(nullptr) {}

	xstring::view::~view() { rem(); }

	s32 xstring::view::size() const { return m_size; }

	bool xstring::view::is_empty() const { return m_size == 0; }

	xstring xstring::view::to_string() const
	{
		if (m_alloc != nullptr && !m_runes.is_empty())
		{
			utf32::runes dstrunes = m_alloc->allocate(0, m_size);
			utf32::runes srcrunes = m_runes;
			srcrunes.m_str += m_from;
			utf32::copy(srcrunes, dstrunes);

			xstring str;
			str.m_allocator = m_alloc;
			str.m_runes		= dstrunes;
			return str;
		}
		return xstring();
	}

	xstring::view xstring::view::operator()(s32 to)
	{
		to = xmin(m_size, to);
		xstring::view v;
		v.m_alloc = m_alloc;
		v.m_runes = m_runes;
		v.m_const = m_const;
		v.m_from  = m_from;
		v.m_size  = to;
		v.add(m_list);
		return v;
	}

	xstring::view xstring::view::operator()(s32 from, s32 to)
	{
		s32 f = xmin(from, to);
		s32 t = xmax(from, to);
		from  = xmin(f, m_size);
		to	= xmin(t, m_size);
		xstring::view v;
		v.m_alloc = m_alloc;
		v.m_runes = m_runes;
		v.m_const = m_const;
		v.m_from  = m_from + from;
		v.m_size  = to - from;
		v.add(m_list);
		return v;
	}

	xstring::view xstring::view::operator()(s32 to) const
	{
		to = xmin(m_size, to);
		xstring::view v;
		v.m_alloc = m_alloc;
		v.m_runes = m_runes;
		v.m_const = m_const;
		v.m_from  = m_from;
		v.m_size  = to;
		v.add(m_list);
		return v;
	}

	xstring::view xstring::view::operator()(s32 from, s32 to) const
	{
		s32 f = xmin(from, to);
		s32 t = xmax(from, to);
		from  = xmin(f, m_size);
		to	= xmin(t, m_size);
		xstring::view v;
		v.m_alloc = m_alloc;
		v.m_runes = m_runes;
		v.m_const = m_const;
		v.m_from  = m_from + from;
		v.m_size  = to - from;
		v.add(m_list);
		return v;
	}

	uchar32 xstring::view::operator[](s32 index) const
	{
		if (index < 0 || index > m_size)
			return '\0';
		return m_runes.m_str[m_from + index];
	}

	xstring::view& xstring::view::operator=(xstring::view const& other)
	{
		rem();

		m_alloc = other.m_alloc;
		m_runes = other.m_runes;
		m_from  = other.m_from;
		m_size  = other.m_size;
		add(other.m_list);

		return *this;
	}

	bool xstring::view::operator==(xstring::view const& other) const
	{
		if (m_size == other.m_size)
		{
			uchar32 const* tsrc = m_runes.m_str + m_from;
			uchar32 const* tend = m_runes.m_str + m_from + m_size;
			uchar32 const* osrc = other.m_runes.m_str + other.m_from;
			uchar32 const* oend = other.m_runes.m_str + other.m_from + other.m_size;
			while (tsrc < tend)
			{
				if (*tsrc++ != *osrc++)
					return false;
			}
			return true;
		}
		return false;
	}

	bool xstring::view::operator!=(xstring::view const& other) const
	{
		if (m_size == other.m_size)
		{
			uchar32 const* tsrc = m_runes.m_str + m_from;
			uchar32 const* tend = m_runes.m_str + m_from + m_size;
			uchar32 const* osrc = other.m_runes.m_str + other.m_from;
			uchar32 const* oend = other.m_runes.m_str + other.m_from + other.m_size;
			while (tsrc < tend)
			{
				if (*tsrc++ != *osrc++)
					return true;
			}
			return false;
		}
		return true;
	}

	void xstring::view::add(xstring::view** _list)
	{
		m_list = _list;

		xstring::view*& list = *m_list;
		if (list == nullptr)
		{
			list   = this;
			m_next = this;
			m_prev = this;
		}
		else
		{
			xstring::view*& prev = list->m_prev;
			xstring::view*& next = list;
			prev->m_next		 = this;
			next->m_prev		 = this;
			m_next				 = next;
			m_prev				 = prev;
			list				 = this;
		}
	}

	void xstring::view::rem()
	{
		xstring::view*& list = *m_list;

		xstring::view* prev = m_prev;
		xstring::view* next = m_next;
		prev->m_next		= next;
		next->m_prev		= prev;

		if (list == this)
		{
			list = next;
			if (list == this)
			{
				list = nullptr;
			}
		}

		m_list  = nullptr;
		m_alloc = nullptr;
		m_runes = utf32::runes();
		m_next  = nullptr;
		m_prev  = nullptr;
	}

	void xstring::view::invalidate()
	{
		m_alloc = nullptr;
		m_runes = utf32::runes();
		m_from  = 0;
		m_size  = 0;
	}

	utf32::crunes xstring::view::get_runes() const
	{
		if (m_runes.is_empty())
		{
			utf32::crunes r(m_runes);
			r.m_str += m_from;
			r.m_end = r.m_str + m_size;
			return r;
		}
		return utf32::crunes();
	}

	//------------------------------------------------------------------------------
	//------------------------------------------------------------------------------
	//------------------------------------------------------------------------------
	class utf32_runes_allocator : public utf32::alloc
	{
	public:
		virtual utf32::runes allocate(s32 len, s32 cap)
		{
			if (len > cap)
				cap = len;

			utf32::runes r;
			r.m_str			 = (utf32::prune)xalloc::get_system()->allocate(cap * sizeof(utf32::rune), sizeof(void*));
			r.m_end			 = r.m_str + len;
			r.m_eos			 = r.m_str + cap - 1;
			r.m_end[0]		 = '\0';
			r.m_end[cap - 1] = '\0';
			return r;
		}

		virtual void deallocate(utf32::runes& r)
		{
			xalloc::get_system()->deallocate(r.m_str);
			r = utf32::runes();
		}
	};

	static utf32_runes_allocator s_utf32_runes_allocator;
	utf32::alloc*				 xstring::s_allocator = &s_utf32_runes_allocator;

	xstring::xstring(void) : m_allocator(nullptr), m_runes(), m_views(nullptr) {}

	xstring::xstring(const char* str) : m_allocator(s_allocator), m_runes(), m_views(nullptr)
	{
		const char* end = nullptr;
		s32 const   len = ascii_nr_chars(str, end);
		m_runes			= m_allocator->allocate(0, len);
		ascii_to_utf32(str, end, m_runes.m_end, m_runes.m_eos);
	}

	xstring::xstring(utf32::alloc* _allocator, s32 _len) : m_allocator(_allocator), m_runes(), m_views(nullptr) { m_runes = m_allocator->allocate(0, _len); }

	xstring::xstring(const xstring& other) : m_allocator(other.m_allocator), m_runes(), m_views(nullptr)
	{
		m_runes = m_allocator->allocate(0, other.m_runes.cap());
		utf32::copy(other.m_runes, m_runes);
	}

	xstring::xstring(const xstring::view& left, const xstring::view& right)
	{
		m_allocator = left.m_alloc;

		s32 cap = left.size() + right.size();
		m_runes = m_allocator->allocate(0, cap);
		utf32::concatenate(m_runes, left.get_runes(), right.get_runes());
	}

	xstring::~xstring()
	{
		if (m_allocator != nullptr)
			m_allocator->deallocate(m_runes);
	}

	//------------------------------------------------------------------------------
	//------------------------------------------------------------------------------

	bool xstring::is_empty() const { return m_runes.size() == 0; }

	s32 xstring::size() const { return m_runes.size(); }

	xstring::view xstring::full()
	{
		xstring::view v;
		v.m_alloc = m_allocator;
		v.m_runes = m_runes;
		v.m_const = false;
		v.m_from  = 0;
		v.m_size  = m_runes.size();
		v.add(&m_views);
		return v;
	}

	xstring::view xstring::full() const
	{
		xstring::view v;
		v.m_alloc = m_allocator;
		v.m_runes = m_runes;
		v.m_const = true;
		v.m_from  = 0;
		v.m_size  = m_runes.size();
		v.add(&m_views);
		return v;
	}

	xstring::view xstring::operator()(s32 to)
	{
		to = xmin(size(), to);
		xstring::view v;
		v.m_alloc = m_allocator;
		v.m_runes = m_runes;
		v.m_const = false;
		v.m_from  = 0;
		v.m_size  = to;
		v.add(&m_views);
		return v;
	}

	xstring::view xstring::operator()(s32 from, s32 to)
	{
		s32 f = xmin(from, to);
		s32 t = xmax(from, to);
		from  = xmin(f, size());
		to	= xmin(t, size());
		xstring::view v;
		v.m_alloc = m_allocator;
		v.m_runes = m_runes;
		v.m_const = false;
		v.m_from  = from;
		v.m_size  = to - from;
		v.add(&m_views);
		return v;
	}

	xstring::view xstring::operator()(s32 to) const
	{
		to = xmin(size(), to);
		xstring::view v;
		v.m_alloc = m_allocator;
		v.m_runes = m_runes;
		v.m_const = true;
		v.m_from  = 0;
		v.m_size  = to;
		v.add(&m_views);
		return v;
	}

	xstring::view xstring::operator()(s32 from, s32 to) const
	{
		s32 f = xmin(from, to);
		s32 t = xmax(from, to);
		from  = xmin(f, size());
		to	= xmin(t, size());
		xstring::view v;
		v.m_alloc = m_allocator;
		v.m_runes = m_runes;
		v.m_const = true;
		v.m_from  = from;
		v.m_size  = to - from;
		v.add(&m_views);
		return v;
	}

	uchar32 xstring::operator[](s32 index)
	{
		if (index >= m_runes.size())
			return '\0';
		return m_runes.m_str[index];
	}

	uchar32 xstring::operator[](s32 index) const
	{
		if (index >= m_runes.size())
			return '\0';
		return m_runes.m_str[index];
	}

	xstring& xstring::operator=(const xstring& other)
	{
		if (this != &other)
		{
			release();
			clone(other.m_runes, other.m_allocator);
		}
		return *this;
	}

	xstring& xstring::operator=(const xstring::view& other)
	{
		if (&m_views != other.m_list)
		{
			release();
			clone(other.m_runes, other.m_alloc);
		}
		return *this;
	}

	void xstring::release()
	{
		if (m_allocator != nullptr)
		{
			m_allocator->deallocate(m_runes);
		}

		// Invalidate all views
		xstring::view* iter = m_views;
		while (iter != nullptr)
		{
			iter->invalidate();
			iter = iter->m_next;
			if (iter == m_views)
				break;
		}
	}

	void xstring::clone(utf32::runes const& str, utf32::alloc* allocator)
	{
		m_allocator = allocator;
		m_runes		= m_allocator->allocate(str.size(), str.cap());
		utf32::copy(str, m_runes);
	}

	//------------------------------------------------------------------------------
	xstring::view selectUntil(const xstring::view& str, uchar32 find)
	{
		for (s32 i = 0; i < str.size(); i++)
		{
			uchar32 const c = str[i];
			if (c == find)
			{
				return str(0, i + 1);
			}
		}
		return xview::default();
	}

	xstring::view selectUntil(const xstring::view& str, const xstring::view& find)
	{
		xstring::view v = str(0, find.size());
		while (!v.is_empty())
		{
			if (v == find)
			{
				// So here we have a view with the size of the @find string on
				// string @str that matches the string @find and we need to return
				// a string view that exist before view @v.
				return xview::select_before(str, v);
			}
			if (!xview::move_view(v, 1))
				break;
		}
		return xview::default();
	}


	xstring::view selectUntilLast(const xstring::view& str, uchar32 find)
	{
		for (s32 i = str.size() - 1; i>=0; --i)
		{
			uchar32 const c = str[i];
			if (c == find)
			{
				return str(0, i + 1);
			}
		}
		return xview::default();
	}

	xstring::view selectUntilLast(const xstring::view& str, const xstring::view& find)
	{
		xstring::view v = str(str.size() - find.size(), str.size());
		while (!v.is_empty())
		{
			if (v == find)
			{
				// So here we have a view with the size of the @find string on
				// string @str that matches the string @find and we need to return
				// a string view that exist before view @v.
				return xview::select_before(str, v);
			}
			if (!xview::move_view(v, -1))
				break;
		}
		return xview::default();
	}

	xstring::view selectUntilIncluded(const xstring::view& str, const xstring::view& find)
	{
		xstring::view v = str(0, find.size());
		while (!v.is_empty())
		{
			if (v == find)
			{
				// So here we have a view with the size of the @find string on
				// string @str that matches the string @find and we need to return
				// a string view that exist before and includes view @v.
				return xview::select_before_included(str, v);
			}
			if (!xview::move_view(v, 1))
				break;
		}
		return xview::default();
	}

	bool isUpper(const xstring::view& str)
	{
		for (s32 i = 0; i < str.size(); i++)
		{
			uchar32 const c = str[i];
			if (utf32::is_lower(c))
				return false;
		}
		return true;
	}

	bool isLower(const xstring::view& str)
	{
		for (s32 i = 0; i < str.size(); i++)
		{
			uchar32 const c = str[i];
			if (utf32::is_upper(c))
				return false;
		}
		return true;
	}

	bool isCapitalized(const xstring::view& str)
	{
		s32 i = 0;
		while (i < str.size())
		{
			uchar32 c = '\0';
			while (i < str.size())
			{
				c = str[i++];
				if (!utf32::is_space(c))
					break;
			}

			if (utf32::is_upper(c))
			{
				while (i < str.size())
				{
					c = str[i++];
					if (utf32::is_space(c))
						break;
					if (!utf32::is_lower(c))
						return false;
				}
			}
			else if (utf32::is_alpha(c))
			{
				return false;
			}
			else
			{
				++i;
			}
		}
		return true;
	}

	bool isQuoted(const xstring::view& str) { return isQuoted(str, '"'); }

	bool isQuoted(const xstring::view& str, uchar32 inQuote) { return isDelimited(str, '"', '"'); }

	bool isDelimited(const xstring::view& str, uchar32 inLeft, uchar32 inRight)
	{
		if (str.is_empty())
			return false;
		s32 const first = 0;
		s32 const last  = str.size() - 1;
		return str[first] == inLeft && str[last] == inRight;
	}

	uchar32 firstChar(const xstring::view& str)
	{
		s32 const first = 0;
		return str[first];
	}

	uchar32 lastChar(const xstring::view& str)
	{
		s32 const last = str.size() - 1;
		return str[last];
	}

	bool startsWith(const xstring::view& str, xstring::view const& start)
	{
		xstring::view v = str(0, start.size());
		if (!v.is_empty())
			return (v == start);
		return false;
	}

	bool endsWith(const xstring::view& str, xstring::view const& end)
	{
		xstring::view v = str(str.size() - end.size(), str.size());
		if (!v.is_empty())
			return (v == end);
		return false;
	}

	xstring::view find(const xstring::view& str, uchar32 find)
	{
		for (s32 i = 0; i < str.size(); i++)
		{
			uchar32 const c = str[i];
			if (c == find)
				return str(i, i + 1);
		}
		return xview::default();
	}

	xstring::view find(xstring::view& str, const xstring::view& find)
	{
		xstring::view v = str(0, find.size());
		while (!v.is_empty())
		{
			if (v == find)
			{
				// So here we have a view with the size of the @find string on
				// string @str that matches the string @find.
				return v;
			}
			if (!xview::move_view(v, 1))
				break;
		}
		return xview::default();
	}

	xstring::view findLast(const xstring::view& str, const xstring::view& find)
	{
		xstring::view v = str(str.size() - find.size(), str.size());
		while (!v.is_empty())
		{
			if (v == find)
			{
				// So here we have a view with the size of the @find string on
				// string @str that matches the string @find.
				return v;
			}
			if (!xview::move_view(v, -1))
				break;
		}
		return xview::default();
	}

	xstring::view findOneOf(const xstring::view& str, const xstring::view& find)
	{
		for (s32 i = 0; i < str.size(); i++)
		{
			uchar32 const sc = str[i];
			for (s32 j = 0; j < find.size(); j++)
			{
				uchar32 const fc = find[i];
				if (sc == fc)
				{
					return str(i, i + 1);
				}
			}
		}
		return xview::default();
	}

	xstring::view findOneOfLast(const xstring::view& str, const xstring::view& find)
	{
		for (s32 i = str.size() - 1; i >= 0; i--)
		{
			uchar32 const sc = str[i];
			for (s32 j = 0; j < find.size(); j++)
			{
				uchar32 const fc = find[i];
				if (sc == fc)
				{
					return str(i, i + 1);
				}
			}
		}
		return xview::default();
	}

	s32 compare(const xstring::view& lhs, const xstring::view& rhs)
	{
		if (lhs.size() < rhs.size())
			return -1;
		if (lhs.size() > rhs.size())
			return 1;

		for (s32 i = 0; i < lhs.size(); i++)
		{
			uchar32 const lc = lhs[i];
			uchar32 const rc = rhs[i];
			if (lc < rc)
				return -1;
			else if (lc > rc)
				return 1;
		}
		return 0;
	}

	bool isEqual(const xstring::view& lhs, const xstring::view& rhs) { return compare(lhs, rhs) == 0; }

	bool contains(const xstring::view& str, const xstring::view& contains)
	{
		xstring::view v = str(0, contains.size());
		while (!v.is_empty())
		{
			if (v == contains)
			{
				// So here we have a view with the size of the @find string on
				// string @str that matches the string @find.
				return true;
			}
			if (!xview::move_view(v, 1))
				break;
		}
		return false;
	}

	bool contains(const xstring::view& str, uchar32 contains)
	{
		for (s32 i = 0; i < str.size(); i++)
		{
			uchar32 const sc = str[i];
			if (sc == contains)
			{
				return true;
			}
		}
		return false;
	}

	void concatenate_repeat(xstring& str, xstring::view const& con, s32 ntimes)
	{
		s32 const len = str.size() + (con.size() * ntimes);
		xview::resize(str, len);
		for (s32 i = 0; i < ntimes; ++i)
		{
			concatenate(str, con);
		}
	}

	s32 format(xstring& str, xstring::view const& format, const x_va_list& args) { return 0; }

	s32 formatAdd(xstring& str, xstring::view const& format, const x_va_list& args) { return 0; }

	void insert(xstring& str, xstring::view const& pos, xstring::view const& insert) {}

	void remove(xstring& str, xstring::view const& selection) {}

	void find_remove(xstring& str, const xstring::view& find) {}

	void find_replace(xstring& str, const xstring::view& find, const xstring::view& remove) {}

	void remove_any(xstring& str, const xstring::view& any)
	{
		// Remove any of the characters in @charset from @str
	}

	void replace_any(xstring& str, const xstring::view& any, uchar32 with)
	{
		// Remove any of the characters in @charset from @str
	}

	void upper(xstring::view& str)
	{
		for (s32 i = 0; i < str.size(); i++)
		{
			uchar32* cp = xview::get_ptr_unsafe(str, i);
			*cp			= utf32::to_upper(*cp);
		}
	}

	void lower(xstring::view& str)
	{
		for (s32 i = 0; i < str.size(); i++)
		{
			uchar32* cp = xview::get_ptr_unsafe(str, i);
			*cp			= utf32::to_lower(*cp);
		}
	}

	void capitalize(xstring::view& str)
	{
		// Standard separator is ' '
		bool prev_is_space = true;
		s32  i			   = 0;
		while (i < str.size())
		{
			uchar32 c = xview::get_char_unsafe(str, i);
			uchar32 d = c;
			if (utf32::is_alpha(c))
			{
				if (prev_is_space)
				{
					c = utf32::to_upper(c);
				}
				else
				{
					c = utf32::to_lower(c);
				}
			}
			else
			{
				prev_is_space = utf32::is_space(c);
			}

			if (c != d)
			{
				xview::set_char_unsafe(str, i, c);
			}
			i++;
		}
	}

	void capitalize(xstring::view& str, xstring::view const& separators)
	{
		bool prev_is_space = false;
		s32  i			   = 0;
		while (i < str.size())
		{
			uchar32 c = xview::get_char_unsafe(str, i);
			uchar32 d = c;
			if (utf32::is_alpha(c))
			{
				if (prev_is_space)
				{
					c = utf32::to_upper(c);
				}
				else
				{
					c = utf32::to_lower(c);
				}
			}
			else
			{
				prev_is_space = false;
				for (s32 j = 0; j < separators.size(); j++)
				{
					if (separators[j] == c)
					{
						prev_is_space = true;
						break;
					}
				}
			}
			if (c != d)
			{
				xview::set_char_unsafe(str, i, c);
			}
			i++;
		}
	}

	// Trim does nothing more than narrowing the <from, to>, nothing is actually removed
	// from the actual underlying string data.
	void trim(xstring::view& str)
	{
		trimLeft(str);
		trimRight(str);
	}

	void trimLeft(xstring::view& str)
	{
		for (s32 i = 0; i < str.size(); ++i)
		{
			uchar32 c = xview::get_char_unsafe(str, i);
			if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
			{
				if (i > 0)
				{
					xview::narrow_view(str, -i);
				}
				return;
			}
		}
	}

	void trimRight(xstring::view& str)
	{
		s32 const last = str.size() - 1;
		for (s32 i = 0; i < str.size(); ++i)
		{
			uchar32 c = xview::get_char_unsafe(str, last - i);
			if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
			{
				if (i > 0)
				{
					xview::narrow_view(str, i);
				}
				return;
			}
		}
	}

	void trim(xstring::view& str, uchar32 r)
	{
		trimLeft(str, r);
		trimRight(str, r);
	}

	void trimLeft(xstring::view& str, uchar32 r)
	{
		for (s32 i = 0; i < str.size(); ++i)
		{
			uchar32 c = xview::get_char_unsafe(str, i);
			if (c != r)
			{
				if (i > 0)
				{
					xview::narrow_view(str, -i);
				}
				return;
			}
		}
	}

	void trimRight(xstring::view& str, uchar32 r)
	{
		s32 const last = str.size() - 1;
		for (s32 i = 0; i < str.size(); ++i)
		{
			uchar32 c = xview::get_char_unsafe(str, last - i);
			if (c != r)
			{
				if (i > 0)
				{
					xview::narrow_view(str, i);
				}
				return;
			}
		}
	}

	void trim(xstring::view& str, xstring::view const& set)
	{
		trimLeft(str, set);
		trimRight(str, set);
	}

	void trimLeft(xstring::view& str, xstring::view const& set)
	{
		for (s32 i = 0; i < str.size(); ++i)
		{
			uchar32 c = xview::get_char_unsafe(str, i);
			if (!contains(set, c))
			{
				if (i > 0)
				{
					xview::narrow_view(str, -i);
				}
				return;
			}
		}
	}

	void trimRight(xstring::view& str, xstring::view const& set)
	{
		s32 const last = str.size() - 1;
		for (s32 i = 0; i < str.size(); ++i)
		{
			uchar32 c = xview::get_char_unsafe(str, last - i);
			if (!contains(set, c))
			{
				if (i > 0)
				{
					xview::narrow_view(str, i);
				}
				return;
			}
		}
	}

	void trimQuotes(xstring::view& str) { trimDelimiters(str, '"', '"'); }

	void trimQuotes(xstring::view& str, uchar32 quote) { trimDelimiters(str, quote, quote); }

	void trimDelimiters(xstring::view& str, uchar32 left, uchar32 right)
	{
		trimLeft(str, left);
		trimRight(str, right);
	}

	void reverse(xstring::view& str)
	{
		s32 const last = str.size() - 1;
		for (s32 i = 0; i < (last - i); ++i)
		{
			uchar32 l = xview::get_char_unsafe(str, i);
			uchar32 r = xview::get_char_unsafe(str, last - i);
			xview::set_char_unsafe(str, i, r);
			xview::set_char_unsafe(str, last - i, l);
		}
	}

	bool splitOn(xstring::view& str, uchar32 inChar, xstring::view& outLeft, xstring::view& outRight)
	{
		outLeft = selectUntil(str, inChar);
		if (outLeft.is_empty())
			return false;
		outRight = str(outLeft.size(), str.size());
		trimRight(outLeft, inChar);
		return true;
	}

	bool splitOn(xstring::view& str, xstring::view& inStr, xstring::view& outLeft, xstring::view& outRight)
	{
		outLeft = selectUntil(str, inStr);
		if (outLeft.is_empty())
			return false;
		outRight = str(outLeft.size() + inStr.size(), str.size());
		return true;
	}

	bool splitOnLast(xstring::view& str, uchar32 inChar, xstring::view& outLeft, xstring::view& outRight)
	{
		outLeft = selectUntilLast(str, inChar);
		if (outLeft.is_empty())
			return false;
		outRight = str(outLeft.size(), str.size());
		trimRight(outLeft, inChar);
		return true;
	}

	bool splitOnLast(xstring::view& str, xstring::view& inStr, xstring::view& outLeft, xstring::view& outRight)
	{
		outLeft = selectUntilLast(str, inStr);
		if (outLeft.is_empty())
			return false;
		outRight = str(outLeft.size() + inStr.size(), str.size());
		return true;
	}

	void concatenate(xstring& str, const xstring::view& con)
	{

	}

	//------------------------------------------------------------------------------
	static void user_case_for_string()
	{
		xstring		  str("This is an ascii string that will be converted to UTF-32");
		xstring::view substr = find(str.full(), xstring("ascii").full());
		upper(substr);
	}

} // namespace xcore
