#include "cbase/c_allocator.h"
#include "cbase/c_context.h"
#include "cbase/c_integer.h"
#include "cbase/c_memory.h"
#include "cbase/c_printf.h"
#include "cbase/c_runes.h"
#include "cstring/c_string.h"

namespace ncore
{
    struct str_data_t
    {
        runes_t::ptr_t m_str_ptr;
        s32            m_str_type;
        s32            m_str_len;

        s32 cap() const { return m_str_len; }
    };

    //==============================================================================
    // We use a (static) class here so that we can access protected members of string_t
    // since we are defined as a friend class.
    class ustring_t
    {
    public:
        static inline uchar32 get_char_unsafe(str_data_t const* data, str_range_t const& view, s32 i)
        {
            switch (data->m_str_type)
            {
                case ascii::TYPE:
                {
                    ascii::pcrune s = data->m_str_ptr.m_ascii + view.from + i;
                    return *s;
                }
                break;
                case utf32::TYPE:
                {
                    utf32::pcrune s = data->m_str_ptr.m_utf32 + view.from + i;
                    return *s;
                }
                break;
            }
            return '\0';
        }

        static inline uchar32 get_char_unsafe(str_slice_t const& str, s32 i) { return get_char_unsafe(str.m_data, str.m_view, i); }

        static inline void set_char_unsafe(str_data_t* data, str_range_t const& view, s32 i, uchar32 c)
        {
            switch (data->m_str_type)
            {
                case ascii::TYPE:
                {
                    ascii::prune s = data->m_str_ptr.m_ascii + view.from + i;
                    *s             = (ascii::rune)c;
                }
                break;
                case utf32::TYPE:
                {
                    utf32::prune s = data->m_str_ptr.m_utf32 + view.from + i;
                    *s             = c;
                }
                break;
            }
        }

        static inline void set_char_unsafe(str_slice_t& str, s32 i, uchar32 c) { set_char_unsafe(str.m_data, str.m_view, i, c); }

        static inline void get_from_to(crunes_t const& runes, s32& from, s32& to)
        {
            switch (runes.m_type)
            {
                case ascii::TYPE:
                {
                    from = (s32)(runes.m_ascii.m_str - runes.m_ascii.m_bos);
                    to   = (s32)(runes.m_ascii.m_end - runes.m_ascii.m_bos);
                }
                break;
                case utf32::TYPE:
                {
                    from = (s32)(runes.m_utf32.m_str - runes.m_utf32.m_bos);
                    to   = (s32)(runes.m_utf32.m_end - runes.m_utf32.m_bos);
                }
                break;
            }
        }

        static inline crunes_t get_crunes(str_data_t const* data, str_range_t view)
        {
            crunes_t r;
            r.m_type = data->m_str_type;
            switch (data->m_str_type)
            {
                case ascii::TYPE:
                    r.m_ascii.m_bos = data->m_str_ptr.m_ascii;
                    r.m_ascii.m_eos = r.m_ascii.m_bos + data->m_str_len;
                    r.m_ascii.m_str = r.m_ascii.m_bos + view.from;
                    r.m_ascii.m_end = r.m_ascii.m_bos + view.to;
                    break;
                case utf32::TYPE:
                    r.m_utf32.m_bos = data->m_str_ptr.m_utf32;
                    r.m_utf32.m_eos = r.m_utf32.m_bos + data->m_str_len;
                    r.m_utf32.m_str = r.m_utf32.m_bos + view.from;
                    r.m_utf32.m_end = r.m_utf32.m_bos + view.to;
                    break;
            }
            return r;
        }
        static inline crunes_t get_crunes(str_slice_t const& str) { return get_crunes(str.m_data, str.m_view); }

        static inline runes_t get_runes(str_data_t* data, str_range_t view)
        {
            runes_t r;
            r.m_type = data->m_str_type;
            switch (data->m_str_type)
            {
                case ascii::TYPE:
                    r.m_ascii.m_bos = data->m_str_ptr.m_ascii;
                    r.m_ascii.m_eos = r.m_ascii.m_bos + data->m_str_len;
                    r.m_ascii.m_str = r.m_ascii.m_bos + view.from;
                    r.m_ascii.m_end = r.m_ascii.m_bos + view.to;
                    break;
                case utf32::TYPE:
                    r.m_utf32.m_bos = data->m_str_ptr.m_utf32;
                    r.m_utf32.m_eos = r.m_utf32.m_bos + data->m_str_len;
                    r.m_utf32.m_str = r.m_utf32.m_bos + view.from;
                    r.m_utf32.m_end = r.m_utf32.m_bos + view.to;
                    break;
            }
            return r;
        }

        static str_data_t* allocdata(s32 _strlen, s32 _strtype)
        {
            str_data_t* data = (str_data_t*)context_t::system_alloc()->allocate(sizeof(str_data_t), sizeof(void*));
            data->m_str_type = _strtype;
            data->m_str_len  = _strlen;

            runes_t strdata         = context_t::string_alloc()->allocate(_strlen, _strlen, _strtype);
            data->m_str_ptr.m_ascii = strdata.m_ascii.m_bos;

            return data;
        }

        static void deallocdata(str_data_t* data)
        {
            runes_t runes = get_runes(data, str_range_t(0, 0));
            context_t::string_alloc()->deallocate(runes);
            context_t::system_alloc()->deallocate(data);
        }

        static void resize(str_slice_t& str, s32 new_size)
        {
            if (new_size > str.m_data->cap())
            {
                str_data_t* data = allocdata(new_size, str.m_data->m_str_type);

                runes_t trunes = get_runes(str.m_data, str_range_t(0, str.m_data->m_str_len));
                runes_t nrunes = get_runes(data, str_range_t(0, data->m_str_len));
                copy(trunes, nrunes);

                // deallocate old data and string
                deallocdata(str.m_data);

                // need to update the new data pointer for each view
                str_slice_t* list = &str;
                str_slice_t* iter = list;
                do
                {
                    iter->m_data = data;
                    iter         = iter->m_next;
                } while (iter != list);
            }
            else
            {
                // TODO: What about shrinking ?
            }
        }

