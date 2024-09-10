#include "cbase/c_allocator.h"
#include "cbase/c_context.h"
#include "cbase/c_integer.h"
#include "cbase/c_memory.h"
#include "cbase/c_printf.h"
#include "cbase/c_runes.h"
#include "cstring/c_string.h"

namespace ncore
{
    namespace nstring_memory
    {
        static alloc_t* s_object_alloc = nullptr;  // for instance_t and data_t, object-size = 32 bytes
        static alloc_t* s_string_alloc = nullptr;  // for the actual string data

        void init(alloc_t* object_alloc, alloc_t* string_alloc)
        {
            s_object_alloc = object_alloc;
            s_string_alloc = string_alloc;
        }
    };  // namespace nstring_memory

    // Strings are stored in memory in UTF-16 format the internal encoding is more correctly described as UCS-2.
    // All code here assumes 2 bytes is one codepoint so only the Basic Multilingual Plane (BMP) is supported.
    // Strings are stored in the endian-ness appropriate for the current platform.
    namespace nstring
    {
        struct range_t;
        struct data_t;

        struct data_t  // 24 bytes
        {
            ucs2::prune m_ptr;   // UCS-2
            instance_t* m_head;  // The first view of this string, doubly linked list of instances
            s32         m_len;
            s32         m_ref;

            inline s32 cap() const { return m_len; }

            data_t* attach()
            {
                m_ref++;
                return this;
            }

            data_t* detach();

            void addToList(instance_t* node);
            void remFromList(instance_t* node);

            static data_t s_default;
        };

        struct range_t
        {
            s32 m_from;
            s32 m_to;

            inline bool is_empty() const { return m_from == m_to; }
            inline s32  size() const
            {
                ASSERT(m_from <= m_to);
                return m_to - m_from;
            }
            inline void move_left()
            {
                --m_from;
                --m_to;
            }
            inline void move_right()
            {
                m_from++;
                m_to++;
            }
            inline bool    is_inside(range_t const& parent) const { return m_from >= parent.m_from && m_to <= parent.m_to; }
            inline range_t local() const { return {0, m_to - m_from}; }
        };

        struct instance_t  // 32 bytes
        {
            range_t     m_range;  // [from,to] view on the string data
            data_t*     m_data;   // reference counted string data
            instance_t* m_next;   // doubly linked list of instances that also own 'm_data'
            instance_t* m_prev;   // doubly linked list of instances that also own 'm_data'

            inline bool is_empty() const { return m_range.is_empty(); }
            inline bool is_slice() const { return m_data->m_head != nullptr; }
            inline s32  cap() const { return m_data->cap(); }
            inline s32  size() const { return m_range.size(); }

            instance_t* clone_full() const;
            instance_t* clone_slice() const;

            instance_t* release();
            void        invalidate();

            static instance_t s_default;
        };

        void data_t::addToList(instance_t* node)
        {
            if (m_head == nullptr)
            {
                m_head       = node;
                node->m_next = node->m_prev = node;
            }
            else
            {
                node->m_next           = m_head;
                node->m_prev           = m_head->m_prev;
                m_head->m_prev->m_next = node;
                m_head->m_prev         = node;
            }
        }

        void nstring::data_t::remFromList(nstring::instance_t* node)
        {
            if (m_head == node)
            {
                m_head = node->m_next;
            }

            node->m_next->m_prev = node->m_prev;
            node->m_prev->m_next = node->m_next;

            if (m_head == node)
            {
                m_head = nullptr;
            }
        }

        static ucs2::rune s_default_str[4]      = {0, 0, 0, 0};
        instance_t        instance_t::s_default = {{0, 0}, &data_t::s_default, &instance_t::s_default, &instance_t::s_default};
        data_t            data_t::s_default     = {s_default_str, &instance_t::s_default, 0, 1};

        static inline bool             s_is_default_data(nstring::data_t* data) { return data == &data_t::s_default; }
        static inline nstring::data_t* s_get_default_data() { return &data_t::s_default; }
        static inline bool             s_is_default_instance(nstring::instance_t* item) { return item == &instance_t::s_default; }
        static nstring::instance_t*    s_get_default_instance() { return &instance_t::s_default; }

        static nstring::data_t* s_alloc_data(s32 strlen)
        {
            nstring::data_t* data = (nstring::data_t*)nstring_memory::s_object_alloc->allocate(sizeof(nstring::data_t));
            data->m_ref           = 0;

            ucs2::prune strdata = (ucs2::prune)nstring_memory::s_string_alloc->allocate((strlen + 1) * sizeof(ucs2::rune));
            data->m_len         = strlen;
            data->m_ptr         = strdata;
            data->m_head        = nullptr;

            return data;
        }

        static void s_resize_data(nstring::data_t* data, s32 new_size)
        {
            if (data->m_len < new_size)
            {
                ucs2::prune newptr = (ucs2::prune)nstring_memory::s_string_alloc->allocate((new_size + 1) * sizeof(uchar16));
                for (s32 i = 0; i < data->m_len; i++)
                    newptr[i] = data->m_ptr[i];
                nstring_memory::s_string_alloc->deallocate(data->m_ptr);
                data->m_ptr                 = newptr;
                data->m_len                 = new_size;
                data->m_ptr[new_size].value = '\0';
            }
        }

        static nstring::data_t* s_unique_data(nstring::data_t* data, u32 from, u32 to)
        {
            ASSERT(from <= to);
            const s32        len     = to - from;
            nstring::data_t* newdata = (nstring::data_t*)nstring_memory::s_object_alloc->allocate(len);
            ucs2::prune      newptr  = (ucs2::prune)nstring_memory::s_string_alloc->allocate(len * sizeof(uchar16) + 1);
            newdata->m_ptr           = newptr;
            newdata->m_head          = nullptr;
            newdata->m_len           = len;
            newdata->m_ref           = 0;

            for (s32 i = 0; i < len; i++)
                newptr[i] = data->m_ptr[from + i];

            return newdata;
        }

