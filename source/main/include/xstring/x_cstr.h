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
		};

		xstr(xstr const& str, xstr::view const& left, xstr::view const& right);

		bool is_empty() const;
		s32  cap() const;
		s32  size() const;

		void clear();

		view full();
		view full() const;

		view operator()(s32 to);
		view operator()(s32 from, s32 to);

		view operator()(s32 to) const;
		view operator()(s32 from, s32 to) const;

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
                ascii::crunes  _cascii;
                utf32::crunes  _cutf32;
                ascii::runes   _ascii;
                utf32::runes   _utf32;
            };
            runes m_runes;
            s32   m_type;
        };
        data   m_data;
	};

} // namespace xcore

#endif