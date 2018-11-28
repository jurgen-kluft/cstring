#ifndef __XSTRING_STRING_SLICE_H__
#define __XSTRING_STRING_SLICE_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

//==============================================================================
// INCLUDES
//==============================================================================
#include "xbase/x_debug.h"
#include "xbase/x_allocator.h"

namespace xcore
{
	//==============================================================================
	// String Slice
	// A reference counted slice owning an array of runes with a view/window (from,to).
	//==============================================================================
	struct strslice
	{
        struct data;
						strslice();
						strslice(xalloc* allocator, s32 _count);
						strslice(data* data, s32 from, s32 to);

		static void		alloc(strslice& slice, xalloc* allocator, s32 _count);
		strslice		construct(s32 _count) const;

		s32				size() const;
		s32				refcnt() const;

		slice			obtain() const;
		void			release();

		void			resize(s32 count);
		void			insert(s32 count);
		void			remove(s32 count);

		strslice		view(s32 from, s32 to) const;
		bool			split(s32 mid, strslice& left, strslice& right) const;

		uchar32 *		begin();
		uchar32 const*	begin() const;
		uchar32 *		end();
		uchar32 const*	end() const;
		uchar32 const*	eos() const;

		uchar32 *		at(s32 index);
		uchar32 const*	at(s32 index) const;

        // ----------------------------------------------------------------------------------------
        //   SLICE REFERENCE COUNTED DATA
        // ----------------------------------------------------------------------------------------
        struct data
        {
            data();
            data(s32 _count);
            data(xbyte* data, s32 _count);

            static data         sNull;

            data*   	    	incref();
            data*   		    incref() const;
            data*	    		decref();

            // This function makes a new 'slice_data' with content copied from this
            data*	    		copy(s32 from, s32 to);

            // These functions 'reallocate' this
            data*	    		resize(s32 from, s32 to);
            data*	    		insert(s32 at, s32 count);
            data*	    		remove(s32 at, s32 count);

            static data*	    alloc(xalloc* allocator, s32& to_count);

            mutable s32			mRefCount;
            s32					mItemCount;					/// Count of total items
            xalloc*				mAllocator;
            uchar32*			mData;
        };

		data*	    	mData;
        utf32::runes    mRunes
	};
}

#endif
