#include "cbase/c_allocator.h"
#include "cbase/c_context.h"
#include "cbase/c_integer.h"
#include "cbase/c_memory.h"
#include "cbase/c_printf.h"
#include "cbase/c_runes.h"
#include "cstring/c_string.h"

namespace ncore
{
    // Strings are stored in memory in UTF-16 format the internal encoding is more correctly described as UCS-2.
    // All code here assumes 2 bytes is one codepoint so only the Basic Multilingual Plane (BMP) is supported.
    // Strings are stored in the endian-ness appropriate for the current platform.
    // holds the memory allocators for string_t
    class string_memory_t
    {
    public:
        static alloc_t* s_object_alloc;  // for instance_t and data_t
        static alloc_t* s_string_alloc;  // for the actual string data

        static void init(alloc_t* object_alloc = nullptr, alloc_t* string_alloc = nullptr);
    };

    alloc_t* string_memory_t::s_object_alloc = nullptr;  // for instance_t and data_t
    alloc_t* string_memory_t::s_string_alloc = nullptr;  // for the actual string data

    void string_memory_t::init(alloc_t* object_alloc, alloc_t* string_alloc)
    {
        s_object_alloc = object_alloc;
        s_string_alloc = string_alloc;
    }

    struct string_t::data_t  // 24 bytes
    {
        uchar16*              m_ptr;   // UCS-2
        string_t::instance_t* m_head;  // The first view of this string, single linked list of instances
        u32                   m_len;
        s32                   m_ref;

        inline s64 cap() const { return m_len; }

        string_t::data_t* attach()
        {
            m_ref++;
            return this;
        }

        string_t::data_t* detach();
    };

    struct string_t::range_t
    {
        u32 m_from;
        u32 m_to;

        inline bool is_empty() const { return m_from == m_to; }
        inline u32  size() const { return m_to - m_from; }
        inline bool is_inside(range_t const& parent) const { return m_from >= parent.m_from && m_to <= parent.m_to; }
    };

    struct string_t::instance_t  // 24 bytes
    {
        string_t::range_t     m_range;  // [from,to] view on the string data
        string_t::instance_t* m_next;   // single linked list of instances that also own 'm_data'
        string_t::data_t*     m_data;   // reference counted string data

        inline bool is_empty() const { return m_range.is_empty(); }
        inline bool is_slice() const { return m_data->m_head != nullptr; }
        inline s64  cap() const { return m_data->cap(); }
        inline s64  size() const { return m_range.size(); }

        string_t::instance_t* clone_full() const;
        string_t::instance_t* clone_slice() const;

        void add(string_t::instance_t* node);
        void rem();

        string_t::instance_t* release();
        void                  invalidate();
    };

    static string_t::instance_t s_default_item;
    static string_t::data_t     s_default_data;
    static uchar16              s_default_str[4] = {0, 0, 0, 0};

    static inline bool              s_is_default_data(string_t::data_t* data) { return data == &s_default_data; }
    static inline string_t::data_t* s_get_default_data() { return &s_default_data; }

    static string_t::data_t* s_alloc_data(s32 strlen)
    {
        string_t::data_t* data = (string_t::data_t*)string_memory_t::s_object_alloc->allocate(sizeof(string_t::data_t));
        data->m_ref            = 0;

        utf16::prune strdata = (utf16::prune)string_memory_t::s_string_alloc->allocate(strlen);
        data->m_len          = strlen;
        data->m_ptr          = strdata;
        data->m_head         = nullptr;

        return data;
    }

    static void s_resize_data(string_t::data_t* data, u32 new_size)
    {
        if (data->m_len < new_size)
        {
            utf16::prune newptr = (utf16::prune)string_memory_t::s_string_alloc->allocate(new_size * sizeof(uchar16) + 1);
            for (s32 i = 0; i < data->m_len; i++)
                newptr[i] = data->m_ptr[i];
            string_memory_t::s_string_alloc->deallocate(data->m_ptr);
            data->m_ptr           = newptr;
            data->m_len           = new_size;
            data->m_ptr[new_size] = '\0';
        }
    }

    static string_t::data_t* s_unique_data(string_t::data_t* data, u32 from, u32 to)
    {
        ASSERT(from <= to);
        const u32         len     = to - from;
        string_t::data_t* newdata = (string_t::data_t*)string_memory_t::s_object_alloc->allocate(len);
        utf16::prune      newptr  = (utf16::prune)string_memory_t::s_string_alloc->allocate(len * sizeof(uchar16) + 1);
        newdata->m_ptr            = newptr;
        newdata->m_head           = nullptr;
        newdata->m_len            = len;
        newdata->m_ref            = 0;

        for (u32 i = 0; i < len; i++)
            newptr[i] = data->m_ptr[from + i];

        return newdata;
    }

    static inline bool           s_is_default_instance(string_t::instance_t* item) { return item == &s_default_item; }
    static string_t::instance_t* s_get_default_instance()
    {
        static string_t::instance_t* s_default_item_ptr = nullptr;
        if (s_default_item_ptr == nullptr)
        {
            s_default_item_ptr = &s_default_item;

            s_default_data.m_ptr  = (utf16::prune)s_default_str;
            s_default_data.m_len  = 0;
            s_default_data.m_ref  = 0;
            s_default_data.m_head = s_default_item_ptr;

            s_default_item.m_data  = s_get_default_data();
            s_default_item.m_range = {0, 0};
            s_default_item.m_next  = s_default_item_ptr;
        }
        return s_default_item_ptr;
    }

    static string_t::instance_t* s_alloc_instance(string_t::range_t range, string_t::data_t* data)
    {
        if (data == nullptr)
            data = s_get_default_data();

        string_t::instance_t* v = (string_t::instance_t*)string_memory_t::s_object_alloc->allocate(sizeof(string_t::instance_t));
        v->m_range              = range;
        v->m_data               = data->attach();
        return v;
    }

    static bool s_is_view_of(string_t::instance_t const* parent, string_t::instance_t const* slice) { return (parent->m_data == slice->m_data) && (slice->m_range.is_inside(parent->m_range)); }

    static bool s_narrow_view(string_t::instance_t* v, s32 move)
    {
        if (v->m_range.size() > 0)
        {
            if (move < 0)  // if negative then narrow the left side
            {
                move = -move;
                if (move <= v->m_range.size())
                {
                    v->m_range.m_from += move;
                    return true;
                }
            }
            else  // if positive then narrow the right side
            {
                if (move <= v->m_range.size())
                {
                    v->m_range.m_to -= move;
                    return true;
                }
            }
        }
        return false;
    }

    static bool s_move_view(string_t::instance_t const* str, string_t::range_t& view, s32 move)
    {
        s32 const from = view.m_from + move;

        // Check if the move doesn't result in an out-of-bounds
        if (from < str->m_range.m_from || from > str->m_range.m_to)
            return false;

        // Check if the movement doesn't invalidate this view
        s32 const to = view.m_to + move;
        if (to < str->m_range.m_from || to > str->m_range.m_to)
            return false;

        // Movement is ok, new view is valid
        view.m_from = from;
        view.m_to   = to;
        return true;
    }

