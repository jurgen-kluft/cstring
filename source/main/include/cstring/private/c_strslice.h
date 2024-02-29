#ifndef __CSTRING_STRING_SLICE_H__
#define __CSTRING_STRING_SLICE_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

#include "ccore/c_debug.h"
#include "cbase/c_allocator.h"

namespace ncore
{
    //==============================================================================
    // String Slice
    // A reference counted slice_t owning an array of runes with a view/window (from,to).
    //==============================================================================
    struct strslice_t
    {
        struct data_t;

        strslice_t();
        strslice_t(alloc_t* allocator, s32 _count);
        strslice_t(data_t* data_t, s32 from, s32 to);

        static void alloc(strslice_t& slice_t, alloc_t* allocator, s32 _count);
        strslice_t  construct(s32 _count) const;

        s32        size() const { return mTo - mFrom; }
        s32        refcnt() const { return mData->mRefCount; }
        strslice_t obtain() const;
        void       release();

        void resize(s32 count);
        void insert(s32 at, s32 count);
        void remove(s32 at, s32 count);
        void replace(s32 from, s32 to, strslice_t const& slice_t);

        strslice_t view(s32 from, s32 to) const;
        bool       split(s32 before, s32 after, strslice_t& left, strslice_t& right) const;

        inline uchar16*       begin() { return mData->mArray + mFrom; }
        inline uchar16 const* begin() const { return mData->mArray + mFrom; }
        inline uchar16*       end() { return mData->mArray + mTo; }
        inline uchar16 const* end() const { return mData->mArray + mTo; }
        inline uchar16 const* eos() const { return mData->mArray + mData->mArrayCap; }

        inline uchar16*       at(s32 index) { return mData->mArray + index; }
        inline uchar16 const* at(s32 index) const { return mData->mArray + index; }

        // ----------------------------------------------------------------------------------------
        //   SLICE REFERENCE COUNTED DATA
        // ----------------------------------------------------------------------------------------
        struct data_t
        {
            data_t();
            data_t(s32 _cap, alloc_t* allocator);
            data_t(uchar16* array, s32 _cap, alloc_t* allocator);

            static data_t sNull;

            inline data_t* incref()
            {
                ++mRefCount;
                return this;
            }
            data_t* incref() const
            {
                ++mRefCount;
                return const_cast<data_t*>(this);
            }
            data_t* decref();

            // This function makes a new 'data_t' with content copied from this
            data_t* copy(s32 from, s32 to);

            // These functions resize the array and return the new array
            data_t* resize(s32 count);

            // These functions insert/remove rune(s) at the specified place
            void insert(s32 at, s32 count);
            void remove(s32 at, s32 count);
            void replace(s32 from, s32 to, strslice_t const& slice_t);

            inline uchar16*       at(s32 index) { return mArray + index; }
            inline uchar16 const* at(s32 index) const { return mArray + index; }

            static data_t* alloc(alloc_t* allocator, s32& to_count);

            mutable s32 mRefCount;   // Reference count
            s32         mArrayCap;   // Capacity of the array, will always point at a null-terminator
            alloc_t*    mAllocator;  // Allocator used to allocate the uchar16 data_t
            uchar16*    mArray;      // Array of runes (UCS-2, UTF-16)
        };

        s32      mFrom;       // View window
        s32      mTo;         // View window
        data_t*  mData;       // Reference counted data_t
        alloc_t* mAllocator;  // Allocator used to allocate strslice_t and data_t
    };
}  // namespace ncore

#endif