        static bool is_view_of(str_slice_t const& str, str_slice_t const& part)
        {
            if (str.m_data == part.m_data)
            {
                if (part.m_view.from >= str.m_view.from)
                {
                    if (part.m_view.to <= str.m_view.to)
                    {
                        return true;
                    }
                }
            }
            return false;
        }

        static bool narrow_view(str_slice_t& v, s32 move)
        {
            if (v.size() > 0)
            {
                // if negative then narrow the left side
                if (move < 0)
                {
                    move = -move;
                    if (move <= v.m_view.size())
                    {
                        v.m_view.from += move;
                        return true;
                    }
                }
                else
                {
                    if (move <= v.m_view.size())
                    {
                        v.m_view.to -= move;
                        return true;
                    }
                }
            }
            return false;
        }

        static bool move_view(str_slice_t const& str, str_slice_t& view, s32 move)
        {
            s32 const from = view.m_view.from + move;

            // Check if the move doesn't result in an out-of-bounds
            if (from < str.m_view.from)
                return false;

            // Check if the movement doesn't invalidate this view
            s32 const to = view.m_view.to + move;
            if (to > str.m_view.to)
                return false;

            // Movement is ok, new view is valid
            view.m_view.from = from;
            view.m_view.to   = to;
            return true;
        }

        static str_slice_t select_before(const str_slice_t& str, const str_slice_t& selection)
        {
            str_slice_t v(selection);
            v.m_view.to   = v.m_view.from;
            v.m_view.from = 0;
            return v;
        }
        static str_slice_t select_before_included(const str_slice_t& str, const str_slice_t& selection)
        {
            str_slice_t v(selection);
            v.m_view.to   = v.m_view.to;
            v.m_view.from = 0;
            return v;
        }

        static str_slice_t select_after(const str_slice_t& str, const str_slice_t& sel)
        {
            str_slice_t v(sel);
            v.m_view.to   = str.m_view.to;
            v.m_view.from = sel.m_view.from;
            return v;
        }

        static str_slice_t select_after_excluded(const str_slice_t& str, const str_slice_t& sel)
        {
            str_slice_t v(sel);
            v.m_view.to   = str.m_view.to;
            v.m_view.from = sel.m_view.to;
            return v;
        }

        static void insert_space(runes_t& r, s32 pos, s32 len)
        {
            s32 src = r.size() - 1;
            s32 dst = src + len;
            switch (r.m_type)
            {
                case ascii::TYPE:
                    while (src >= pos)
                    {
                        r.m_ascii.m_str[dst--] = r.m_ascii.m_str[src--];
                    }
                    r.m_ascii.m_end += len;
                    r.m_ascii.m_end[0] = '\0';
                    break;
                case utf32::TYPE:
                    while (src >= pos)
                    {
                        r.m_utf32.m_str[dst--] = r.m_utf32.m_str[src--];
                    }
                    r.m_utf32.m_end += len;
                    r.m_utf32.m_end[0] = '\0';
                    break;
            }
        }

        static void remove_space(runes_t& r, s32 pos, s32 len)
        {
            s32       src = pos + len;
            s32       dst = pos;
            s32 const end = pos + len;
            switch (r.m_type)
            {
                case ascii::TYPE:
                    while (src < r.size())
                    {
                        r.m_ascii.m_str[dst++] = r.m_ascii.m_str[src++];
                    }
                    r.m_ascii.m_end -= len;
                    r.m_ascii.m_end[0] = '\0';
                    break;
                case utf32::TYPE:
                    while (src < r.size())
                    {
                        r.m_utf32.m_str[dst++] = r.m_utf32.m_str[src++];
                    }
                    r.m_utf32.m_end -= len;
                    r.m_utf32.m_end[0] = '\0';
                    break;
            }
        }

        static void insert(str_slice_t& str, str_slice_t const& pos, str_slice_t const& insert)
        {
            if (insert.is_empty() || (str.m_data != pos.m_data))
                return;

            s32 dst = pos.m_view.from;

            str_range_t insert_range(dst, dst + insert.size());
            ustring_t::adjust_active_views(&str, ustring_t::INSERTION, insert_range);

            s32 const current_len = str.m_data->m_str_len;
            ustring_t::resize(str, str.m_data->m_str_len + insert.size() + 1);
            {
                //@TODO: it should be better to get an actual full view from the list of strings, currently we
                //       take the easy way and just take the whole allocated size as the full view.
                runes_t str_runes = ustring_t::get_runes(str.m_data, str_range_t(0, current_len));
                insert_space(str_runes, dst, insert.size());
            }
            s32 src = 0;
            while (src < insert.size())
            {
                uchar32 const c = ustring_t::get_char_unsafe(insert.m_data, insert.m_view, src);
                ustring_t::set_char_unsafe(str.m_data, str.m_view, dst, c);
                ++src;
                ++dst;
            }
        }

        static void remove(str_slice_t& str, str_slice_t const& selection)
        {
            if (selection.is_empty())
                return;

            if (ustring_t::is_view_of(str, selection))
            {
                //@TODO: it should be better to get an actual full view from the list of strings, currently we
                //       take the easy way and just take the whole allocated size as the full view.
                runes_t str_runes = get_runes(str.m_data, str_range_t(0, str.m_data->m_str_len));
                remove_space(str_runes, selection.m_view.from, selection.size());

                // TODO: Decision to shrink the allocated memory of m_runes ?

                str_range_t remove_range(selection.m_view.from, selection.m_view.to);
                ustring_t::adjust_active_views(&str, ustring_t::REMOVAL, remove_range);
            }
        }

        static void find_remove(str_slice_t& _str, const str_slice_t& _find)
        {
            str_slice_t strvw(_str);
            str_slice_t sel = find(strvw, _find);
            if (sel.is_empty() == false)
            {
                remove(_str, sel);
            }
        }