    static void s_insert_space(uchar16* str, s32 strlen, s32 pos, s32 len)
    {
        // insert space, need to start at the end to avoid overwriting
        s32       src = strlen;
        s32       dst = strlen + len;
        s32 const end = pos + len;
        while (src > end)
        {
            str[--dst] = str[--src];
        }
    }

    static void s_remove_space(uchar16* str, s32 strlen, s32 pos, s32 len)
    {
        uchar16*       dst = str + pos;
        const uchar16* src = dst + len;
        const uchar16* end = str + strlen;
        while (src < end)
            *dst++ = *src++;
        *dst = '\0';
    }

    // forward declare
    static void s_adjust_active_views(string_t::instance_t* list, s32 op_code, s32 op_range_from, s32 op_range_to);

    static const s32 REMOVAL   = 0;
    static const s32 INSERTION = 1;
    static const s32 CLEARED   = 2;
    static const s32 RELEASED  = 3;

    static void s_string_insert(string_t::instance_t* str, string_t::range_t location, string_t::instance_t const* insert)
    {
        if (insert->is_empty())
            return;

        s32 const len = location.size();
        s32       pos = location.m_from;
        if (len < insert->size())
        {
            // The string to insert is larger than the selection, so we have to insert some
            // space into the string.
            s_resize_data(str->m_data, str->m_data->m_len + (insert->size() - len));
            s_insert_space(str->m_data->m_ptr, str->m_data->m_len, pos, insert->size() - len);
            s_adjust_active_views(str, INSERTION, pos, pos + insert->size() - len);
        }
        else if (len > insert->size())
        {
            // The string to insert is smaller than the selection, so we have to remove some
            // space from the string.
            s_remove_space(str->m_data->m_ptr, str->m_data->m_len, pos, len - insert->size());
            s_adjust_active_views(str, REMOVAL, pos, pos + len - insert->size());
        }
        else
        {
            // The string to insert is the same size as the selection, so we can just replace the
            // selection with the new string.
        }

        s32            src         = 0;
        uchar16 const* insert_data = insert->m_data->m_ptr + insert->m_range.m_from;
        uchar16*       str_data    = str->m_data->m_ptr + str->m_range.m_from;
        while (src < insert->size())
        {
            uchar32 const c = insert_data[src];
            str_data[pos]   = c;
            ++src;
            ++pos;
        }
    }

    static void string_remove(string_t::instance_t* str, string_t::range_t selection)
    {
        if (selection.is_empty())
            return;
        {
            //@TODO: it should be better to get an actual full view from the list of strings, currently we
            //       take the easy way and just take the whole allocated size as the full
            s_remove_space(str->m_data->m_ptr, str->m_data->m_len, selection.m_from, selection.size());

            // TODO: Decision to shrink the allocated memory of m_runes ?
            s_adjust_active_views(str, REMOVAL, selection.m_from, selection.m_to);
        }
    }

    static string_t::range_t s_find(string_t::instance_t* str, const string_t::instance_t* find)
    {
        if (find->is_empty() == false)
        {
            u32 const      strsize  = str->m_range.size();
            u32 const      findsize = find->m_range.size();
            uchar16 const* strdata  = str->m_data->m_ptr + str->m_range.m_from;
            uchar16 const* finddata = find->m_data->m_ptr + find->m_range.m_from;

            u32 i = 0;
            while (i < strsize)
            {
                if (strdata[i] == finddata[0])
                {
                    u32 j = 1;
                    while (j < findsize)
                    {
                        if (strdata[i + j] != finddata[j])
                            goto continue_search;
                        j++;
                    }
                    return {(u32)i, (u32)i + findsize};
                }
            continue_search:
                i++;
            }
        }
        return {0, 0};
    }

    static bool s_find_replace(string_t::instance_t* str, const string_t::instance_t* find, const string_t::instance_t* replace)
    {
        string_t::range_t remove = s_find(str, find);
        if (remove.is_empty() == false)
        {
            u32 const remove_from = remove.m_from;
            u32 const remove_len  = remove.size();
            s32 const diff        = remove_len - replace->size();
            if (diff > 0)  // The string to replace the selection with is smaller, so we have to remove some space from the string.
            {
                s_remove_space(str->m_data->m_ptr, str->m_data->m_len, remove_from, diff);
                // TODO: Decision to shrink the allocated memory of runes ?
                s_adjust_active_views(str, REMOVAL, remove_from, remove_from + diff);
            }
            else if (diff < 0)  // The string to replace the selection with is longer, so we have to insert some space into the string.
            {
                s_resize_data(str->m_data, str->size() + (-diff));
                s_insert_space(str->m_data->m_ptr, str->m_data->m_len, remove_from, -diff);
                s_adjust_active_views(str, INSERTION, remove_from, remove_from + -diff);
            }

            // Copy string 'remove' into the (now) same size selection space
            s32            src          = 0;
            s32            dst          = remove_from;
            s32 const      end          = remove_from + replace->size();
            utf16::prune   pdst         = (utf16::prune)str->m_data->m_ptr;
            uchar16 const* replace_data = replace->m_data->m_ptr + replace->m_range.m_from;
            while (src < replace->size())
            {
                pdst[dst++] = replace_data[src++];
            }
            return true;
        }
        return false;
    }

    static bool s_contains(const string_t::instance_t* str, uchar32 find)
    {
        u32            len     = str->size();
        u32            i       = 0;
        uchar16 const* strdata = str->m_data->m_ptr + str->m_range.m_from;
        while (i < len)
        {
            uchar32 const c = strdata[i];
            if (c == find)
                return true;
        }
        return false;
    }

    static void s_remove_any(string_t::instance_t* str, const string_t::instance_t* any)
    {
        // Remove any of the characters in @charset from @str
        u32 const strfrom = str->m_range.m_from;
        u32 const strto   = str->m_range.m_to;
        u32 const strsize = str->m_range.m_to - str->m_range.m_from;

        s32      d       = 0;
        s32      i       = 0;
        s32      r       = -1;
        uchar16* strdata = str->m_data->m_ptr + strfrom;
        while (i < strsize)
        {
            uchar16 const c = strdata[i];
            if (s_contains(any, c))
            {
                if (r == -1)
                    r = i;
                i++;
            }
            else
            {
                if (r >= 0)
                {
                    // This might not be the first character(s)/range removed, if not then the views already
                    // have been adjusted according to the previous range removal. So here we have to adjust
                    // this removal range by shifting it left with 'gap'.
                    s32 const gap = r - d;
                    s_adjust_active_views(str, REMOVAL, r - gap, i - gap);
                    r = -1;
                }

                if (i > d)
                {
                    strdata[d] = c;
                }
                i++;
                d++;
            }
        }

        s32 const l = i - d;
        if (l > 0)
        {
            // We still need to move the rest of the string that is not in this view but is part of
            // of the full string.
            uchar16* strdata = str->m_data->m_ptr + strfrom;
            while (i < str->m_data->m_len)
            {
                uchar32 const c = strdata[i];
                strdata[d]      = c;
                i++;
                d++;
            }

            str->m_data->m_len -= l;
            utf16::prune pdst        = (utf16::prune)str->m_data->m_ptr;
            pdst[str->m_data->m_len] = '\0';
        }
    }

