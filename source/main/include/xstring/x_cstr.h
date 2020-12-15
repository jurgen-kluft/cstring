#ifndef __XSTRING_CSTR_H__
#define __XSTRING_CSTR_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

#include "xbase/x_runes.h"

namespace xcore
{
    // A non-dynamic xstr (ascii or utf-32)  with a similar API as 'xstring'
	class xstr
	{
	protected:
		struct data;
		struct range
		{
			inline range() : from(0), to(0) { }
			inline range(s32 f, s32 t) : from(f), to(t) { }
			inline s32 size() const { return to - from; }
			s32 from, to;
		};

	public:
		xstr();
		xstr(char* str, char* end, char* eos);
		xstr(const char* str, const char* end = nullptr);
		xstr(utf32::rune* str, utf32::rune* end, utf32::rune* eos);
		xstr(const utf32::rune* str, const utf32::rune* end);
		xstr(xstr const& other);
		~xstr();

		struct view
		{
			view(const view& other);
			~view();

			s32		size() const;
			bool	is_valid() const;
			bool	is_empty() const;

			view  operator()(s32 to);
			view  operator()(s32 from, s32 to);
			view  operator()(s32 to) const;
			view  operator()(s32 from, s32 to) const;

			uchar32 operator[](s32 index) const;

			view& operator=(view const& other);

			bool operator == (view const& other) const;
			bool operator != (view const& other) const;

		protected:
			friend class xstr;
			friend class xview;
			view(data*);

			void	add();
			void	rem();
			void	invalidate();

		    data*   m_data;
			range	m_view;

			view*	m_next;
			view*	m_prev;

		private:
			void*	operator new(xcore::xsize_t num_bytes, void* mem)			{ return mem; }
			void	operator delete(void* mem, void* )							{ }
			void*	operator new(xcore::xsize_t num_bytes)						{ return nullptr; }
			void	operator delete(void* mem)									{ }
		};

		xstr(xstr const& str, xstr::view const& left, xstr::view const& right);

		bool is_empty() const;
		bool is_valid() const;
		s32  cap() const;
		s32  size() const;

		void clear();

		view full();

		view operator()(s32 to);
		view operator()(s32 from, s32 to);

		uchar32 operator[](s32 index) const;

		xstr& operator=(const xstr& other);
		xstr& operator=(const xstr::view& other);

		bool operator==(const xstr& other) const;
		bool operator!=(const xstr& other) const;

		operator view() { return full(); }

	protected:
		friend struct view;
		friend class xview;

		void release();

        struct data
        {
    		mutable view* m_views;
            union runes {
                inline runes() : _ascii() {}
                ascii::crunes_t  _cascii;
                utf32::crunes_t  _cutf32;
                ascii::runes   _ascii;
                utf32::runes   _utf32;
            };
            runes m_runes;
            s32   m_type;

			data() : m_views(nullptr), m_runes(), m_type(0) {}
			data(const data& d) : m_views(nullptr), m_runes(), m_type(0) {}
        };
        data   m_data;
	};


    uchar32 firstChar(const xstr::view& str);
    uchar32 lastChar(const xstr::view& str);
	bool isQuoted(const xstr::view& str, uchar32 inQuote) ;
    bool isQuoted(const xstr::view& str) ;
    bool isDelimited(const xstr::view& str, uchar32 inLeft, uchar32 inRight);
    bool startsWith(const xstr::view& str, xstr::view const& start);
    bool endsWith(const xstr::view& str, xstr::view const& end);
    xstr::view find(xstr& str, uchar32 find);
    xstr::view find(xstr::view& str, uchar32 find);
    xstr::view find(xstr& str, const xstr::view& find);
    xstr::view find(xstr::view& str, const xstr::view& find);
    xstr::view findLast(const xstr::view& str, const xstr::view& find);
    xstr::view findOneOf(const xstr::view& str, const xstr::view& charset);
    xstr::view findOneOfLast(const xstr::view& str, const xstr::view& charset);
    s32 compare(const xstr::view& lhs, const xstr::view& rhs);
    bool isEqual(const xstr::view& lhs, const xstr::view& rhs);
    bool contains(const xstr::view& str, const xstr::view& contains);
    bool contains(const xstr::view& str, uchar32 contains);
    void concatenate_repeat(xstr& str, xstr::view const& con, s32 ntimes);
    s32 format(xstr& str, xstr::view const& format, const x_va_list& args);
    s32 formatAdd(xstr& str, xstr::view const& format, const x_va_list& args);
    void insert(xstr& str, xstr::view const& pos, xstr::view const& insert);
    void insert_after(xstr& str, xstr::view const& pos, xstr::view const& insert);
    void remove(xstr& str, xstr::view const& selection);
    void find_remove(xstr& str, const xstr::view& _find);
    void find_replace(xstr& str, const xstr::view& find, const xstr::view& replace);
    void remove_any(xstr& str, const xstr::view& any);
    void replace_any(xstr::view& str, const xstr::view& any, uchar32 with);
    void upper(xstr::view& str);
    void lower(xstr::view& str);
    void capitalize(xstr::view& str);
    void capitalize(xstr::view& str, xstr::view const& separators);
    void trim(xstr::view& str);
    void trimLeft(xstr::view& str);
    void trimRight(xstr::view& str);
    void trim(xstr::view& str, uchar32 r);
    void trimLeft(xstr::view& str, uchar32 r);
    void trimRight(xstr::view& str, uchar32 r);
    void trim(xstr::view& str, xstr::view const& set);
    void trimLeft(xstr::view& str, xstr::view const& set);
    void trimRight(xstr::view& str, xstr::view const& set);
    void trimQuotes(xstr::view& str);
    void trimQuotes(xstr::view& str, uchar32 quote);
    void trimDelimiters(xstr::view& str, uchar32 left, uchar32 right);
    void reverse(xstr::view& str);
    bool splitOn(xstr::view& str, uchar32 inChar, xstr::view& outLeft, xstr::view& outRight);
    bool splitOn(xstr::view& str, xstr::view& inStr, xstr::view& outLeft, xstr::view& outRight);
    bool splitOnLast(xstr::view& str, uchar32 inChar, xstr::view& outLeft, xstr::view& outRight);
	bool splitOnLast(xstr::view& str, xstr::view& inStr, xstr::view& outLeft, xstr::view& outRight);
    void concatenate(xstr& str, const xstr::view& con);


} // namespace xcore

#endif