        static void find_replace(str_slice_t& str, const str_slice_t& _find, const str_slice_t& replace)
        {
            str_slice_t strvw(str);
            str_slice_t remove = find(strvw, _find);
            if (remove.is_empty() == false)
            {
                s32 const remove_from = remove.m_view.from;
                s32 const remove_len  = remove.size();
                s32 const diff        = remove_len - replace.size();
                if (diff > 0)
                {
                    // The string to replace the selection with is smaller, so we have to remove
                    // some space from the string.
                    runes_t str_runes = ustring_t::get_runes(str.m_data, str.m_view);
                    remove_space(str_runes, remove_from, diff);

                    // TODO: Decision to shrink the allocated memory of m_runes ?

                    str_range_t remove_range(remove_from, remove_from + diff);
                    ustring_t::adjust_active_views(&str, ustring_t::REMOVAL, remove_range);
                }
                else if (diff < 0)
                {
                    // The string to replace the selection with is longer, so we have to insert some
                    // space into the string.
                    ustring_t::resize(str, str.size() + (-diff));
                    runes_t str_runes = ustring_t::get_runes(str.m_data, str.m_view);
                    insert_space(str_runes, remove_from, -diff);

                    str_range_t insert_range(remove_from, remove_from + -diff);
                    ustring_t::adjust_active_views(&str, ustring_t::INSERTION, insert_range);
                }
                // Copy string 'remove' into the (now) same size selection space
                s32       src = 0;
                s32       dst = remove_from;
                s32 const end = remove_from + replace.size();
                switch (str.m_data->m_str_type)
                {
                    case ascii::TYPE:
                    {
                        ascii::prune pdst = str.m_data->m_str_ptr.m_ascii;
                        while (src < replace.size())
                        {
                            pdst[dst++] = replace[src++];
                        }
                    }
                    case utf32::TYPE:
                    {
                        utf32::prune pdst = str.m_data->m_str_ptr.m_utf32;
                        while (src < replace.size())
                        {
                            pdst[dst++] = replace[src++];
                        }
                    }
                }
            }
        }

        static void remove_any(str_slice_t& str, const str_slice_t& any)
        {
            // Remove any of the characters in @charset from @str
            str_range_t const strview = str.m_view;
            s32 const         strsize = strview.size();

            s32 d = 0;
            s32 i = 0;
            s32 r = -1;
            while (i < strsize)
            {
                uchar32 const c = ustring_t::get_char_unsafe(str.m_data, strview, i);
                if (contains(any, c))
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
                        s32 const   gap = r - d;
                        str_range_t removal_range(r - gap, i - gap);
                        ustring_t::adjust_active_views(&str, ustring_t::REMOVAL, removal_range);
                        r = -1;
                    }

                    if (i > d)
                    {
                        ustring_t::set_char_unsafe(str.m_data, strview, d, c);
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
                while (i < str.m_data->m_str_len)
                {
                    uchar32 const c = ustring_t::get_char_unsafe(str.m_data, strview, i);
                    ustring_t::set_char_unsafe(str.m_data, strview, d, c);
                    i++;
                    d++;
                }

                str.m_data->m_str_len -= l;
                switch (str.m_data->m_str_type)
                {
                    case ascii::TYPE:
                    {
                        ascii::prune pdst           = str.m_data->m_str_ptr.m_ascii;
                        pdst[str.m_data->m_str_len] = '\0';
                    }
                    case utf32::TYPE:
                    {
                        utf32::prune pdst           = str.m_data->m_str_ptr.m_utf32;
                        pdst[str.m_data->m_str_len] = '\0';
                    }
                }
            }
        }

        static str_slice_t get_default() { return str_slice_t(); }

        static const s32 NONE     = 0;
        static const s32 LEFT     = 0x0800; // binary(0000,1000,0000,0000);
        static const s32 RIGHT    = 0x0010; // binary(0000,0000,0001,0000);
        static const s32 INSIDE   = 0x0180; // binary(0000,0001,1000,0000);
        static const s32 MATCH    = 0x03C0; // binary(0000,0011,1100,0000);
        static const s32 OVERLAP  = 0x07E0; // binary(0000,0111,1110,0000);
        static const s32 ENVELOPE = 0x37EC; // binary(0011,0111,1110,1100);

        static s32 compare(str_range_t const& lhs, str_range_t const& rhs)
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
            if (lhs.to <= rhs.from)
                return RIGHT;
            else if (lhs.from >= rhs.to)
                return LEFT;
            else if (lhs.from == rhs.from && lhs.to > rhs.to)
                return LEFT | INSIDE;
            else if (lhs.from < rhs.from && lhs.to == rhs.to)
                return RIGHT | INSIDE;
            else if (lhs.from < rhs.from && lhs.to > rhs.to)
                return INSIDE;
            else if (lhs.from == rhs.from && lhs.to == rhs.to)
                return MATCH;
            else if (rhs.from < lhs.from && rhs.to < lhs.to)
                return LEFT | OVERLAP;
            else if (rhs.from > lhs.from && rhs.to > lhs.to)
                return RIGHT | OVERLAP;
            else if (lhs.from == rhs.from && lhs.to < rhs.to)
                return LEFT | ENVELOPE;
            else if (rhs.from < lhs.from && lhs.to == rhs.to)
                return RIGHT | ENVELOPE;
            else if (rhs.from < lhs.from && rhs.to > lhs.to)
                return ENVELOPE;
            return NONE;
        }
        static inline s32 compute_range_overlap(str_range_t const& lhs, str_range_t const& rhs)
        {
            if (rhs.from < lhs.from && rhs.to < lhs.to)
                return rhs.to - lhs.from;
            else if (rhs.from > lhs.from && rhs.to > lhs.to)
                return lhs.to - rhs.from;
            return 0;
        }