    static const s32 NONE     = 0;
    static const s32 LEFT     = 0x0800;  // binary(0000,1000,0000,0000);
    static const s32 RIGHT    = 0x0010;  // binary(0000,0000,0001,0000);
    static const s32 INSIDE   = 0x0180;  // binary(0000,0001,1000,0000);
    static const s32 MATCH    = 0x03C0;  // binary(0000,0011,1100,0000);
    static const s32 OVERLAP  = 0x07E0;  // binary(0000,0111,1110,0000);
    static const s32 ENVELOPE = 0x37EC;  // binary(0011,0111,1110,1100);

    static s32 s_compare(s32 lhs_from, s32 lhs_to, s32 rhs_from, s32 rhs_to)
    {
        // Return where 'rhs' is in relation to 'lhs'
        // --------| lhs |--------[ rhs ]--------        RIGHT
        // --------[ rhs ]--------| lhs |--------        LEFT
        // --------[ rhs ]          lhs |--------        LEFT INSIDE
        // --------|     lhs      [ rhs ]--------        RIGHT INSIDE
        // --------|    [ rhs ]     lhs |--------        INSIDE
        // --------[     rhs  lhs       ]--------        MATCH
        // --------[ rhs    |    ]  lhs |--------        LEFT OVERLAP
        // --------| lhs    [    |  rhs ]--------        RIGHT OVERLAP
        // --------[    | lhs |     rhs ]--------        ENVELOPE
        // --------[ lhs |          rhs ]--------        LEFT ENVELOPE
        // --------[ rhs         |  lhs ]--------        RIGHT ENVELOPE
        if (lhs_to <= rhs_from)
            return RIGHT;
        else if (lhs_from >= rhs_to)
            return LEFT;
        else if (lhs_from == rhs_from && lhs_to > rhs_to)
            return LEFT | INSIDE;
        else if (lhs_from < rhs_from && lhs_to == rhs_to)
            return RIGHT | INSIDE;
        else if (lhs_from < rhs_from && lhs_to > rhs_to)
            return INSIDE;
        else if (lhs_from == rhs_from && lhs_to == rhs_to)
            return MATCH;
        else if (rhs_from < lhs_from && rhs_to < lhs_to)
            return LEFT | OVERLAP;
        else if (rhs_from > lhs_from && rhs_to > lhs_to)
            return RIGHT | OVERLAP;
        else if (lhs_from == rhs_from && lhs_to < rhs_to)
            return LEFT | ENVELOPE;
        else if (rhs_from < lhs_from && lhs_to == rhs_to)
            return RIGHT | ENVELOPE;
        else if (rhs_from < lhs_from && rhs_to > lhs_to)
            return ENVELOPE;
        return NONE;
    }

    static inline s32 s_compute_range_overlap(s32 lhs_from, s32 lhs_to, s32 rhs_from, s32 rhs_to)
    {
        if (rhs_from < lhs_from && rhs_to < lhs_to)
            return rhs_to - lhs_from;
        else if (rhs_from > lhs_from && rhs_to > lhs_to)
            return lhs_to - rhs_from;
        return 0;
    }

    // When the string is modified views can become invalid, here we try to keep them
    // 'correct' as much as we can. For example when we insert text into the string we
    // can correct all of the existing views.
    // Removal of text from a string may invalidate views that intersect with the range
    // of text that is removed.
    // Clear and Reset will invalidate all views on the string.
    static void s_adjust_active_view(string_t::instance_t* v, s32 op_code, s32 rhs_from, s32 rhs_to)
    {
        s32 lhs_from = v->m_range.m_from;
        s32 lhs_to   = v->m_range.m_to;

        switch (op_code)
        {
            case REMOVAL:
            {
                // --------| lhs |--------[ rhs ]--------        RIGHT = NOTHING
                // --------[ rhs ]--------| lhs |--------        LEFT = SHIFT LEFT rhs.size()
                // --------|    [ rhs ]     lhs |--------        INSIDE = NARROW RIGHT by rhs.size()
                // --------[ rhs ]          lhs |--------        LEFT INSIDE = NARROW RIGHT by rhs.size()
                // --------|     lhs      [ rhs ]--------        RIGHT INSIDE = NARROW RIGHT by rhs.size()
                // --------[     rhs  lhs       ]--------        MATCH = INVALIDATE
                // --------[ rhs    |    ]  lhs |--------        LEFT OVERLAP = NARROW LEFT by overlap & SHIFT LEFT by rhs.size()-overlap
                // --------| lhs    [    |  rhs ]--------        RIGHT OVERLAP = NARROW RIGHT by overlap
                // --------[    | lhs |     rhs ]--------        ENVELOPE = INVALIDATE
                // --------[ lhs |          rhs ]--------        LEFT ENVELOPE = INVALIDATE
                // --------[ rhs         |  lhs ]--------        RIGHT ENVELOPE = INVALIDATE
                s32 const c = s_compare(lhs_from, lhs_to, rhs_from, rhs_to);
                switch (c)
                {
                    case LEFT:
                        lhs_from -= (rhs_to - rhs_from);
                        lhs_to -= (rhs_to - rhs_from);
                        break;
                    case INSIDE:
                    case LEFT | INSIDE:
                    case RIGHT | INSIDE: lhs_to -= (rhs_to - rhs_from); break;
                    case MATCH:
                    case ENVELOPE:
                    case LEFT | ENVELOPE:
                    case RIGHT | ENVELOPE: lhs_from = lhs_to = 0; break;
                    case LEFT | OVERLAP:
                    {
                        s32 const overlap = s_compute_range_overlap(lhs_from, lhs_to, rhs_from, rhs_to);
                        lhs_from += overlap;
                        lhs_to -= ((rhs_to - rhs_from) - overlap);
                        lhs_from -= ((rhs_to - rhs_from) - overlap);
                    }
                    break;
                    case RIGHT | OVERLAP: lhs_to -= s_compute_range_overlap(lhs_from, lhs_to, rhs_from, rhs_to); break;
                    case RIGHT: break;
                }
            }
            break;

            case INSERTION:
            {
                // Insertion happens at rhs.from
                // --------| lhs |--------[ rhs ]--------        RIGHT = DO NOTHING
                // --------[     rhs  lhs       ]--------        MATCH = SHIFT RIGHT by rhs.size()
                // --------[ rhs ]--------| lhs |--------        LEFT = SHIFT RIGHT by rhs.size()
                // --------[    | lhs |     rhs ]--------        ENVELOPE = SHIFT RIGHT by rhs.size()
                // --------[ lhs |          rhs ]--------        LEFT ENVELOPE = SHIFT RIGHT by rhs.size()
                // --------[ rhs         |  lhs ]--------        RIGHT ENVELOPE = SHIFT RIGHT by rhs.size()
                // --------[ rhs    |    ]  lhs |--------        LEFT OVERLAP = SHIFT RIGHT by rhs.size()
                // --------[ rhs ]          lhs |--------        LEFT INSIDE = SHIFT RIGHT by rhs.size()
                // --------|    [ rhs ]     lhs |--------        INSIDE = EXTEND RIGHT by rhs.size()
                // --------|     lhs      [ rhs ]--------        RIGHT INSIDE = EXTEND RIGHT by rhs.size()
                // --------| lhs    [    |  rhs ]--------        RIGHT OVERLAP = EXTEND RIGHT by rhs.size()
                s32 const c = s_compare(lhs_from, lhs_to, rhs_from, rhs_to);
                switch (c)
                {
                    case RIGHT: break;
                    case MATCH:
                    case LEFT:
                    case ENVELOPE:
                    case (LEFT | ENVELOPE):
                    case (RIGHT | ENVELOPE):
                    case (LEFT | OVERLAP):
                    case (LEFT | INSIDE):
                        lhs_to += (rhs_to - rhs_from);
                        lhs_from += (rhs_to - rhs_from);
                        break;
                    case INSIDE:
                    case (RIGHT | INSIDE):
                    case (RIGHT | OVERLAP): lhs_to += (rhs_to - rhs_from); break;
                }
            }
            break;

            case CLEARED:
            case RELEASED:
            {
                lhs_from = lhs_to = 0;
            }
            break;
        }
    }