        static nstring::instance_t* s_alloc_instance(nstring::range_t range, nstring::data_t* data)
        {
            nstring::instance_t* v = (nstring::instance_t*)nstring_memory::s_object_alloc->allocate(sizeof(nstring::instance_t));
            v->m_range             = range;
            v->m_data              = data->attach();
            v->m_data->addToList(v);
            return v;
        }

        static bool s_is_view_of(nstring::instance_t const* parent, nstring::instance_t const* slice) { return (parent->m_data == slice->m_data) && (slice->m_range.is_inside(parent->m_range)); }

        static bool s_narrow_view(nstring::instance_t* v, s32 move)
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

        static inline s32 s_move_view_right_count(nstring::instance_t const* str, nstring::range_t& view) { return str->m_range.m_to - view.m_to; }
        static inline s32 s_move_view_left_count(nstring::instance_t const* str, nstring::range_t& view) { return view.m_from - str->m_range.m_from; }

        static inline bool s_move_view(nstring::instance_t const* str, nstring::range_t& view, s32 move)
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

        static void s_insert_space(ucs2::prune str, s32 strlen, s32 pos, s32 len)
        {
            // insert space, need to start at the end to avoid overwriting
            ucs2::rune const* src = str + strlen;
            ucs2::rune*       dst = str + strlen + len;
            ucs2::rune const* end = str + pos;
            while (src > end)
            {
                *(--dst) = *(--src);
            }
        }

        static void s_remove_space(ucs2::prune str, s32 strlen, s32 pos, s32 len)
        {
            ucs2::prune  dst = str + pos;
            ucs2::pcrune src = dst + len;
            ucs2::pcrune end = str + strlen;
            while (src < end)
                *dst++ = *src++;
            dst->value = '\0';
        }

        // forward declare
        static void s_adjust_active_views(nstring::instance_t* list, s32 op_code, s32 op_range_from, s32 op_range_to);

        static const s32 REMOVAL   = 0;
        static const s32 INSERTION = 1;
        static const s32 CLEARED   = 2;
        static const s32 RELEASED  = 3;

        static void s_string_insert(nstring::instance_t* item, nstring::range_t selection, nstring::instance_t const* insert)
        {
            s32 const insertionLength = insert->size();
            if (insertionLength == 0)
                return;

            s32 const selectionLength = selection.size();
            s32       insertionPos    = selection.m_from;
            if (selectionLength < insertionLength)
            {
                // The string to insert is larger than the selection, so we have to insert some
                // space into the string.
                s_resize_data(item->m_data, item->m_data->m_len + (insertionLength - selectionLength));
                s_insert_space(item->m_data->m_ptr, item->m_data->m_len, insertionPos, insertionLength - selectionLength);
                s_adjust_active_views(item, INSERTION, insertionPos, insertionPos + insertionLength - selectionLength);
            }
            else if (selectionLength > insertionLength)
            {
                // The string to insert is smaller than the selection, so we have to remove some
                // space from the string.
                s_remove_space(item->m_data->m_ptr, item->m_data->m_len, insertionPos, selectionLength - insertionLength);
                s_adjust_active_views(item, REMOVAL, insertionPos, insertionPos + selectionLength - insertionLength);
            }
            else
            {
                // The string to insert is the same size as the selection, so we can just replace the
                // selection with the new string.
            }

            s32          src         = 0;
            ucs2::pcrune insert_data = insert->m_data->m_ptr + insert->m_range.m_from;
            ucs2::pcrune insert_end  = insert_data + insertionLength;
            ucs2::prune  str_data    = item->m_data->m_ptr + item->m_range.m_from + insertionPos;
            while (insert_data < insert_end)
                *str_data++ = *insert_data++;
        }

        static void string_remove(nstring::instance_t* str, nstring::range_t selection)
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

        static nstring::range_t s_find(nstring::instance_t* str, const nstring::instance_t* find)
        {
            if (find->is_empty() == false)
            {
                s32 const   strsize  = str->m_range.size();
                s32 const   findsize = find->m_range.size();
                ucs2::prune strdata  = str->m_data->m_ptr + str->m_range.m_from;
                ucs2::prune finddata = find->m_data->m_ptr + find->m_range.m_from;

                s32 i = 0;
                while (i < strsize)
                {
                    if (strdata[i] == finddata[0])
                    {
                        s32 j = 1;
                        while (j < findsize)
                        {
                            if (strdata[i + j] != finddata[j])
                                goto continue_search;
                            j++;
                        }
                        return {i, i + findsize};
                    }
                continue_search:
                    i++;
                }
            }
            return {0, 0};
        }

        static bool s_find_replace(nstring::instance_t* str, const nstring::instance_t* find, const nstring::instance_t* replace)
        {
            nstring::range_t remove = s_find(str, find);
            if (remove.is_empty() == false)
            {
                s32 const remove_from = remove.m_from;
                s32 const remove_len  = remove.size();
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
                s32          src          = 0;
                s32          dst          = remove_from;
                s32 const    end          = remove_from + replace->size();
                ucs2::prune  pdst         = str->m_data->m_ptr;
                ucs2::pcrune replace_data = replace->m_data->m_ptr + replace->m_range.m_from;
                while (src < replace->size())
                {
                    pdst[dst++] = replace_data[src++];
                }
                return true;
            }
            return false;
        }

        static bool s_contains(const uchar16* str, s32 strLen, uchar32 find)
        {
            uchar16 const* strData = str;
            uchar16 const* strEnd  = str + strLen;
            while (strData < strEnd)
            {
                uchar32 const c = *strData++;
                if (c == find)
                    return true;
            }
            return false;
        }