        // When the string is modified views can become invalid, here we try to keep them
        // 'correct' as much as we can. For example when we insert text into the string we
        // can correct all of the existing views.
        // Removal of text from a string may invalidate views that intersect with the range
        // of text that is removed.
        // Clear and Reset will invalidate all views on the string.
        static const s32 REMOVAL   = 0;
        static const s32 INSERTION = 1;
        static const s32 CLEARED   = 2;
        static const s32 RELEASED  = 3;
        static void      adjust_active_view(str_slice_t* v, s32 op_code, str_range_t const rhs)
        {
            str_range_t& lhs = v->m_view;

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
                    s32 const c = compare(lhs, rhs);
                    switch (c)
                    {
                        case LEFT:
                            lhs.from -= rhs.size();
                            lhs.to -= rhs.size();
                            break;
                        case INSIDE:
                        case LEFT | INSIDE:
                        case RIGHT | INSIDE: lhs.to -= rhs.size(); break;
                        case MATCH:
                        case ENVELOPE:
                        case LEFT | ENVELOPE:
                        case RIGHT | ENVELOPE: lhs.reset(); break;
                        case LEFT | OVERLAP:
                        {
                            s32 const overlap = compute_range_overlap(lhs, rhs);
                            lhs.from += overlap;
                            lhs.to -= (rhs.size() - overlap);
                            lhs.from -= (rhs.size() - overlap);
                        }
                        break;
                        case RIGHT | OVERLAP: lhs.to -= compute_range_overlap(lhs, rhs); break;
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
                    s32 const c = compare(lhs, rhs);
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
                            lhs.to += rhs.size();
                            lhs.from += rhs.size();
                            break;
                        case INSIDE:
                        case (RIGHT | INSIDE):
                        case (RIGHT | OVERLAP): lhs.to += rhs.size(); break;
                    }
                }
                break;