    static void s_adjust_active_views(string_t::instance_t* list, s32 op_code, s32 op_range_from, s32 op_range_to)
    {
        string_t::instance_t* iter = list;
        do
        {
            s_adjust_active_view(iter, op_code, op_range_from, op_range_to);
            iter = iter->m_next;
        } while (iter != list);
    }

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    string_t::data_t* string_t::data_t::detach()
    {
        if (m_ref == 1)
        {
            if (s_is_default_data(this))
                return this;

            string_memory_t::s_string_alloc->deallocate(m_ptr);
            string_memory_t::s_object_alloc->deallocate(this);
            return s_get_default_data();
        }
        m_ref--;
        return this;
    }

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    string_t::instance_t* string_t::instance_t::clone_full() const
    {
        u32 const             strlen = m_range.size();
        string_t::data_t*     data   = s_unique_data(m_data, m_range.m_from, m_range.m_to);
        string_t::instance_t* v      = s_alloc_instance({0, strlen}, data);
        return v;
    }

    string_t::instance_t* string_t::instance_t::clone_slice() const
    {
        string_t::instance_t* v = s_alloc_instance(m_range, m_data->attach());
        return v;
    }

    void string_t::instance_t::add(string_t::instance_t* node)
    {
        if (node != nullptr)
        {
            node->m_next = m_next;
            m_next       = node;
        }
        else
        {
            m_next = nullptr;
        }
    }

    void string_t::instance_t::rem()
    {
        string_t::instance_t* head = m_data->m_head;
        if (head == this)
        {
            m_data->m_head = head->m_next;
        }
        else
        {
            while (head->m_next != this)
                head = head->m_next;
            head->m_next = m_next;
        }
    }

    string_t::instance_t* string_t::instance_t::release()
    {
        if (!s_is_default_instance(this))
        {
            if (!s_is_default_data(m_data))
            {
                rem();
                m_data->detach();
            }
            string_memory_t::s_object_alloc->deallocate(this);
        }
        return s_get_default_instance();
    }

    void string_t::instance_t::invalidate()
    {
        if (!s_is_default_data(m_data))
        {
            rem();
            m_data->detach();
            m_data = s_get_default_data();
        }
        m_range = {0, 0};
    }

    //------------------------------------------------------------------------------
    //------------ range -----------------------------------------------------------
    //------------------------------------------------------------------------------
    class string_functions_t : public string_t
    {
    public:
        // select parst of the string, return local view range
        static range_t selectBeforeLocal(const instance_t* str, const instance_t* sel) { return {str->m_range.m_from - str->m_range.m_from, sel->m_range.m_from - str->m_range.m_from}; }
        static range_t selectBeforeIncludedLocal(const instance_t* str, const instance_t* sel) { return {str->m_range.m_from - str->m_range.m_from, sel->m_range.m_to - str->m_range.m_from}; }
        static range_t selectAfterLocal(const instance_t* str, const instance_t* sel) { return {sel->m_range.m_to - str->m_range.m_from, str->m_range.m_to - str->m_range.m_from}; }
        static range_t selectAfterIncludedLocal(const instance_t* str, const instance_t* sel) { return {sel->m_range.m_from - str->m_range.m_from, str->m_range.m_to - str->m_range.m_from}; }

        static s32 compare(const string_t::instance_t* lhs, string_t::range_t const& lhsview, const string_t::instance_t* rhs)
        {
            ASSERT(lhsview.is_inside(lhs->m_range));

            if (lhsview.size() < rhs->size())
                return -1;
            if (lhsview.size() > rhs->size())
                return 1;
            uchar16 const* lhsdata = lhs->m_data->m_ptr + lhs->m_range.m_from;
            uchar16 const* rhsdata = rhs->m_data->m_ptr + rhs->m_range.m_from;
            uchar16 const* lhsend   = lhsdata + lhsview.size();
            while (lhsdata < lhsend)
            {
                uchar32 const lc = *lhsdata++;
                uchar32 const rc = *rhsdata++;
                if (lc < rc)
                    return -1;
                else if (lc > rc)
                    return 1;
            }
            return 0;
        }

        static bool isEqual(const string_t::instance_t* lhs, string_t::range_t const& lhsview, const string_t::instance_t* rhs)
        {
            return compare(lhs, lhsview, rhs) == 0;
        }

        static string_t::range_t findCharUntil(const string_t::instance_t* str, uchar32 find)
        {
            uchar16 const* strdata = str->m_data->m_ptr + str->m_range.m_from;
            for (u32 i = 0; i < str->size(); i++)
            {
                uchar32 const c = strdata[i];
                if (c == find)
                {
                    return {(u32)0, i};
                }
            }
            return {0, 0};
        }

        static string_t::range_t findStrUntil(const string_t::instance_t* str, const string_t::instance_t* find)
        {
            string_t::range_t v = {0, 0};
            v.m_to              = find->size();

            while (!v.is_empty())
            {
                if (string_functions_t::isEqual(str, v, find))
                {
                    // So here we have a view with the size of the @find string on
                    // string @str that matches the string @find and we need to return
                    // a string view that exist before view @v.
                    return {0, v.m_from};
                }
                if (!s_move_view(str, v, 1))
                    break;
            }
            return {0, 0};
        }

        static string_t::range_t findCharUntilLast(const string_t::instance_t* str, uchar32 find)
        {
            uchar16 const* strdata = str->m_data->m_ptr + str->m_range.m_from;
            for (u32 i = str->size() - 1; i >= 0; --i)
            {
                uchar32 const c = strdata[i];
                if (c == find)
                {
                    return {str->m_range.m_from, str->m_range.m_from + i};
                }
            }
            return {0, 0};
        }