        static void s_remove_any(nstring::instance_t* str, const uchar16* any, s32 any_count)
        {
            // Remove any of the characters in @charset from @str
            s32 const strfrom = str->m_range.m_from;
            s32 const strto   = str->m_range.m_to;
            s32 const strsize = str->m_range.m_to - str->m_range.m_from;

            s32         d       = 0;
            s32         i       = 0;
            s32         r       = -1;
            ucs2::prune strdata = str->m_data->m_ptr + strfrom;
            while (i < strsize)
            {
                uchar32 const c = strdata[i].value;
                if (s_contains(any, any_count, c))
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
                        strdata[d].value = c;
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
                ucs2::prune strdata = str->m_data->m_ptr + strfrom;
                while (i < str->m_data->m_len)
                {
                    uchar32 const c  = strdata[i].value;
                    strdata[d].value = c;
                    i++;
                    d++;
                }

                str->m_data->m_len -= l;
                ucs2::prune pdst               = (ucs2::prune)str->m_data->m_ptr;
                pdst[str->m_data->m_len].value = '\0';
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
        static void s_adjust_active_view(nstring::instance_t* v, s32 op_code, s32 rhs_from, s32 rhs_to)
        {
            s32& lhs_from = v->m_range.m_from;
            s32& lhs_to   = v->m_range.m_to;

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

        static void s_adjust_active_views(nstring::instance_t* list, s32 op_code, s32 op_range_from, s32 op_range_to)
        {
            nstring::instance_t* iter = list;
            do
            {
                s_adjust_active_view(iter, op_code, op_range_from, op_range_to);
                iter = iter->m_next;
            } while (iter != list);
        }

        //------------------------------------------------------------------------------
        //------------------------------------------------------------------------------
        //------------------------------------------------------------------------------
        nstring::data_t* nstring::data_t::detach()
        {
            if (m_ref == 1)
            {
                if (s_is_default_data(this))
                    return this;

                nstring_memory::s_string_alloc->deallocate(m_ptr);
                nstring_memory::s_object_alloc->deallocate(this);
                return s_get_default_data();
            }
            else if (m_ref > 0)
            {
                m_ref--;
            }
            return this;
        }

        //------------------------------------------------------------------------------
        //------------------------------------------------------------------------------
        //------------------------------------------------------------------------------
        nstring::instance_t* nstring::instance_t::clone_full() const
        {
            s32 const        strlen = m_range.size();
            nstring::data_t* data;
            if (s_is_default_data(m_data))
            {
                data = m_data->attach();
            }
            else
            {
                data = s_unique_data(m_data, m_range.m_from, m_range.m_to);
            }
            nstring::instance_t* v = s_alloc_instance({0, strlen}, data);
            return v;
        }

        nstring::instance_t* nstring::instance_t::clone_slice() const
        {
            nstring::instance_t* v = s_alloc_instance(m_range, m_data);
            return v;
        }

        nstring::instance_t* nstring::instance_t::release()
        {
            if (!s_is_default_instance(this))
            {
                if (!s_is_default_data(m_data))
                {
                    m_data->remFromList(this);
                    m_data->detach();
                }
                nstring_memory::s_object_alloc->deallocate(this);
            }
            return s_get_default_instance();
        }

        void nstring::instance_t::invalidate()
        {
            if (!s_is_default_data(m_data))
            {
                m_data->remFromList(this);
                m_data->detach();
                m_data = s_get_default_data();
            }
            m_range = {0, 0};
        }

        //------------------------------------------------------------------------------
        //------------ range -----------------------------------------------------------
        //------------------------------------------------------------------------------
        // select parst of the string, return local view range
        static nstring::range_t selectBeforeLocal(const nstring::instance_t* str, const nstring::instance_t* sel) { return {str->m_range.m_from - str->m_range.m_from, sel->m_range.m_from - str->m_range.m_from}; }
        static nstring::range_t selectBeforeIncludedLocal(const nstring::instance_t* str, const nstring::instance_t* sel) { return {str->m_range.m_from - str->m_range.m_from, sel->m_range.m_to - str->m_range.m_from}; }
        static nstring::range_t selectAfterLocal(const nstring::instance_t* str, const nstring::instance_t* sel) { return {sel->m_range.m_to - str->m_range.m_from, str->m_range.m_to - str->m_range.m_from}; }
        static nstring::range_t selectAfterIncludedLocal(const nstring::instance_t* str, const nstring::instance_t* sel) { return {sel->m_range.m_from - str->m_range.m_from, str->m_range.m_to - str->m_range.m_from}; }

        static s32 compare(const nstring::instance_t* lhs, nstring::range_t const& lhsview, const nstring::instance_t* rhs)
        {
            ASSERT(lhsview.is_inside(lhs->m_range));

            if (lhsview.size() < rhs->size())
                return -1;
            if (lhsview.size() > rhs->size())
                return 1;
            ucs2::pcrune lhsdata = lhs->m_data->m_ptr + lhs->m_range.m_from + lhsview.m_from;
            ucs2::pcrune rhsdata = rhs->m_data->m_ptr + rhs->m_range.m_from;
            ucs2::pcrune lhsend  = lhsdata + lhsview.size();
            while (lhsdata < lhsend)
            {
                ucs2::rune const lc = *lhsdata++;
                ucs2::rune const rc = *rhsdata++;
                if (lc != rc)
                    return (lc.value < rc.value) ? -1 : 1;
            }
            return 0;
        }

        static bool isEqual(const nstring::instance_t* lhs, nstring::range_t const& lhsview, const nstring::instance_t* rhs) { return compare(lhs, lhsview, rhs) == 0; }

        static nstring::range_t findCharUntil(const nstring::instance_t* str, uchar32 find)
        {
            ucs2::pcrune strdata = str->m_data->m_ptr + str->m_range.m_from;
            for (s32 i = 0; i < str->size(); i++)
            {
                uchar32 const c = strdata[i].value;
                if (c == find)
                {
                    return {0, i};
                }
            }
            return {0, 0};
        }

        static nstring::range_t findStrUntil(const nstring::instance_t* str, const nstring::instance_t* find)
        {
            nstring::range_t view  = {0, find->size()};
            s32 const        count = s_move_view_right_count(str, view);
            for (s32 m = 0; m < count; m++)
            {
                if (nstring::isEqual(str, view, find))
                    return {0, view.m_from};
                view.move_right();
            }
            return {0, 0};
        }

        static nstring::range_t findCharUntilLast(const nstring::instance_t* str, uchar32 find)
        {
            ucs2::pcrune strdata = str->m_data->m_ptr + str->m_range.m_from;
            for (s32 i = str->size() - 1; i >= 0; --i)
            {
                uchar32 const c = strdata[i].value;
                if (c == find)
                {
                    return {str->m_range.m_from, str->m_range.m_from + i};
                }
            }
            return {0, 0};
        }

        static nstring::range_t findStrUntilLast(const nstring::instance_t* str, const nstring::instance_t* find)
        {
            nstring::range_t view = {0, 0};
            view.m_from           = str->size() - find->size();
            view.m_to             = str->size();
            s32 const count       = s_move_view_left_count(str, view);
            for (s32 m = 0; m < count; m++)
            {
                if (isEqual(str, view, find))
                    return {0, view.m_from};
                view.move_left();
            }
            return {0, 0};
        }

        static void toAscii(const nstring::instance_t* str, char* dst, s32 dstMaxLen)
        {
            s32 const len = str->size();
            s32       i   = 0;
            dstMaxLen -= 1;
            for (; i < len && i < dstMaxLen; ++i)
            {
                ucs2::rune c16 = str->m_data->m_ptr[str->m_range.m_from + i];
                uchar8     c8  = c16.value;
                if (c16.value > 127)
                    c8 = '?';
                dst[i] = c8;
            }
            dst[i] = '\0';
        }

    };  // namespace nstring

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------

    string_t::string_t() { m_item = nstring::s_get_default_instance(); }

    string_t::string_t(const char* str)
    {
        crunes_t srcrunes(str);

        s32 const strlen = srcrunes.size();

        s32 from = 0;
        s32 to   = strlen;

        if (strlen > 0)
        {
            nstring::data_t* data = nstring::s_alloc_data(strlen);
            m_item                = nstring::s_alloc_instance({from, to}, data);
            ucs2::prune dst       = data->m_ptr;
            while (*str != '\0')
            {
                ucs2::rune r;
                r.value = *str++;
                *dst++  = r;
            }
            dst->value = '\0';
        }
        else
        {
            m_item = nstring::s_get_default_instance();
        }
    }

    string_t::string_t(s32 _len)
        : m_item(nullptr)
    {
        const s32 strlen = _len;

        if (strlen > 0)
        {
            nstring::data_t* data = nstring::s_alloc_data(strlen);
            m_item                = nstring::s_alloc_instance({0, 0}, data);
        }
        else
        {
            m_item = nstring::s_get_default_instance();
        }
    }

    string_t::string_t(const string_t& other) { m_item = other.m_item->clone_full(); }

    string_t::string_t(const string_t& left, const string_t& right)
    {
        const s32 strlen = left.size() + right.size();

        nstring::data_t* data = nstring::s_alloc_data(strlen);
        m_item                = nstring::s_alloc_instance({0, strlen}, data);

        // manually copy the left and right strings into the new string
        ucs2::prune src = left.m_item->m_data->m_ptr + left.m_item->m_range.m_from;
        ucs2::prune dst = m_item->m_data->m_ptr;
        for (s32 i = 0; i < left.size(); i++)
            *dst++ = *src++;
        src = right.m_item->m_data->m_ptr + right.m_item->m_range.m_from;
        for (s32 i = 0; i < right.size(); i++)
            *dst++ = *src++;

        m_item->m_range.m_to = strlen;
    }

    string_t::string_t(nstring::instance_t* instance, s32 weird) { m_item = instance; }

    string_t::string_t(nstring::instance_t* instance, s32 from, s32 to, s32 weird)
    {
        m_item = instance;
        m_item->m_range.m_from += from;
        m_item->m_range.m_to = m_item->m_range.m_from + (to - from);
    }

    string_t::~string_t() { release(); }

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    s32      string_t::size() const { return m_item->size(); }
    s32      string_t::cap() const { return m_item->cap(); }
    bool     string_t::is_empty() const { return m_item->is_empty(); }
    bool     string_t::is_slice() const { return m_item->is_slice(); }
    void     string_t::clear() { release(); }
    string_t string_t::slice() const
    {
        nstring::instance_t* item = m_item->clone_slice();
        return string_t(item, m_item->m_range.m_from, m_item->m_range.m_to, 8888);
    }
    string_t string_t::clone() const { return string_t(m_item->clone_full(), 8888); }

    string_t string_t::operator()(s32 _from, s32 _to) const
    {
        math::sort(_from, _to);
        const u32            from = math::min(m_item->m_range.m_from + _from, m_item->m_range.m_to);
        const u32            to   = math::min(m_item->m_range.m_from + _to, m_item->m_range.m_to);
        nstring::instance_t* item = m_item->clone_slice();
        return string_t(item, from, to, 8888);
    }

    uchar32 string_t::operator[](s32 index) const
    {
        if (index >= size())
            return '\0';
        ucs2::pcrune str = m_item->m_data->m_ptr + m_item->m_range.m_from;
        return str[index].value;
    }

    string_t& string_t::operator=(const char* other)
    {
        s32 strlen = 0;
        while (other[strlen] != '\0')
            ++strlen;

        release();

        if (strlen != 0)
        {
            nstring::data_t*     data = nstring::s_alloc_data(strlen);
            nstring::instance_t* item = nstring::s_alloc_instance({0, strlen}, data);
            ucs2::prune          dst  = data->m_ptr;
            ucs2::pcrune         end  = dst + strlen;
            while (dst < end)
            {
                ucs2::rune r;
                r.value = *other;
                *dst++  = r;
            }
            dst->value             = '\0';
            m_item                 = item;
            m_item->m_range.m_from = 0;
            m_item->m_range.m_to   = strlen;
        }
        else
        {
            m_item = nstring::s_get_default_instance();
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
            m_item->m_data->remFromList(m_item);
            m_item->m_data->detach();

            m_item->m_data = other.m_item->m_data->attach();
            m_item->m_data->addToList(m_item);
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

    bool string_t::operator==(const string_t& other) const { return nstring::isEqual(m_item, m_item->m_range, other.m_item); }
    bool string_t::operator!=(const string_t& other) const { return !nstring::isEqual(m_item, m_item->m_range, other.m_item); }

    void string_t::release() { m_item = m_item->release(); }

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    string_t string_t::select(u32 from, u32 to) const
    {
        // Make sure we keep within the bounds of the string
        from = math::min(from, (u32)size());
        to   = math::min(to, (u32)size());

        nstring::instance_t* item = m_item->clone_slice();
        item->m_range.m_from      = m_item->m_range.m_from + from;
        item->m_range.m_to        = m_item->m_range.m_from + to;

        string_t str;
        str.m_item = item;
        return str;
    }

    string_t string_t::selectUntil(uchar32 find) const
    {
        nstring::range_t view = nstring::findCharUntil(m_item, find);
        if (view.is_empty())
            return string_t(nstring::s_get_default_instance(), 8888);
        return select(view.m_from, view.m_to);
    }

    string_t string_t::selectUntil(const string_t& selection) const
    {
        nstring::range_t view = nstring::selectBeforeLocal(m_item, selection.m_item);
        if (view.is_empty())
            return string_t(nstring::s_get_default_instance(), 8888);
        return select(view.m_from, view.m_to);
    }

    string_t string_t::selectUntilLast(uchar32 find) const
    {
        nstring::range_t view = nstring::findCharUntilLast(m_item, find);
        if (view.is_empty())
            return string_t(nstring::s_get_default_instance(), 8888);
        return select(view.m_from, view.m_to);
    }

    string_t string_t::selectUntilLast(const string_t& selection) const
    {
        // TODO What if 'selection' is not part of this string ?
        nstring::range_t view = nstring::selectBeforeLocal(m_item, selection.m_item);
        if (view.is_empty())
            return string_t(nstring::s_get_default_instance(), 8888);
        return select(view.m_from, view.m_to);
    }

    string_t string_t::selectUntilIncluded(uchar32 find) const
    {
        nstring::range_t view = nstring::findCharUntil(m_item, find);
        if (view.is_empty())
            return string_t(nstring::s_get_default_instance(), 8888);
        return select(view.m_from, view.m_to + 1);
    }

    string_t string_t::selectUntilIncluded(const string_t& selection) const
    {
        nstring::range_t view = nstring::selectBeforeIncludedLocal(m_item, selection.m_item);
        if (view.is_empty())
            return string_t(nstring::s_get_default_instance(), 8888);
        return select(view.m_from, view.m_to);
    }

    string_t string_t::selectUntilEndExcludeSelection(const string_t& selection) const
    {
        nstring::range_t     range = nstring::selectAfterLocal(m_item, selection.m_item);
        nstring::instance_t* item  = m_item->clone_slice();
        return string_t(item, range.m_from, range.m_to, 8888);
    }
    string_t string_t::selectUntilEndIncludeSelection(const string_t& selection) const
    {
        nstring::range_t     range = nstring::selectAfterIncludedLocal(m_item, selection.m_item);
        nstring::instance_t* item  = m_item->clone_slice();
        return string_t(item, range.m_from, range.m_to, 8888);
    }

    bool string_t::isUpper() const
    {
        ucs2::pcrune strdata = m_item->m_data->m_ptr + m_item->m_range.m_from;
        ucs2::pcrune strend  = m_item->m_data->m_ptr + m_item->m_range.m_to;
        while (strdata < strend)
        {
            ucs2::rune r = *strdata++;
            if (nrunes::is_lower(r.value))
                return false;
        }
        return true;
    }

    bool string_t::isLower() const
    {
        ucs2::pcrune strdata = m_item->m_data->m_ptr + m_item->m_range.m_from;
        ucs2::pcrune strend  = m_item->m_data->m_ptr + m_item->m_range.m_to;
        while (strdata < strend)
        {
            ucs2::rune r = *strdata++;
            if (nrunes::is_upper(r.value))
                return false;
        }
        return true;
    }

    bool string_t::isCapitalized() const
    {
        ucs2::pcrune strdata = m_item->m_data->m_ptr + m_item->m_range.m_from;
        ucs2::pcrune strend  = m_item->m_data->m_ptr + m_item->m_range.m_to;
        while (strdata < strend)
        {
            ucs2::rune c = *strdata++;
            while (strdata < strend)
            {
                if (!nrunes::is_space(c.value))
                    break;
                c = *strdata++;
            }
            if (nrunes::is_upper(c.value))
            {
                while (strdata < strend)
                {
                    c = *strdata;
                    if (nrunes::is_space(c.value))
                        break;
                    if (nrunes::is_upper(c.value))
                        return false;
                    strdata++;
                }
            }
            else if (nrunes::is_alpha(c.value))
            {
                return false;
            }
        }
        return true;
    }

    bool string_t::isQuoted() const { return isQuoted('"'); }
    bool string_t::isQuoted(uchar32 inQuote) const { return isDelimited(inQuote, inQuote); }

    bool string_t::isDelimited(uchar32 inLeft, uchar32 inRight) const
    {
        if (m_item->m_range.is_empty())
            return false;
        ucs2::rune const l = m_item->m_data->m_ptr[m_item->m_range.m_from];
        ucs2::rune const r = m_item->m_data->m_ptr[m_item->m_range.m_to - 1];
        return (l.value == inLeft && r.value == inRight);
    }

    uchar32 string_t::firstChar() const { return m_item->m_data->m_ptr[m_item->m_range.m_from].value; }
    uchar32 string_t::lastChar() const
    {
        if (m_item->m_range.is_empty())
            return '\0';
        return m_item->m_data->m_ptr[m_item->m_range.m_to - 1].value;
    }

    bool string_t::startsWith(const string_t& start) const
    {
        if (start.size() > 0)
        {
            nstring::range_t v = start.m_item->m_range.local();
            if (v.is_inside(m_item->m_range))
                return nstring::isEqual(m_item, v, start.m_item);
        }
        return false;
    }

    bool string_t::endsWith(const string_t& end) const
    {
        if (end.size() > size())
            return false;
        nstring::range_t v = {0, 0};
        v.m_from           = size() - end.size();
        v.m_to             = size();
        if (!v.is_empty())
            return nstring::isEqual(m_item, v, end.m_item);
        return false;
    }

    string_t string_t::find(uchar32 find) const
    {
        ucs2::pcrune strdata = m_item->m_data->m_ptr + m_item->m_range.m_from;
        for (s32 i = 0; i < size(); i++)
        {
            uchar32 const c = strdata[i].value;
            if (c == find)
                return select(i, i + 1);
        }
        return string_t(nstring::s_get_default_instance(), 8888);
    }

    string_t string_t::findLast(uchar32 find) const
    {
        if (size() == 0)
            return string_t(nstring::s_get_default_instance(), 8888);

        ucs2::pcrune strbegin = m_item->m_data->m_ptr + m_item->m_range.m_from;
        ucs2::pcrune strdata  = m_item->m_data->m_ptr + m_item->m_range.m_to - 1;
        while (strdata >= strbegin)
        {
            uchar32 const c = strdata->value;
            if (c == find)
                return select((s32)(strdata - strbegin), (s32)((strdata + 1) - strbegin));
            strdata++;
        }
        return string_t(nstring::s_get_default_instance(), 8888);
    }

    string_t string_t::find(const char* inFind) const
    {
        ucs2::pcrune str = m_item->m_data->m_ptr + m_item->m_range.m_from;
        s32 const    len = size();
        for (s32 i = 0; i < len; i++)
        {
            uchar32 const c = str[i].value;
            if (c == *inFind)
            {
                s32 s = 1;
                for (s32 j = i + 1; j < len; j++, ++s)
                {
                    if (inFind[s] == '\0')
                        return select(m_item->m_range.m_from + i, m_item->m_range.m_from + j);
                    if (str[j].value != inFind[s])
                        break;
                }
                if (inFind[s] == '\0')
                    return select(m_item->m_range.m_from + i, m_item->m_range.m_from + len);
            }
        }
        return string_t(nstring::s_get_default_instance(), 8888);
    }

    string_t string_t::find(const string_t& find) const
    {
        if (find.size() > size())
            return string_t(nstring::s_get_default_instance(), 8888);

        nstring::range_t v = nstring::s_find(m_item, find.m_item);
        if (v.is_empty())
            return string_t(nstring::s_get_default_instance(), 8888);

        return select(v.m_from, v.m_to);
    }

    string_t string_t::findLast(const string_t& find) const
    {
        if (find.size() > size())
            return string_t(nstring::s_get_default_instance(), 8888);

        nstring::range_t v = {0, 0};
        v.m_from           = size() - find.size();
        v.m_to             = size();

        while (!v.is_empty())
        {
            if (nstring::isEqual(m_item, v, find.m_item))
            {
                // So here we have a view with the size of the @find string on
                // string @str that matches the string @find.
                return select(v.m_from, v.m_to);
            }
            if (!s_move_view(m_item, v, -1))
                break;
        }
        return string_t(nstring::s_get_default_instance(), 8888);
    }

    string_t string_t::findOneOf(const string_t& charset) const
    {
        ucs2::pcrune strdata = m_item->m_data->m_ptr + m_item->m_range.m_from;
        for (s32 i = 0; i < size(); i++)
        {
            uchar32 const sc = strdata[i].value;
            if (charset.contains(sc))
            {
                return select(i, i + 1);
            }
        }
        return string_t(nstring::s_get_default_instance(), 8888);
    }

    string_t string_t::findOneOfLast(const string_t& charset) const
    {
        ucs2::pcrune strdata = m_item->m_data->m_ptr + m_item->m_range.m_from;
        for (s32 i = size() - 1; i >= 0; --i)
        {
            uchar32 const sc = strdata[i].value;
            if (charset.contains(sc))
            {
                return select(i, i + 1);
            }
        }
        return string_t(nstring::s_get_default_instance(), 8888);
    }

    s32  string_t::compare(const string_t& rhs) const { return nstring::compare(m_item, m_item->m_range, rhs.m_item); }
    bool string_t::isEqual(const string_t& rhs) const { return nstring::compare(m_item, m_item->m_range, rhs.m_item) == 0; }

    bool string_t::contains(const string_t& contains) const
    {
        if (contains.size() > size())
            return false;

        nstring::range_t v = {0, 0};
        v.m_to             = contains.size();

        while (!v.is_empty())
        {
            if (nstring::isEqual(m_item, v, contains.m_item))
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
        ucs2::pcrune strdata = m_item->m_data->m_ptr + m_item->m_range.m_from;
        for (s32 i = 0; i < size(); i++)
        {
            uchar32 const sc = strdata[i].value;
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
        ucs2::pcrune src = con.m_item->m_data->m_ptr + con.m_item->m_range.m_from;
        ucs2::prune  dst = m_item->m_data->m_ptr + m_item->m_range.m_to;
        for (s32 i = 0; i < con.size(); i++)
            *dst++ = *src++;
        dst->value = '\0';

        m_item->m_range.m_to = len;
    }

    void string_t::concatenate(const string_t& strA, const string_t& strB)
    {
        s32 const len = size() + strA.size() + strB.size();
        s_resize_data(m_item->m_data, len);

        // manually append the incoming strings to the end of the current string
        ucs2::prune dst = m_item->m_data->m_ptr + m_item->m_range.m_to;

        ucs2::pcrune srcA = strA.m_item->m_data->m_ptr + strA.m_item->m_range.m_from;
        for (s32 i = 0; i < strA.size(); i++)
            *dst++ = *srcA++;

        ucs2::pcrune srcB = strB.m_item->m_data->m_ptr + strB.m_item->m_range.m_from;
        for (s32 i = 0; i < strB.size(); i++)
            *dst++ = *srcB++;

        dst->value = '\0';

        m_item->m_range.m_to = len;
    }

    void string_t::concatenate_repeat(const string_t& con, s32 ntimes)
    {
        s32 const len = size() + (con.size() * ntimes);
        s_resize_data(m_item->m_data, len);

        ucs2::prune dst = m_item->m_data->m_ptr + m_item->m_range.m_to;
        for (s32 i = 0; i < ntimes; ++i)
        {
            ucs2::pcrune src = con.m_item->m_data->m_ptr + con.m_item->m_range.m_from;
            for (s32 i = 0; i < con.size(); i++)
                *dst++ = *src++;
        }
        dst->value           = '\0';
        m_item->m_range.m_to = len;
    }

    s32 string_t::format(const string_t& format, const va_t* argv, s32 argc)
    {
        release();

        crunes_t  fmt(format.m_item->m_data->m_ptr, format.m_item->m_range.m_from, format.m_item->m_range.m_to, format.m_item->m_data->m_len);
        const s32 len = cprintf_(fmt, argv, argc);

        nstring::data_t*     data = nstring::s_alloc_data(len);
        nstring::instance_t* item = nstring::s_alloc_instance({0, len}, data);

        runes_t str(item->m_data->m_ptr, item->m_range.m_from, item->m_range.m_to, item->m_data->m_len);
        sprintf_(str, fmt, argv, argc);
        item->m_range.m_to = str.m_end;

        m_item = item;
        return len;
    }

    s32 string_t::formatAdd(const string_t& format, const va_t* argv, s32 argc)
    {
        crunes_t  fmt(format.m_item->m_data->m_ptr, format.m_item->m_range.m_from, format.m_item->m_range.m_to, format.m_item->m_data->m_len);
        const s32 len = cprintf_(fmt, argv, argc);
        s_resize_data(m_item->m_data, len);
        runes_t str(m_item->m_data->m_ptr, m_item->m_range.m_from, m_item->m_range.m_to, m_item->m_data->m_len);
        str.m_str = str.m_end;
        sprintf_(str, fmt, argv, argc);
        m_item->m_range.m_to = str.m_end;
        return len;
    }

    void string_t::insertReplaceSelection(const string_t& selection, const string_t& insert)
    {
        nstring::range_t range = selection.m_item->m_range;
        s_string_insert(m_item, range, insert.m_item);
    }

    void string_t::insertBeforeSelection(const string_t& selection, const string_t& insert)
    {
        nstring::range_t range(selection.m_item->m_range);
        range.m_to = range.m_from;
        s_string_insert(m_item, range, insert.m_item);
    }

    void string_t::insertAfterSelection(const string_t& selection, const string_t& insert)
    {
        nstring::range_t range = nstring::selectAfterLocal(m_item, selection.m_item);
        range.m_from           = range.m_to;
        s_string_insert(m_item, range, insert.m_item);
    }

    void string_t::removeSelection(const string_t& selection) { string_remove(m_item, selection.m_item->m_range); }

    s32 string_t::findRemove(const string_t& find, s32 ntimes)
    {
        nstring::range_t v = {0, 0};
        v.m_to             = find.size();

        for (s32 i = 0; i < ntimes; i++)
        {
            while (!v.is_empty())
            {
                if (nstring::isEqual(m_item, v, find.m_item))
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
        s32 n = ntimes;
        if (n == 0)
            n = size();

        ucs2::prune strdata = m_item->m_data->m_ptr + m_item->m_range.m_from;

        s32 i = 0;
        s32 p = 0;
        while (i < size())
        {
            if (n > 0 && strdata[i].value == c)
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
            strdata[p].value = '\0';

        // adjust the 'to' of the string
        m_item->m_range.m_to -= ntimes - n;

        return ntimes - n;
    }

    s32 string_t::removeAnyChar(const string_t& any, s32 ntimes)
    {
        ucs2::prune strdata = m_item->m_data->m_ptr + m_item->m_range.m_from;
        s32         len     = size();

        if (ntimes == 0)
            ntimes = len;

        s32 i = 0;
        s32 n = 0;
        while (i < len)
        {
            if (n < ntimes && any.contains(strdata[i].value))
            {
                string_remove(m_item, {i, i + 1});
                --len;
                ++n;
                continue;
            }
            ++i;
        }
        return n;
    }

    s32 string_t::replaceAnyChar(const string_t& any, uchar32 with, s32 ntimes)
    {
        // Replace any of the characters in @charset from @str with character @with
        ucs2::prune strdata = m_item->m_data->m_ptr + m_item->m_range.m_from;
        s32         n       = ntimes == 0 ? size() : ntimes;
        for (s32 i = 0; i < size(); ++i)
        {
            uchar32 const c = strdata[i].value;
            if (any.contains(c))
            {
                if (--n == 0)
                    break;
                strdata[i].value = with;
            }
        }
        return n;
    }

    void string_t::toUpper()
    {
        ucs2::prune  strdata = m_item->m_data->m_ptr + m_item->m_range.m_from;
        ucs2::pcrune strend  = m_item->m_data->m_ptr + m_item->m_range.m_to;
        while (strdata < strend)
        {
            strdata->value = nrunes::to_upper(strdata->value);
            ++strdata;
        }
    }

    void string_t::toLower()
    {
        ucs2::prune  strdata = m_item->m_data->m_ptr + m_item->m_range.m_from;
        ucs2::pcrune strend  = m_item->m_data->m_ptr + m_item->m_range.m_to;
        while (strdata < strend)
        {
            strdata->value = nrunes::to_lower(strdata->value);
            ++strdata;
        }
    }

    void string_t::capitalize()
    {
        // Standard separator is ' '
        bool        prev_is_space = true;
        s32         i             = 0;
        ucs2::prune strdata       = m_item->m_data->m_ptr + m_item->m_range.m_from;
        while (i < size())
        {
            uchar32 c = strdata[i].value;
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
                strdata[i].value = c;
            }
            i++;
        }
    }

    void string_t::capitalize(const string_t& separators)
    {
        bool        prev_is_space = false;
        s32         i             = 0;
        ucs2::prune strdata       = m_item->m_data->m_ptr + m_item->m_range.m_from;
        while (i < size())
        {
            uchar32 c = strdata[i].value;
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
                strdata[i].value = c;
            }
            i++;
        }
    }

    static void sTrimLeft(nstring::instance_t* item, ucs2::pcrune any, s32 num)
    {
        ucs2::pcrune end   = item->m_data->m_ptr + item->m_range.m_to;
        ucs2::pcrune begin = item->m_data->m_ptr + item->m_range.m_from;
    trim_continue:
        while (begin < end)
        {
            uchar16 const c = begin->value;
            for (s32 j = 0; j < num; ++j)
            {
                if (c == any[j].value)
                {
                    ++begin;
                    goto trim_continue;
                }
            }
            break;
        }
        item->m_range.m_from = begin - item->m_data->m_ptr;
    }

    static void sTrimRight(nstring::instance_t* item, ucs2::pcrune any, s32 num)
    {
        ucs2::pcrune end   = item->m_data->m_ptr + item->m_range.m_to;
        ucs2::pcrune begin = item->m_data->m_ptr + item->m_range.m_from;
    trim_continue:
        while (end > begin)
        {
            uchar16 const c = end[-1].value;
            for (s32 j = 0; j < num; ++j)
            {
                if (c == any[j].value)
                {
                    --end;
                    goto trim_continue;
                }
            }
            break;
        }
        item->m_range.m_to = end - item->m_data->m_ptr;
    }

    static const ucs2::rune sTrimWhiteSpace[] = {' ', '\t', '\r', '\n'};

    // Trim does nothing more than narrowing the <from, to>, nothing is actually removed
    // from the actual underlying string string_data.
    void string_t::trim()
    {
        sTrimLeft(m_item, sTrimWhiteSpace, sizeof(sTrimWhiteSpace) / sizeof(sTrimWhiteSpace[0]));
        sTrimRight(m_item, sTrimWhiteSpace, sizeof(sTrimWhiteSpace) / sizeof(sTrimWhiteSpace[0]));
    }

    void string_t::trimLeft() { sTrimLeft(m_item, sTrimWhiteSpace, sizeof(sTrimWhiteSpace) / sizeof(sTrimWhiteSpace[0])); }
    void string_t::trimRight() { sTrimRight(m_item, sTrimWhiteSpace, sizeof(sTrimWhiteSpace) / sizeof(sTrimWhiteSpace[0])); }

    void string_t::trim(uchar32 c)
    {
        ucs2::rune const any[1] = {c};
        sTrimLeft(m_item, any, 1);
        sTrimRight(m_item, any, 1);
    }

    void string_t::trimLeft(uchar32 c)
    {
        ucs2::rune const any[1] = {c};
        sTrimLeft(m_item, any, 1);
    }

    void string_t::trimRight(uchar32 c)
    {
        ucs2::rune const any[1] = {c};
        sTrimRight(m_item, any, 1);
    }

    void string_t::trim(const string_t& set)
    {
        sTrimLeft(m_item, set.m_item->m_data->m_ptr, set.size());
        sTrimRight(m_item, set.m_item->m_data->m_ptr, set.size());
    }

    void string_t::trimLeft(const string_t& set) { sTrimLeft(m_item, set.m_item->m_data->m_ptr, set.size()); }
    void string_t::trimRight(const string_t& set) { sTrimRight(m_item, set.m_item->m_data->m_ptr, set.size()); }

    void string_t::trimQuotes()
    {
        trimLeft('"');
        trimRight('"');
    }
    void string_t::trimQuotes(uchar32 quote)
    {
        trimLeft(quote);
        trimRight(quote);
    }

    void string_t::trimDelimiters(uchar32 left, uchar32 right)
    {
        trimLeft(left);
        trimRight(right);
    }

    void string_t::reverse()
    {
        s32 const   last    = size() - 1;
        ucs2::prune strdata = m_item->m_data->m_ptr + m_item->m_range.m_from;
        for (s32 i = 0; i < (last - i); ++i)
        {
            uchar32 l               = strdata[i].value;
            uchar32 r               = strdata[last - i].value;
            strdata[i].value        = r;
            strdata[last - i].value = l;
        }
    }

    bool string_t::selectBeforeAndAfter(const string_t& selection, string_t& outLeft, string_t& outRight) const
    {
        nstring::range_t range = nstring::selectBeforeLocal(m_item, selection.m_item);
        if (range.is_empty())
            return false;
        outLeft  = select(range.m_from, range.m_to);
        outRight = select(range.m_to + selection.size(), size());
        return true;
    }

    bool string_t::findCharSelectBeforeAndAfter(uchar32 find, string_t& outLeft, string_t& outRight) const
    {
        nstring::range_t range = nstring::findCharUntil(m_item, find);
        if (range.is_empty())
            return false;
        outLeft  = select(range.m_from, range.m_to);
        outRight = select(outLeft.size() + 1, size());
        return true;
    }

    bool string_t::findCharLastSelectBeforeAndAfter(uchar32 find, string_t& outLeft, string_t& outRight) const
    {
        nstring::range_t range = nstring::findCharUntilLast(m_item, find);
        if (range.is_empty())
            return false;
        outLeft  = select(range.m_from, range.m_to);
        outRight = select(range.m_to + 1, size());
        return true;
    }

    bool string_t::findStrSelectBeforeAndAfter(const string_t& find, string_t& outLeft, string_t& outRight) const
    {
        nstring::range_t range = nstring::findStrUntil(m_item, find.m_item);
        if (range.is_empty())
            return false;
        outLeft  = select(range.m_from, range.m_to);
        outRight = select(range.m_to + find.size(), size());
        return true;
    }

    bool string_t::findStrLastSelectBeforeAndAfter(const string_t& find, string_t& outLeft, string_t& outRight) const
    {
        nstring::range_t range = nstring::findStrUntilLast(m_item, find.m_item);
        if (range.is_empty())
            return false;
        outLeft  = select(range.m_from, range.m_to);
        outRight = select(range.m_to + find.size(), size());
        return true;
    }

    void string_t::toAscii(char* str, s32 maxlen) const { nstring::toAscii(m_item, str, maxlen); }

}  // namespace ncore