                case CLEARED:
                case RELEASED:
                {
                    lhs.reset();
                }
                break;
            }
        }

        static void adjust_active_views(str_slice_t* list, s32 op_code, str_range_t op_range)
        {
            str_slice_t* iter = list;
            do
            {
                adjust_active_view(iter, op_code, op_range);
                iter = iter->m_next;
            } while (iter != list);
        }

        static str_data_t  s_default_data;
        static str_data_t* get_default_string_data()
        {
            static str_data_t* s_default_data_ptr = nullptr;
            if (s_default_data_ptr == nullptr)
            {
                s_default_data_ptr                    = &s_default_data;
                s_default_data_ptr->m_str_len         = 0;
                s_default_data_ptr->m_str_ptr.m_ascii = "\0\0\0\0";
                s_default_data_ptr->m_str_type        = ascii::TYPE;
            }
            return s_default_data_ptr;
        }

        static inline bool is_default_string_data(str_data_t* data) { return data == &s_default_data; }
    };
    str_data_t ustring_t::s_default_data;

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------

    bool str_slice_t::is_empty() const { return m_view.size() == 0; }

    void str_slice_t::add_to_list(str_slice_t const* node)
    {
        if (node != nullptr)
        {
            str_slice_t* prev = node->m_next;
            str_slice_t* next = prev->m_next;
            prev->m_next      = this;
            next->m_prev      = this;
            this->m_prev      = prev;
            this->m_next      = next;
        }
        else
        {
            m_next = this;
            m_prev = this;
        }
    }

    void str_slice_t::rem_from_list()
    {
        m_prev->m_next = m_next;
        m_next->m_prev = m_prev;
        m_next         = this;
        m_prev         = this;
    }

    void str_slice_t::invalidate()
    {
        rem_from_list();
        m_data      = nullptr;
        m_view.from = 0;
        m_view.to   = 0;
    }

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------

    string_t string_t::clone() const
    {
        if (m_data->m_str_len > 0)
        {
            str_range_t view(0, m_data->m_str_len);
            runes_t     dstrunes = context_t::string_alloc()->allocate(0, view.size(), m_data->m_str_type);
            crunes_t    srcrunes = ustring_t::get_crunes(this->m_data, view);
            copy(srcrunes, dstrunes);

            string_t str;
            str.m_data                    = (str_data_t*)context_t::system_alloc()->allocate(sizeof(str_data_t), sizeof(void*));
            str.m_data->m_str_ptr.m_ascii = dstrunes.m_ascii.m_bos;
            str.m_data->m_str_len         = view.size();
            str.m_data->m_str_type        = m_data->m_str_type;
            return str;
        }
        return string_t();
    }

    string_t::string_t()
    {
        m_data = ustring_t::get_default_string_data();
        // m_view = range(0, 0);
        // add_to_list(nullptr);
    }

    string_t::~string_t()
    {
        if (!ustring_t::is_default_string_data(m_data))
        {
            if (m_data != nullptr)
                ustring_t::deallocdata(m_data);
        }
    }

    string_t::string_t(const char* str)
    {
        crunes_t srcrunes(str);

        s32 const strlen  = srcrunes.size();
        s32 const strtype = ascii::TYPE;

        str_range_t view = str_range_t(0, strlen);

        if (strlen > 0)
        {
            m_data        = ustring_t::allocdata(strlen, strtype);
            runes_t runes = ustring_t::get_runes(m_data, view);
            copy(srcrunes, runes);
        }
        else
        {
            m_data = ustring_t::get_default_string_data();
        }

        // add_to_list(nullptr);
    }

    string_t::string_t(s32 _len, s32 _type) : m_data(nullptr)
    {
        s32 strlen  = _len;
        s32 strtype = _type;

        if (strlen > 0)
        {
            m_data = ustring_t::allocdata(_len, _type);
        }
        else
        {
            m_data = ustring_t::get_default_string_data();
        }

        // add_to_list(nullptr);
    }

    string_t::string_t(const string_t& other) : m_data(other.m_data)
    //, m_view(other.m_view)
    {
        // add_to_list(&other);
    }

    string_t::string_t(const str_slice_t& left, const str_slice_t& right)
    {
        s32 strlen  = left.size() + right.size();
        s32 strtype = math::max(left.m_data->m_str_type, right.m_data->m_str_type);

        crunes_t leftrunes  = ustring_t::get_crunes(left.m_data, left.m_view);
        crunes_t rightrunes = ustring_t::get_crunes(right.m_data, right.m_view);

        runes_t strdata;
        concatenate(strdata, leftrunes, rightrunes, context_t::string_alloc(), 16);

        m_data                    = (str_data_t*)context_t::system_alloc()->allocate(sizeof(str_data_t), sizeof(void*));
        m_data->m_str_type        = left.m_data->m_str_type;
        m_data->m_str_len         = strdata.cap();
        m_data->m_str_ptr.m_ascii = strdata.m_ascii.m_bos;
    }

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    s32 str_slice_t::size() const { return m_view.size(); }
    s32 str_slice_t::cap() const { return m_data->m_str_len; }

    s32  string_t::size() const { return m_data->m_str_len; }
    s32  string_t::cap() const { return m_data->m_str_len; }
    bool string_t::is_empty() const { return m_data->m_str_len == 0; }

    void string_t::clear() { m_data = ustring_t::get_default_string_data(); }

    str_slice_t string_t::slice() const { return str_slice_t(m_data, str_range_t(0, m_data->m_str_len)); }
    str_slice_t string_t::operator(s32 to) const { return str_slice_t(m_data, str_range_t(0, to)); }
    str_slice_t string_t::operator(s32 from, s32 to) const { return str_slice_t(m_data, str_range_t(from, to)); }

    str_slice_t str_slice_t::operator()(s32 to)
    {
        str_slice_t v;
        v.m_data      = m_data;
        v.m_view.from = m_view.from;
        v.m_view.to   = math::min(m_view.from + to, m_view.to);
        v.add_to_list(this);
        return v;
    }

    str_slice_t str_slice_t::operator()(s32 from, s32 to)
    {
        math::sort(from, to);
        to   = math::min(m_view.from + to, m_view.to);
        from = m_view.from + math::min(from, to);
        str_slice_t v;
        v.m_data      = m_data;
        v.m_view.from = from;
        v.m_view.to   = to;
        v.add_to_list(this);
        return v;
    }

    str_slice_t str_slice_t::operator()(s32 to) const
    {
        str_slice_t v;
        v.m_data      = m_data;
        v.m_view.from = m_view.from;
        v.m_view.to   = math::min(m_view.from + to, m_view.to);
        v.add_to_list(this);
        return v;
    }

    str_slice_t str_slice_t::operator()(s32 from, s32 to) const
    {
        math::sort(from, to);
        to   = math::min(m_view.from + to, m_view.to);
        from = m_view.from + math::min(from, to);
        str_slice_t v;
        v.m_data      = m_data;
        v.m_view.from = from;
        v.m_view.to   = to;
        v.add_to_list(this);
        return v;
    }

    uchar32 string_t::operator[](s32 index) const
    {
        str_range_t view = str_range_t(0, this->m_data->m_str_len);
        if (index >= view.size())
            return '\0';
        return ustring_t::get_char_unsafe(this->m_data, view, index);
    }

    str_slice_t& str_slice_t::operator=(const char* other)
    {
        crunes_t srcrunes(other);

        release();

        s32 strlen  = srcrunes.size();
        s32 strtype = ascii::TYPE;

        if (strlen > 0)
        {
            m_data        = ustring_t::allocdata(strlen, ascii::TYPE);
            runes_t runes = ustring_t::get_runes(m_data, m_view);
            copy(srcrunes, runes);
        }
        else
        {
            m_data = ustring_t::get_default_string_data();
        }

        m_view = str_range_t(0, strlen);
        add_to_list(nullptr);

        return *this;
    }

    str_slice_t& str_slice_t::operator=(const str_slice_t& other)
    {
        if (this != &other)
        {
            if (this->m_data == other.m_data)
            {
                this->m_view = other.m_view;
            }
            else
            {
                clone(other);
            }
        }
        return *this;
    }

    bool str_slice_t::operator==(const str_slice_t& other) const
    {
        if (size() != other.size())
            return false;
        for (s32 i = 0; i < size(); i++)
        {
            uchar32 const lc = ustring_t::get_char_unsafe(this->m_data, this->m_view, i);
            uchar32 const rc = ustring_t::get_char_unsafe(other.m_data, other.m_view, i);
            if (lc != rc)
                return false;
        }
        return true;
    }

    bool str_slice_t::operator!=(const str_slice_t& other) const
    {
        if (size() != other.size())
            return true;
        for (s32 i = 0; i < size(); i++)
        {
            uchar32 const lc = ustring_t::get_char_unsafe(this->m_data, this->m_view, i);
            uchar32 const rc = ustring_t::get_char_unsafe(other.m_data, other.m_view, i);
            if (lc != rc)
                return true;
        }
        return false;
    }

    void str_slice_t::attach(str_slice_t& str)
    {
        str_slice_t* next = str.m_next;
        str.m_next        = this;
        this->m_next      = next;
        this->m_prev      = &str;
        next->m_prev      = this;
    }

    void str_slice_t::release()
    {
        // If we are the only one in the list then it means we can deallocate 'string_data'
        if (m_next == this && m_prev == this)
        {
            if (!ustring_t::is_default_string_data(m_data))
            {
                if (m_data != nullptr)
                    ustring_t::deallocdata(m_data);
            }
        }
        rem_from_list();
        m_data = nullptr;
    }

    void str_slice_t::clone(str_slice_t const& str)
    {
        release();

        crunes_t srcrunes = ustring_t::get_crunes(str.m_data, str.m_view);

        m_data           = ustring_t::allocdata(srcrunes.size(), str.m_data->m_str_type);
        runes_t dstrunes = ustring_t::get_runes(m_data, m_view);
        copy(srcrunes, dstrunes);

        m_view = str_range_t(0, srcrunes.size());
        add_to_list(nullptr);
    }

    //------------------------------------------------------------------------------
    str_slice_t selectUntil(const str_slice_t& str, uchar32 find)
    {
        for (s32 i = 0; i < str.size(); i++)
        {
            uchar32 const c = ustring_t::get_char_unsafe(str, i);
            if (c == find)
            {
                return str(0, i);
            }
        }
        return ustring_t::get_default();
    }

    str_slice_t selectUntil(const str_slice_t& str, const str_slice_t& find)
    {
        str_slice_t v = str(0, find.size());
        while (!v.is_empty())
        {
            if (v == find)
            {
                // So here we have a view with the size of the @find string on
                // string @str that matches the string @find and we need to return
                // a string view that exist before view @v.
                return ustring_t::select_before(str, v);
            }
            if (!ustring_t::move_view(str, v, 1))
                break;
        }
        return ustring_t::get_default();
    }

    str_slice_t selectUntilLast(const str_slice_t& str, uchar32 find)
    {
        for (s32 i = str.size() - 1; i >= 0; --i)
        {
            uchar32 const c = str[i];
            if (c == find)
            {
                return str(0, i);
            }
        }
        return ustring_t::get_default();
    }

    str_slice_t selectUntilLast(const str_slice_t& str, const str_slice_t& find)
    {
        str_slice_t v = str(str.size() - find.size(), str.size());
        while (!v.is_empty())
        {
            if (v == find)
            {
                // So here we have a view with the size of the @find string on
                // string @str that matches the string @find and we need to return
                // a string view that exist before view @v.
                return ustring_t::select_before(str, v);
            }
            if (!ustring_t::move_view(str, v, -1))
                break;
        }
        return ustring_t::get_default();
    }

    str_slice_t selectUntilIncluded(const str_slice_t& str, const str_slice_t& find)
    {
        str_slice_t v = str(0, find.size());
        while (!v.is_empty())
        {
            if (v == find)
            {
                // So here we have a view with the size of the @find string on
                // string @str that matches the string @find and we need to return
                // a string view that exist before and includes view @v.
                return ustring_t::select_before_included(str, v);
            }
            if (!ustring_t::move_view(str, v, 1))
                break;
        }
        return ustring_t::get_default();
    }

    str_slice_t selectUntilEndExcludeSelection(const str_slice_t& str, const str_slice_t& selection) { return ustring_t::select_after_excluded(str, selection); }

    str_slice_t selectUntilEndIncludeSelection(const str_slice_t& str, const str_slice_t& selection) { return ustring_t::select_after(str, selection); }

    bool isUpper(const str_slice_t& str)
    {
        for (s32 i = 0; i < str.size(); i++)
        {
            uchar32 const c = str[i];
            if (is_lower(c))
                return false;
        }
        return true;
    }

    bool isLower(const str_slice_t& str)
    {
        for (s32 i = 0; i < str.size(); i++)
        {
            uchar32 const c = ustring_t::get_char_unsafe(str, i);
            if (is_upper(c))
                return false;
        }
        return true;
    }

    bool isCapitalized(const str_slice_t& str)
    {
        s32 i = 0;
        while (i < str.size())
        {
            uchar32 c = '\0';
            while (i < str.size())
            {
                c = ustring_t::get_char_unsafe(str, i);
                if (!is_space(c))
                    break;
                i += 1;
            }

            if (is_upper(c))
            {
                i += 1;
                while (i < str.size())
                {
                    c = ustring_t::get_char_unsafe(str, i);
                    if (is_space(c))
                        break;
                    if (is_upper(c))
                        return false;
                    i += 1;
                }
            }
            else if (is_alpha(c))
            {
                return false;
            }
            i += 1;
        }
        return true;
    }

    bool isQuoted(const str_slice_t& str) { return isQuoted(str, '"'); }

    bool isQuoted(const str_slice_t& str, uchar32 inQuote) { return isDelimited(str, inQuote, inQuote); }

    bool isDelimited(const str_slice_t& str, uchar32 inLeft, uchar32 inRight)
    {
        if (str.is_empty())
            return false;
        s32 const first = 0;
        s32 const last  = str.size() - 1;
        return str[first] == inLeft && str[last] == inRight;
    }

    uchar32 firstChar(const str_slice_t& str)
    {
        s32 const first = 0;
        return str[first];
    }

    uchar32 lastChar(const str_slice_t& str)
    {
        s32 const last = str.size() - 1;
        return str[last];
    }

    bool startsWith(const str_slice_t& str, str_slice_t const& start)
    {
        str_slice_t v = str(0, start.size());
        if (!v.is_empty())
            return (v == start);
        return false;
    }

    bool endsWith(const str_slice_t& str, str_slice_t const& end)
    {
        str_slice_t v = str(str.size() - end.size(), str.size());
        if (!v.is_empty())
            return (v == end);
        return false;
    }

    str_slice_t find(str_slice_t& str, uchar32 find)
    {
        for (s32 i = 0; i < str.size(); i++)
        {
            uchar32 const c = str[i];
            if (c == find)
                return str(i, i + 1);
        }
        return ustring_t::get_default();
    }

    str_slice_t find(str_slice_t& inStr, const char* inFind)
    {
        crunes_t strfind(inFind);
        crunes_t strstr = ustring_t::get_crunes(inStr);

        crunes_t strfound = find(strstr, strfind);
        if (strfound.is_empty() == false)
        {
            s32 from, to;
            ustring_t::get_from_to(strfound, from, to);
            return inStr(from, to);
        }
        return ustring_t::get_default();
    }

    str_slice_t find(str_slice_t& str, const str_slice_t& find)
    {
        str_slice_t v = str(0, find.size());
        while (!v.is_empty())
        {
            if (v == find)
            {
                // So here we have a view with the size of the @find string on
                // string @str that matches the string @find.
                return v(0, v.size());
            }
            if (!ustring_t::move_view(str, v, 1))
                break;
        }
        return ustring_t::get_default();
    }

    str_slice_t findLast(const str_slice_t& str, const str_slice_t& find)
    {
        str_slice_t v = str(str.size() - find.size(), str.size());
        while (!v.is_empty())
        {
            if (v == find)
            {
                // So here we have a view with the size of the @find string on
                // string @str that matches the string @find.
                return v(0, v.size());
            }
            if (!ustring_t::move_view(str, v, -1))
                break;
        }
        return ustring_t::get_default();
    }

    str_slice_t findOneOf(const str_slice_t& str, const str_slice_t& charset)
    {
        for (s32 i = 0; i < str.size(); i++)
        {
            uchar32 const sc = str[i];
            for (s32 j = 0; j < charset.size(); j++)
            {
                uchar32 const fc = charset[i];
                if (sc == fc)
                {
                    return str(i, i + 1);
                }
            }
        }
        return ustring_t::get_default();
    }

    str_slice_t findOneOfLast(const str_slice_t& str, const str_slice_t& charset)
    {
        for (s32 i = str.size() - 1; i >= 0; i--)
        {
            uchar32 const sc = str[i];
            for (s32 j = 0; j < charset.size(); j++)
            {
                uchar32 const fc = charset[i];
                if (sc == fc)
                {
                    return str(i, i + 1);
                }
            }
        }
        return ustring_t::get_default();
    }

    s32 compare(const str_slice_t& lhs, const str_slice_t& rhs)
    {
        if (lhs.size() < rhs.size())
            return -1;
        if (lhs.size() > rhs.size())
            return 1;

        for (s32 i = 0; i < lhs.size(); i++)
        {
            uchar32 const lc = ustring_t::get_char_unsafe(lhs, i);
            uchar32 const rc = ustring_t::get_char_unsafe(rhs, i);
            if (lc < rc)
                return -1;
            else if (lc > rc)
                return 1;
        }
        return 0;
    }

    bool isEqual(const str_slice_t& lhs, const str_slice_t& rhs) { return compare(lhs, rhs) == 0; }

    bool contains(const str_slice_t& str, const str_slice_t& contains)
    {
        str_slice_t v = str(0, contains.size());
        while (!v.is_empty())
        {
            if (v == contains)
            {
                // So here we have a view with the size of the @find string on
                // string @str that matches the string @find.
                return true;
            }
            if (!ustring_t::move_view(str, v, 1))
                break;
        }
        return false;
    }

    bool contains(const str_slice_t& str, uchar32 contains)
    {
        for (s32 i = 0; i < str.size(); i++)
        {
            uchar32 const sc = ustring_t::get_char_unsafe(str, i);
            if (sc == contains)
            {
                return true;
            }
        }
        return false;
    }

    void concatenate_repeat(str_slice_t& str, str_slice_t const& con, s32 ntimes)
    {
        s32 const len = str.size() + (con.size() * ntimes) + 1;
        ustring_t::resize(str, len);
        for (s32 i = 0; i < ntimes; ++i)
        {
            concatenate(str, con);
        }
    }

    s32 str_slice_t::format(str_slice_t const& format, const va_list_t& args)
    {
        release();

        s32 len = vcprintf(ustring_t::get_crunes(format.m_data, format.m_view), args);

        str_data_t* ndata = ustring_t::allocdata(len, format.m_data->m_str_type);
        m_next            = this;
        m_prev            = this;

        runes_t str = ustring_t::get_runes(ndata, str_range_t(0, 0));
        vsprintf(str, ustring_t::get_crunes(format.m_data, format.m_view), args);
        switch (m_data->m_str_type)
        {
            case ascii::TYPE: m_view.to = (s32)(str.m_ascii.m_end - str.m_ascii.m_bos); break;
            case utf32::TYPE: m_view.to = (s32)(str.m_utf32.m_end - str.m_utf32.m_bos); break;
        }
        return len;
    }

    s32 str_slice_t::formatAdd(str_slice_t const& format, const va_list_t& args)
    {
        s32 len = vcprintf(ustring_t::get_crunes(format.m_data, format.m_view), args);
        ustring_t::resize(*this, len);
        runes_t str       = ustring_t::get_runes(m_data, m_view);
        str.m_ascii.m_str = str.m_ascii.m_end;
        vsprintf(str, ustring_t::get_crunes(format.m_data, format.m_view), args);
        switch (m_data->m_str_type)
        {
            case ascii::TYPE: m_view.to = (s32)(str.m_ascii.m_end - str.m_ascii.m_bos); break;
            case utf32::TYPE: m_view.to = (s32)(str.m_utf32.m_end - str.m_utf32.m_bos); break;
        }
        return len;
    }

    void insert(str_slice_t& str, str_slice_t const& pos, str_slice_t const& insert) { ustring_t::insert(str, pos, insert); }

    void insert_after(str_slice_t& str, str_slice_t const& pos, str_slice_t const& insert)
    {
        str_slice_t after = ustring_t::select_after_excluded(str, pos);
        ustring_t::insert(str, after, insert);
    }

    void remove(str_slice_t& str, str_slice_t const& selection) { ustring_t::remove(str, selection); }

    void find_remove(str_slice_t& str, const str_slice_t& _find)
    {
        str_slice_t sel = find(str, _find);
        if (sel.is_empty() == false)
        {
            ustring_t::remove(str, sel);
        }
    }

    void find_replace(str_slice_t& str, const str_slice_t& find, const str_slice_t& replace) { ustring_t::find_replace(str, find, replace); }

    void remove_any(str_slice_t& str, const str_slice_t& any) { ustring_t::remove_any(str, any); }

    void replace_any(str_slice_t& str, const str_slice_t& any, uchar32 with)
    {
        // Replace any of the characters in @charset from @str with character @with
        for (s32 i = 0; i < str.size(); ++i)
        {
            uchar32 const c = ustring_t::get_char_unsafe(str, i);
            if (contains(any, c))
            {
                ustring_t::set_char_unsafe(str, i, with);
            }
        }
    }

    void upper(str_slice_t& str)
    {
        for (s32 i = 0; i < str.size(); i++)
        {
            uchar32 c = ustring_t::get_char_unsafe(str, i);
            c         = to_upper(c);
            ustring_t::set_char_unsafe(str, i, c);
        }
    }

    void lower(str_slice_t& str)
    {
        for (s32 i = 0; i < str.size(); i++)
        {
            uchar32 c = ustring_t::get_char_unsafe(str, i);
            c         = to_lower(c);
            ustring_t::set_char_unsafe(str, i, c);
        }
    }

    void capitalize(str_slice_t& str)
    {
        // Standard separator is ' '
        bool prev_is_space = true;
        s32  i             = 0;
        while (i < str.size())
        {
            uchar32 c = ustring_t::get_char_unsafe(str, i);
            uchar32 d = c;
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

            if (c != d)
            {
                ustring_t::set_char_unsafe(str, i, c);
            }
            i++;
        }
    }

    void capitalize(str_slice_t& str, str_slice_t const& separators)
    {
        bool prev_is_space = false;
        s32  i             = 0;
        while (i < str.size())
        {
            uchar32 c = ustring_t::get_char_unsafe(str, i);
            uchar32 d = c;
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
                ustring_t::set_char_unsafe(str, i, c);
            }
            i++;
        }
    }

    // Trim does nothing more than narrowing the <from, to>, nothing is actually removed
    // from the actual underlying string string_data.
    void trim(str_slice_t& str)
    {
        trimLeft(str);
        trimRight(str);
    }

    void trimLeft(str_slice_t& str)
    {
        for (s32 i = 0; i < str.size(); ++i)
        {
            uchar32 c = ustring_t::get_char_unsafe(str, i);
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
            {
                if (i > 0)
                {
                    ustring_t::narrow_view(str, -i);
                }
                return;
            }
        }
    }

    void trimRight(str_slice_t& str)
    {
        s32 const last = str.size() - 1;
        for (s32 i = 0; i < str.size(); ++i)
        {
            uchar32 c = ustring_t::get_char_unsafe(str, last - i);
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
            {
                if (i > 0)
                {
                    ustring_t::narrow_view(str, i);
                }
                return;
            }
        }
    }

    void trim(str_slice_t& str, uchar32 r)
    {
        trimLeft(str, r);
        trimRight(str, r);
    }

    void trimLeft(str_slice_t& str, uchar32 r)
    {
        for (s32 i = 0; i < str.size(); ++i)
        {
            uchar32 c = ustring_t::get_char_unsafe(str, i);
            if (c != r)
            {
                if (i > 0)
                {
                    ustring_t::narrow_view(str, -i);
                }
                return;
            }
        }
    }

    void trimRight(str_slice_t& str, uchar32 r)
    {
        s32 const last = str.size() - 1;
        for (s32 i = 0; i < str.size(); ++i)
        {
            uchar32 c = ustring_t::get_char_unsafe(str, last - i);
            if (c != r)
            {
                if (i > 0)
                {
                    ustring_t::narrow_view(str, i);
                }
                return;
            }
        }
    }

    void trim(str_slice_t& str, str_slice_t const& set)
    {
        trimLeft(str, set);
        trimRight(str, set);
    }

    void trimLeft(str_slice_t& str, str_slice_t const& set)
    {
        for (s32 i = 0; i < str.size(); ++i)
        {
            uchar32 c = ustring_t::get_char_unsafe(str, i);
            if (!contains(set, c))
            {
                if (i > 0)
                {
                    ustring_t::narrow_view(str, -i);
                }
                return;
            }
        }
    }

    void trimRight(str_slice_t& str, str_slice_t const& set)
    {
        s32 const last = str.size() - 1;
        for (s32 i = 0; i < str.size(); ++i)
        {
            uchar32 c = ustring_t::get_char_unsafe(str, last - i);
            if (!contains(set, c))
            {
                if (i > 0)
                {
                    ustring_t::narrow_view(str, i);
                }
                return;
            }
        }
    }

    void trimQuotes(str_slice_t& str) { trimDelimiters(str, '"', '"'); }

    void trimQuotes(str_slice_t& str, uchar32 quote) { trimDelimiters(str, quote, quote); }

    void trimDelimiters(str_slice_t& str, uchar32 left, uchar32 right)
    {
        trimLeft(str, left);
        trimRight(str, right);
    }

    void reverse(str_slice_t& str)
    {
        s32 const last = str.size() - 1;
        for (s32 i = 0; i < (last - i); ++i)
        {
            uchar32 l = ustring_t::get_char_unsafe(str, i);
            uchar32 r = ustring_t::get_char_unsafe(str, last - i);
            ustring_t::set_char_unsafe(str, i, r);
            ustring_t::set_char_unsafe(str, last - i, l);
        }
    }

    bool splitOn(str_slice_t& str, uchar32 inChar, str_slice_t& outLeft, str_slice_t& outRight)
    {
        outLeft = selectUntil(str, inChar);
        if (outLeft.is_empty())
            return false;
        outRight = str(outLeft.size(), str.size());
        trimRight(outLeft, inChar);
        return true;
    }

    bool splitOn(str_slice_t& str, str_slice_t& inStr, str_slice_t& outLeft, str_slice_t& outRight)
    {
        outLeft = selectUntil(str, inStr);
        if (outLeft.is_empty())
            return false;
        outRight = str(outLeft.size() + inStr.size(), str.size());
        return true;
    }

    bool splitOnLast(str_slice_t& str, uchar32 inChar, str_slice_t& outLeft, str_slice_t& outRight)
    {
        outLeft = selectUntilLast(str, inChar);
        if (outLeft.is_empty())
            return false;
        outRight = str(outLeft.size(), str.size());
        trimRight(outLeft, inChar);
        return true;
    }

    bool splitOnLast(str_slice_t& str, str_slice_t& inStr, str_slice_t& outLeft, str_slice_t& outRight)
    {
        outLeft = selectUntilLast(str, inStr);
        if (outLeft.is_empty())
            return false;
        outRight = str(outLeft.size() + inStr.size(), str.size());
        return true;
    }

    void concatenate(str_slice_t& str, const str_slice_t& con) { ustring_t::resize(str, str.size() + con.size() + 1); }

    //------------------------------------------------------------------------------
    static void user_case_for_string()
    {
        string_t str("This is an ascii string that will be converted to UTF-32");

        str_slice_t strvw  = str.slice();
        string_t    ascii  = ("ascii");
        str_slice_t substr = find(strvw, ascii.slice());
        upper(substr);

        string_t converted("converted ");
        find_remove(strvw, converted.slice());
    }

} // namespace ncore