        static string_t::range_t findStrUntilLast(const string_t::instance_t* str, const string_t::instance_t* find)
        {
            string_t::range_t v = {0, 0};
            v.m_from            = str->size() - find->size();
            v.m_to              = str->size();

            while (!v.is_empty())
            {
                if (isEqual(str, v, find))
                {
                    return {0, v.m_from};
                }
                if (!s_move_view(str, v, -1))
                    break;
            }
            return {0, 0};
        }
    };

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------

    string_t::string_t() { m_item = s_get_default_instance(); }

    string_t::string_t(const char* str)
    {
        crunes_t srcrunes(str);

        u32 const strlen = srcrunes.size();

        u32 from = 0;
        u32 to   = strlen;

        if (strlen > 0)
        {
            string_t::data_t* data = s_alloc_data(strlen);
            m_item                 = s_alloc_instance({from, to}, data);
            while (*str != '\0')
                *data->m_ptr++ = *str++;
            *data->m_ptr = '\0';
        }
        else
        {
            m_item = s_get_default_instance();
        }
    }

    string_t::string_t(s32 _len)
        : m_item(nullptr)
    {
        const s32 strlen = _len;

        if (strlen > 0)
        {
            string_t::data_t* data = s_alloc_data(strlen);
            m_item                 = s_alloc_instance({0, 0}, data);
        }
        else
        {
            m_item = s_get_default_instance();
        }
    }

    string_t::string_t(const string_t& other) { m_item = other.m_item->clone_full(); }

    string_t::string_t(const string_t& left, const string_t& right)
    {
        const u32 strlen = left.size() + right.size();

        string_t::data_t* data = s_alloc_data(strlen);
        m_item                 = s_alloc_instance({0, strlen}, data);

        // manually copy the left and right strings into the new string
        uchar16* src = left.m_item->m_data->m_ptr + left.m_item->m_range.m_from;
        uchar16* dst = m_item->m_data->m_ptr;
        for (u32 i = 0; i < left.size(); i++)
            *dst++ = *src++;
        src = right.m_item->m_data->m_ptr + right.m_item->m_range.m_from;
        for (u32 i = 0; i < right.size(); i++)
            *dst++ = *src++;

        m_item->m_range.m_to = strlen;
    }

    string_t::string_t(string_t::instance_t* instance)
    {
        m_item = instance->clone_slice();
        instance->add(m_item);
    }

    string_t::string_t(string_t::instance_t* instance, const string_t::range_t& range)
    {
        m_item = instance->clone_slice();
        instance->add(m_item);
        m_item->m_range.m_from += range.m_from;
        m_item->m_range.m_to = m_item->m_range.m_from + range.size();
    }

    string_t::~string_t() { release(); }

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    s32  string_t::size() const { return m_item->size(); }
    s32  string_t::cap() const { return m_item->cap(); }
    bool string_t::is_empty() const { return m_item->is_empty(); }
    bool string_t::is_slice() const { return m_item->is_slice(); }

    void string_t::clear() { release(); }

    string_t string_t::slice() const { return string_t(m_item); }

    string_t string_t::clone() const { return string_t(m_item->clone_full()); }

    string_t string_t::operator()(s32 _from, s32 _to) const
    {
        math::sort(_from, _to);
        const u32 from = math::min(m_item->m_range.m_from + _from, m_item->m_range.m_to);
        const u32 to   = math::min(m_item->m_range.m_from + _to, m_item->m_range.m_to);
        return string_t(m_item, {from, to});
    }

    uchar32 string_t::operator[](s32 index) const
    {
        if (index >= size())
            return '\0';
        uchar16 const* str = m_item->m_data->m_ptr + m_item->m_range.m_from;
        return str[index];
    }

    string_t& string_t::operator=(const char* other)
    {
        u32 strlen = 0;
        while (other[strlen] != '\0')
            ++strlen;

        release();

        if (strlen != 0)
        {
            string_t::data_t*     data = s_alloc_data(strlen);
            string_t::instance_t* item = s_alloc_instance({0, strlen}, data);
            uchar16*              dst  = data->m_ptr;
            uchar16 const*        end  = dst + strlen;
            while (dst < end)
                *dst++ = *other++;
            *dst                   = '\0';
            m_item                 = item;
            m_item->m_range.m_from = 0;
            m_item->m_range.m_to   = strlen;
        }
        else
        {
            m_item = s_get_default_instance();
        }

        return *this;
    }

    string_t& string_t::operator=(const string_t& other)
    {
        if (this->m_item->m_data == other.m_item->m_data)
        {
            m_item->m_range.m_from = other.m_item->m_range.m_from;
            m_item->m_range.m_to   = other.m_item->m_range.m_to;
        }
        else
        {
            string_t::data_t* data = other.m_item->m_data->attach();
            m_item->m_data         = m_item->m_data->detach();
            m_item->m_data         = data;
            m_item->m_range.m_from = other.m_item->m_range.m_from;
            m_item->m_range.m_to   = other.m_item->m_range.m_to;
        }
        return *this;
    }

    string_t& string_t::operator+=(const string_t& other)
    {
        concatenate(other);
        return *this;
    }

    bool string_t::operator==(const string_t& other) const
    {
        return string_functions_t::isEqual(m_item, m_item->m_range, other.m_item);
    }

    bool string_t::operator!=(const string_t& other) const
    {
        return !string_functions_t::isEqual(m_item, m_item->m_range, other.m_item);
    }

    void string_t::release() { m_item = m_item->release(); }

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    string_t string_t::select(u32 from, u32 to) const
    {
        string_t::instance_t* item = m_item->clone_slice();
        item->m_range.m_from = m_item->m_range.m_from + from;
        item->m_range.m_to   = m_item->m_range.m_from + to;
        return string_t(item);
    }

    string_t string_t::selectUntil(uchar32 find) const
    {
        string_t::range_t view = string_functions_t::findCharUntil(m_item, find);
        if (view.is_empty())
            return string_t(s_get_default_instance());
        return select(view.m_from, view.m_to);
    }

    string_t string_t::selectUntil(const string_t& selection) const
    {
        string_t::range_t view = string_functions_t::selectBeforeLocal(m_item, selection.m_item);
        if (view.is_empty())
            return string_t(s_get_default_instance());
        return select(view.m_from, view.m_to);
    }

    string_t string_t::selectUntilIncluded(const string_t& find) const
    {
        string_t::range_t view = string_functions_t::selectBeforeIncludedLocal(m_item, find.m_item);
        if (view.is_empty())
            return string_t(s_get_default_instance());
        return select(view.m_from, view.m_to + find.size());
    }

    string_t string_t::selectUntilEndExcludeSelection(const string_t& selection) const
    {
        string_t::range_t range = string_functions_t::selectAfterLocal(m_item, selection.m_item);
        return string_t(m_item, range);
    }
    string_t string_t::selectUntilEndIncludeSelection(const string_t& selection) const
    {
        string_t::range_t range = string_functions_t::selectAfterIncludedLocal(m_item, selection.m_item);
        return string_t(m_item, range);
    }

    bool string_t::isUpper() const
    {
        uchar16 const* strdata = m_item->m_data->m_ptr + m_item->m_range.m_from;
        for (s32 i = 0; i < size(); i++)
        {
            uchar32 const c = strdata[i];
            if (nrunes::is_lower(c))
                return false;
        }
        return true;
    }

