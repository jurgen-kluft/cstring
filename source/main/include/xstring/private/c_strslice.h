#ifndef __XSTRING_STRING_SLICE_H__
#define __XSTRING_STRING_SLICE_H__
#include "cbase/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

#include "cbase/c_debug.h"
#include "cbase/c_allocator.h"

namespace ncore
{
    //==============================================================================
    // String Slice
    // A reference counted slice_t owning an array of runes with a view/window (from,to).
    //==============================================================================
    struct strslice_t
    {
        struct data;

        strslice_t();
        strslice_t(alloc_t* allocator, s32 _count);
        strslice_t(data* data, s32 from, s32 to);

        static void alloc(strslice_t& slice_t, alloc_t* allocator, s32 _count);
        strslice_t  construct(s32 _count) const;

        s32        size() const;
        s32        refcnt() const;
        strslice_t obtain() const;
        void       release();

        void resize(s32 count);
        void insert(s32 count);
        void remove(s32 count);

        strslice_t view(s32 from, s32 to) const;
        bool       split(s32 mid, strslice_t& left, strslice_t& right) const;

        uchar32*       begin();
        uchar32 const* begin() const;
        uchar32*       end();
        uchar32 const* end() const;
        uchar32 const* eos() const;

        uchar32*       at(s32 index);
        uchar32 const* at(s32 index) const;

        // ----------------------------------------------------------------------------------------
        //   SLICE REFERENCE COUNTED DATA
        // ----------------------------------------------------------------------------------------
        struct data
        {
            data();
            data(s32 _count);
            data(u8* data, s32 _count);

            static data sNull;

            data* incref();
            data* incref() const;
            data* decref();

            // This function makes a new 'slice_data_t' with content copied from this
            data* copy(s32 from, s32 to);

            // These functions 'reallocate' this
            data* resize(s32 from, s32 to);
            data* insert(s32 at, s32 count);
            data* remove(s32 at, s32 count);

            static data* alloc(alloc_t* allocator, s32& to_count);

            mutable s32 mRefCount;
            s32         mItemCount; /// Count of total items
            alloc_t*    mAllocator;
            uchar32*    mData;
        };

        data* mData;
    };
} // namespace ncore

#endif