    bool string_t::isLower() const
    {
        uchar16 const* strdata = m_item->m_data->m_ptr + m_item->m_range.m_from;
        for (s32 i = 0; i < size(); i++)
        {
            uchar32 const c = strdata[i];
            if (nrunes::is_upper(c))
                return false;
        }
        return true;
    }

    bool string_t::isCapitalized() const
    {
        s32            i       = 0;
        uchar16 const* strdata = m_item->m_data->m_ptr + m_item->m_range.m_from;
        while (i < size())
        {
            uchar32 c = '\0';
            while (i < size())
            {
                c = strdata[i];
                if (!nrunes::is_space(c))
                    break;
                i += 1;
            }

            if (nrunes::is_upper(c))
            {
                i += 1;
                while (i < size())
                {
                    c = strdata[i];
                    if (nrunes::is_space(c))
                        break;
                    if (nrunes::is_upper(c))
                        return false;
                    i += 1;
                }
            }
            else if (nrunes::is_alpha(c))
            {
                return false;
            }
            i += 1;
        }
        return true;
    }

    bool string_t::isQuoted() const { return isQuoted('"'); }
    bool string_t::isQuoted(uchar32 inQuote) const { return isDelimited(inQuote, inQuote); }

    bool string_t::isDelimited(uchar32 inLeft, uchar32 inRight) const
    {
        if (is_empty())
            return false;
        s32 const first = 0;
        s32 const last  = size() - 1;
        return (firstChar() == inLeft && lastChar() == inRight);
    }

    uchar32 string_t::firstChar() const
    {
        s32 const      first   = 0;
        uchar16 const* strdata = m_item->m_data->m_ptr + m_item->m_range.m_from;
        return strdata[first];
    }

    uchar32 string_t::lastChar() const
    {
        s32 const      last    = size() - 1;
        uchar16 const* strdata = m_item->m_data->m_ptr + m_item->m_range.m_from;
        return strdata[last];
    }

    bool string_t::startsWith(const string_t& start) const
    {
        string_t v = select(0, start.size());
        if (!v.is_empty())
            return (v == start);
        return false;
    }

    bool string_t::endsWith(const string_t& end) const
    {
        string_t v = select(size() - end.size(), size());
        if (!v.is_empty())
            return (v == end);
        return false;
    }

    string_t string_t::find(uchar32 find) const
    {
        uchar16 const* strdata = m_item->m_data->m_ptr + m_item->m_range.m_from;
        for (s32 i = 0; i < size(); i++)
        {
            uchar32 const c = strdata[i];
            if (c == find)
                return select(i, i + 1);
        }
        return string_t(s_get_default_instance());
    }

    string_t string_t::find(const char* inFind) const
    {
        uchar16 const* str = m_item->m_data->m_ptr + m_item->m_range.m_from;
        for (s32 i = 0; i < size(); i++)
        {
            uchar32 const c = str[i];
            if (c == *inFind)
            {
                s32 s = 1;
                for (s32 j = i + 1; j < size(); j++, ++s)
                {
                    if (str[m_item->m_range.m_from + j] != inFind[s])
                        break;
                }
                if (inFind[s] == '\0')
                {
                    // match
                    return select(i, i + (s - 1));
                }
            }
        }
        return string_t(s_get_default_instance());
    }

    string_t string_t::find(const string_t& find) const
    {
        string_t::range_t v = {0, 0};
        v.m_to              = find.size();

        while (!v.is_empty())
        {
            if (string_functions_t::isEqual(m_item, v, find.m_item))
            {
                // So here we have a view with the size of the @find string on
                // string @str that matches the string @find.
                return select(v.m_from, v.m_to);
            }
            if (!s_move_view(m_item, v, 1))
                break;
        }
        return string_t(s_get_default_instance());
    }

    string_t string_t::findLast(const string_t& find) const
    {
        string_t::range_t v = {0, 0};
        v.m_from            = size() - find.size();
        v.m_to              = size();

        while (!v.is_empty())
        {
            if (string_functions_t::isEqual(m_item, v, find.m_item))
            {
                // So here we have a view with the size of the @find string on
                // string @str that matches the string @find.
                return select(v.m_from, v.m_to);
            }
            if (!s_move_view(m_item, v, -1))
                break;
        }
        return string_t(s_get_default_instance());
    }

    string_t string_t::findOneOf(const string_t& charset) const
    {
        uchar16 const* strdata = m_item->m_data->m_ptr + m_item->m_range.m_from;
        for (s32 i = 0; i < size(); i++)
        {
            uchar32 const sc = strdata[i];
            if (charset.contains(sc))
            {
                return select(i, i + 1);
            }
        }
        return string_t(s_get_default_instance());
    }

    string_t string_t::findOneOfLast(const string_t& charset) const
    {
        uchar16 const* strdata = m_item->m_data->m_ptr + m_item->m_range.m_from;
        for (s32 i = size() - 1; i >= 0; --i)
        {
            uchar32 const sc = strdata[i];
            if (charset.contains(sc))
            {
                return select(i, i + 1);
            }
        }
        return string_t(s_get_default_instance());
    }

    s32 string_t::compare(const string_t& rhs) const { return string_functions_t::compare(m_item, m_item->m_range, rhs.m_item); }
    bool string_t::isEqual(const string_t& rhs) const { return string_functions_t::compare(m_item, m_item->m_range, rhs.m_item) == 0; }

    bool string_t::contains(const string_t& contains) const
    {
        string_t::range_t v = {0, 0};
        v.m_to              = contains.size();

        while (!v.is_empty())
        {
            if (string_functions_t::isEqual(m_item, v, contains.m_item))
            {
                return true;
            }
            if (!s_move_view(m_item, v, 1))
                break;
        }
        return false;
    }

    bool string_t::contains(uchar32 contains) const
    {
        uchar16 const* strdata = m_item->m_data->m_ptr + m_item->m_range.m_from;
        for (s32 i = 0; i < size(); i++)
        {
            uchar32 const sc = strdata[i];
            if (sc == contains)
            {
                return true;
            }
        }
        return false;
    }

    void string_t::concatenate(const string_t& con)
    {
        s32 const len = size() + con.size();
        s_resize_data(m_item->m_data, len);

        // manually append the incoming string to the end of the current string
        uchar16 const* src = con.m_item->m_data->m_ptr + con.m_item->m_range.m_from;
        uchar16*       dst = m_item->m_data->m_ptr + m_item->m_range.m_to;
        for (s32 i = 0; i < con.size(); i++)
            *dst++ = *src++;
        *dst = '\0';

        m_item->m_range.m_to = len;
    }

    void string_t::concatenate(const string_t& strA, const string_t& strB)
    {
        s32 const len = size() + strA.size() + strB.size();
        s_resize_data(m_item->m_data, len);

        // manually append the incoming strings to the end of the current string
        uchar16* dst = m_item->m_data->m_ptr + m_item->m_range.m_to;

        uchar16 const* srcA = strA.m_item->m_data->m_ptr + strA.m_item->m_range.m_from;
        for (s32 i = 0; i < strA.size(); i++)
            *dst++ = *srcA++;

        uchar16 const* srcB = strB.m_item->m_data->m_ptr + strB.m_item->m_range.m_from;
        for (s32 i = 0; i < strB.size(); i++)
            *dst++ = *srcB++;

        *dst = '\0';

        m_item->m_range.m_to = len;
    }

    void string_t::concatenate_repeat(const string_t& con, s32 ntimes)
    {
        s32 const len = size() + (con.size() * ntimes);
        s_resize_data(m_item->m_data, len);

        uchar16* dst = m_item->m_data->m_ptr + m_item->m_range.m_to;
        for (s32 i = 0; i < ntimes; ++i)
        {
            uchar16 const* src = con.m_item->m_data->m_ptr + con.m_item->m_range.m_from;
            for (s32 i = 0; i < con.size(); i++)
                *dst++ = *src++;
        }
        *dst                 = '\0';
        m_item->m_range.m_to = len;
    }

    s32 string_t::format(const string_t& format, const va_list_t& args)
    {
        release();

        s32 const   argc = args.argc();
        va_t const* argv = args.argv();
        crunes_t    fmt(format.m_item->m_data->m_ptr, format.m_item->m_range.m_from, format.m_item->m_range.m_to, format.m_item->m_data->m_len);
        const u32   len = cprintf_(fmt, argv, argc);

        string_t::data_t*     data = s_alloc_data(len);
        string_t::instance_t* item = s_alloc_instance({0, len}, data);

        runes_t str(item->m_data->m_ptr, item->m_range.m_from, item->m_range.m_to, item->m_data->m_len);
        sprintf_(str, fmt, argv, argc);
        item->m_range.m_to = str.m_utf16.m_end;

        m_item = item;
        return len;
    }

    s32 string_t::formatAdd(const string_t& format, const va_list_t& args)
    {
        s32 const   argc = args.argc();
        va_t const* argv = args.argv();
        crunes_t    fmt(format.m_item->m_data->m_ptr, format.m_item->m_range.m_from, format.m_item->m_range.m_to, format.m_item->m_data->m_len);
        const s32   len = cprintf_(fmt, argv, argc);
        s_resize_data(m_item->m_data, len);
        runes_t str(m_item->m_data->m_ptr, m_item->m_range.m_from, m_item->m_range.m_to, m_item->m_data->m_len);
        str.m_ascii.m_str = str.m_ascii.m_end;
        sprintf_(str, fmt, argv, argc);
        m_item->m_range.m_to = str.m_utf16.m_end;
        return len;
    }

    void string_t::insertReplaceSelection(const string_t& selection, const string_t& insert)
    {
        string_t::range_t range = selection.m_item->m_range;
        s_string_insert(m_item, range, insert.m_item);
    }

    void string_t::insertBeforeSelection(const string_t& selection, const string_t& insert)
    {
        string_t::range_t range = string_functions_t::selectAfterLocal(m_item, selection.m_item);
        range.m_from            = range.m_to;
        s_string_insert(m_item, range, insert.m_item);
    }

    void string_t::insertAfterSelection(const string_t& selection, const string_t& insert)
    {
        string_t::range_t range = string_functions_t::selectAfterLocal(m_item, selection.m_item);
        range.m_from            = range.m_to;
        s_string_insert(m_item, range, insert.m_item);
    }

    void string_t::removeSelection(const string_t& selection) { string_remove(m_item, selection.m_item->m_range); }

    s32 string_t::findRemove(const string_t& find, s32 ntimes)
    {
        string_t::range_t v = {0, 0};
        v.m_to              = find.size();

        for (s32 i = 0; i < ntimes; i++)
        {
            while (!v.is_empty())
            {
                if (string_functions_t::isEqual(m_item, v, find.m_item))
                {
                    // So here we have a view with the size of the @find string on
                    // string @str that matches the string @find.
                    // We need to remove this part from the string.
                    string_remove(m_item, v);
                    break;
                }
                else
                {
                    if (!s_move_view(m_item, v, 1))
                    {
                        return i + 1;  // did not find the string, so we are done
                    }
                }
            }
        }
        return ntimes;
    }

    s32 string_t::findReplace(const string_t& find, const string_t& replace, s32 ntimes)
    {
        for (s32 i = 0; i < ntimes; i++)
        {
            if (s_find_replace(m_item, find.m_item, replace.m_item) == 0)
                return i + 1;
        }
        return ntimes;
    }

    s32 string_t::removeChar(uchar32 c, s32 ntimes)
    {
        u32 n = ntimes;
        if (n == 0)
            n = size();

        uchar16* strdata = m_item->m_data->m_ptr + m_item->m_range.m_from;

        u32 i = 0;
        u32 p = 0;
        while (i < size())
        {
            if (n > 0 && strdata[i] == c)
            {
                --n;
                ++i;
                continue;
            }
            if (p < i)
            {
                strdata[p] = strdata[i];
            }
            ++p;
            ++i;
        }
        if (p < i)
            strdata[p] = '\0';

        // adjust the 'to' of the string
        m_item->m_range.m_to -= ntimes - n;

        return ntimes - n;
    }

    s32 string_t::removeAnyChar(const string_t& any, s32 ntimes)
    {
        u32 n = ntimes;
        if (n == 0)
            n = size();

        uchar16* strdata = m_item->m_data->m_ptr + m_item->m_range.m_from;

        u32 i = 0;
        u32 p = 0;
        while (i < size())
        {
            if (n > 0 && any.contains(strdata[i]))
            {
                --n;
                ++i;
                continue;
            }
            if (p < i)
            {
                strdata[p] = strdata[i];
            }
            ++p;
            ++i;
        }
        if (p < i)
            strdata[p] = '\0';

        // adjust the 'to' of the string
        m_item->m_range.m_to -= ntimes - n;

        return ntimes - n;
    }

    s32 string_t::replaceAnyChar(const string_t& any, uchar32 with, s32 ntimes)
    {
        // Replace any of the characters in @charset from @str with character @with
        uchar16* strdata = m_item->m_data->m_ptr + m_item->m_range.m_from;
        s32      n       = ntimes == 0 ? size() : ntimes;
        for (s32 i = 0; i < size(); ++i)
        {
            uchar32 const c = strdata[i];
            if (any.contains(c))
            {
                if (--n == 0)
                    break;
                strdata[i] = with;
            }
        }
        return n;
    }

    void string_t::toUpper()
    {
        uchar16* strdata = m_item->m_data->m_ptr + m_item->m_range.m_from;
        for (s32 i = 0; i < size(); i++)
        {
            uchar32 c  = strdata[i];
            c          = nrunes::to_upper(c);
            strdata[i] = c;
        }
    }

    void string_t::toLower()
    {
        uchar16* strdata = m_item->m_data->m_ptr + m_item->m_range.m_from;
        for (s32 i = 0; i < size(); i++)
        {
            uchar32 c  = strdata[i];
            c          = nrunes::to_lower(c);
            strdata[i] = c;
        }
    }

    void string_t::capitalize()
    {
        // Standard separator is ' '
        bool     prev_is_space = true;
        s32      i             = 0;
        uchar16* strdata       = m_item->m_data->m_ptr + m_item->m_range.m_from;
        while (i < size())
        {
            uchar32 c = strdata[i];
            uchar32 d = c;
            if (nrunes::is_alpha(c))
            {
                if (prev_is_space)
                {
                    c = nrunes::to_upper(c);
                }
                else
                {
                    c = nrunes::to_lower(c);
                }
            }
            else
            {
                prev_is_space = nrunes::is_space(c);
            }

            if (c != d)
            {
                strdata[i] = c;
            }
            i++;
        }
    }

    void string_t::capitalize(const string_t& separators)
    {
        bool     prev_is_space = false;
        s32      i             = 0;
        uchar16* strdata       = m_item->m_data->m_ptr + m_item->m_range.m_from;
        while (i < size())
        {
            uchar32 c = strdata[i];
            uchar32 d = c;
            if (nrunes::is_alpha(c))
            {
                if (prev_is_space)
                {
                    c = nrunes::to_upper(c);
                }
                else
                {
                    c = nrunes::to_lower(c);
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
                strdata[i] = c;
            }
            i++;
        }
    }

    // Trim does nothing more than narrowing the <from, to>, nothing is actually removed
    // from the actual underlying string string_data.
    void string_t::trim()
    {
        trimLeft();
        trimRight();
    }

    void string_t::trimLeft()
    {
        uchar16* strdata = m_item->m_data->m_ptr + m_item->m_range.m_from;
        for (s32 i = 0; i < size(); ++i)
        {
            uchar32 c = strdata[i];
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
            {
                if (i > 0)
                {
                    s_narrow_view(m_item, -i);
                }
                return;
            }
        }
    }

    void string_t::trimRight()
    {
        s32 const last    = size() - 1;
        uchar16*  strdata = m_item->m_data->m_ptr + m_item->m_range.m_from;
        for (s32 i = 0; i < size(); ++i)
        {
            uchar32 c = strdata[last - i];
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
            {
                if (i > 0)
                {
                    s_narrow_view(m_item, i);
                }
                return;
            }
        }
    }

    void string_t::trim(uchar32 c)
    {
        trimLeft(c);
        trimRight(c);
    }

    void string_t::trimLeft(uchar32 c)
    {
        uchar16* strdata = m_item->m_data->m_ptr + m_item->m_range.m_from;
        for (s32 i = 0; i < size(); ++i)
        {
            uchar32 const r = strdata[i];
            if (c != r)
            {
                if (i > 0)
                {
                    s_narrow_view(m_item, -i);
                }
                return;
            }
        }
    }

    void string_t::trimRight(uchar32 c)
    {
        s32 const last    = size() - 1;
        uchar16*  strdata = m_item->m_data->m_ptr + m_item->m_range.m_from;
        for (s32 i = 0; i < size(); ++i)
        {
            uchar32 const r = strdata[last - i];
            if (c != r)
            {
                if (i > 0)
                {
                    s_narrow_view(m_item, i);
                }
                return;
            }
        }
    }

    void string_t::trim(const string_t& set)
    {
        trimLeft(set);
        trimRight(set);
    }

    void string_t::trimLeft(const string_t& set)
    {
        uchar16* strdata = m_item->m_data->m_ptr + m_item->m_range.m_from;
        for (s32 i = 0; i < size(); ++i)
        {
            uchar32 c = strdata[i];
            if (!set.contains(c))
            {
                if (i > 0)
                {
                    s_narrow_view(m_item, -i);
                }
                return;
            }
        }
    }

    void string_t::trimRight(const string_t& set)
    {
        s32 const last    = size() - 1;
        uchar16*  strdata = m_item->m_data->m_ptr + m_item->m_range.m_from;
        for (s32 i = 0; i < size(); ++i)
        {
            uchar32 c = strdata[last - i];
            if (!set.contains(c))
            {
                if (i > 0)
                {
                    s_narrow_view(m_item, i);
                }
                return;
            }
        }
    }

    void string_t::trimQuotes() { trimDelimiters('"', '"'); }
    void string_t::trimQuotes(uchar32 quote) { trimDelimiters(quote, quote); }

    void string_t::trimDelimiters(uchar32 left, uchar32 right)
    {
        trimLeft(left);
        trimRight(right);
    }

    void string_t::reverse()
    {
        s32 const last    = size() - 1;
        uchar16*  strdata = m_item->m_data->m_ptr + m_item->m_range.m_from;
        for (s32 i = 0; i < (last - i); ++i)
        {
            uchar32 l         = strdata[i];
            uchar32 r         = strdata[last - i];
            strdata[i]        = r;
            strdata[last - i] = l;
        }
    }

    bool string_t::selectBeforeAndAfter(const string_t& selection, string_t& outLeft, string_t& outRight) const
    {
        string_t::range_t range = string_functions_t::selectBeforeLocal(m_item, selection.m_item);
        if (range.is_empty())
            return false;
        outLeft  = select(range.m_from, range.m_to);
        outRight = select(range.m_to + selection.size(), size());
        return true;
    }

    bool string_t::findCharSelectBeforeAndAfter(uchar32 find, string_t& outLeft, string_t& outRight) const
    {
        string_t::range_t range = string_functions_t::findCharUntil(m_item, find);
        if (range.is_empty())
            return false;
        outLeft  = select(range.m_from, range.m_to);
        outRight = select(outLeft.size() + 1, size());
        return true;
    }

    bool string_t::findCharLastSelectBeforeAndAfter(uchar32 find, string_t& outLeft, string_t& outRight) const
    {
        string_t::range_t range = string_functions_t::findCharUntilLast(m_item, find);
        if (range.is_empty())
            return false;
        outLeft  = select(range.m_from, range.m_to);
        outRight = select(range.m_to + 1, size());
        return true;
    }

    bool string_t::findStrSelectBeforeAndAfter(const string_t& find, string_t& outLeft, string_t& outRight) const
    {
        string_t::range_t range = string_functions_t::findStrUntil(m_item, find.m_item);
        if (range.is_empty())
            return false;
        outLeft  = select(range.m_from, range.m_to);
        outRight = select(range.m_to + find.size(), size());
        return true;
    }

    bool string_t::findStrLastSelectBeforeAndAfter(const string_t& find, string_t& outLeft, string_t& outRight) const
    {
        string_t::range_t range = string_functions_t::findStrUntilLast(m_item, find.m_item);
        if (range.is_empty())
            return false;
        outLeft  = select(range.m_from, range.m_to);
        outRight = select(range.m_to + find.size(), size());
        return true;
    }

    void concatenate(string_t& str, const string_t& con) { s_resize_data(str.m_item->m_data, str.size() + con.size() + 1); }

    //------------------------------------------------------------------------------
    static void user_case_for_string()
    {
        string_t str("This is an ascii string that will be converted to UTF-32");

        string_t strvw  = str.slice();
        string_t ascii  = "ascii";
        string_t substr = strvw.find(ascii.slice());
        substr.toUpper();

        string_t converted("converted ");
        strvw.findRemove(converted.slice());
    }

}  // namespace ncore
