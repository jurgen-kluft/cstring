#include "xbase/x_allocator.h"
#include "xbase/x_integer.h"
#include "xbase/x_memory.h"
#include "xbase/x_runes.h"
#include "xbase/x_printf.h"
#include "xstring/x_string.h"

namespace xcore
{
    struct xstring::data
    {
        inline data() : m_alloc(nullptr), m_str_alloc(nullptr), m_str_ptr(), m_str_type(ascii::TYPE), m_str_len(0) {}
        inline data(alloc_t* a, runes_alloc_t* sa) : m_alloc(a), m_str_alloc(sa), m_str_ptr(), m_str_type(ascii::TYPE), m_str_len(0) {}
        alloc_t*       m_alloc;
        runes_alloc_t* m_str_alloc;
        runes_t::ptr_t m_str_ptr;
        s32            m_str_type;
        s32            m_str_len;

        XCORE_CLASS_PLACEMENT_NEW_DELETE
    };

    //==============================================================================
    class ustring
    {
    public:
        //------------------------------------------------------------------------------
        //------------------------------------------------------------------------------
        class runes_allocator : public runes_alloc_t
        {
        public:
            virtual runes_t allocate(s32 len, s32 cap, s32 type)
            {
                if (len > cap)
                    cap = len;

                ASSERT(type == utf32::TYPE);
                u32 runesize = 4;
                switch (type)
                {
                    case ascii::TYPE: runesize = 1; break;
                    case utf32::TYPE: runesize = 1; break;
                }

                void* str = alloc_t::get_system()->allocate(cap * runesize, sizeof(void*));

                runes_t r;
                r.m_type                = type;
                r.m_runes.m_ascii.m_bos = (ascii::prune)str;
                r.m_runes.m_ascii.m_str = (ascii::prune)str;
                switch (type)
                {
                    case ascii::TYPE:
                        r.m_runes.m_ascii.m_end = (ascii::prune)str + len;
                        r.m_runes.m_ascii.m_eos = (ascii::prune)str + cap - 1;
                        break;
                    case utf32::TYPE:
                        r.m_runes.m_utf32.m_end = (utf32::prune)str + len;
                        r.m_runes.m_utf32.m_eos = (utf32::prune)str + cap - 1;
                        break;
                }

                switch (type)
                {
                    case ascii::TYPE:
                        r.m_runes.m_ascii.m_end[0]       = '\0';
                        r.m_runes.m_ascii.m_end[cap - 1] = '\0';
                        break;
                    case utf32::TYPE:
                        r.m_runes.m_utf32.m_end[0]       = '\0';
                        r.m_runes.m_utf32.m_end[cap - 1] = '\0';
                        break;
                }

                r.m_runes.m_utf32.m_end[0]       = '\0';
                r.m_runes.m_utf32.m_end[cap - 1] = '\0';
                return r;
            }

            virtual void deallocate(runes_t& r)
            {
                if (r.m_runes.m_utf32.m_bos != nullptr)
                {
                    alloc_t::get_system()->deallocate(r.m_runes.m_utf32.m_bos);
                    r = runes_t();
                }
            }
        };

        static runes_allocator s_default_stralloc;

        static inline uchar32 get_char_unsafe(xstring const& str, s32 i)
        {
            switch (str.m_data->m_str_type)
            {
                case ascii::TYPE:
                {
                    ascii::pcrune s = str.m_data->m_str_ptr.m_ptr.m_ascii + str.m_view.from;
                    return s[i];
                }
                break;
                case utf32::TYPE:
                {
                    utf32::pcrune s = str.m_data->m_str_ptr.m_ptr.m_utf32 + str.m_view.from;
                    return s[i];
                }
                break;
            }
            return '\0';
        }

        static inline void set_char_unsafe(xstring& str, s32 i, uchar32 c)
        {
            switch (str.m_data->m_str_type)
            {
                case ascii::TYPE:
                {
                    ascii::prune s = str.m_data->m_str_ptr.m_ptr.m_ascii + str.m_view.from;
                    s[i]           = (ascii::rune)c;
                }
                break;
                case utf32::TYPE:
                {
                    utf32::prune s = str.m_data->m_str_ptr.m_ptr.m_utf32 + str.m_view.from;
                    s[i]           = c;
                }
                break;
            }
        }

        static inline crunes_t get_crunes(xstring::data const* data, xstring::range view)
        {
            crunes_t r;
            r.m_type = data->m_str_type;
            switch (data->m_str_type)
            {
                case ascii::TYPE:
                    r.m_runes.m_ascii.m_bos = data->m_str_ptr.m_ptr.m_ascii;
                    r.m_runes.m_ascii.m_eos = r.m_runes.m_ascii.m_bos + data->m_str_len;
                    r.m_runes.m_ascii.m_str = r.m_runes.m_ascii.m_bos + view.from;
                    r.m_runes.m_ascii.m_end = r.m_runes.m_ascii.m_bos + view.to;
                    break;
                case utf32::TYPE:
                    r.m_runes.m_utf32.m_bos = data->m_str_ptr.m_ptr.m_utf32;
                    r.m_runes.m_utf32.m_eos = r.m_runes.m_utf32.m_bos + data->m_str_len;
                    r.m_runes.m_utf32.m_str = r.m_runes.m_utf32.m_bos + view.from;
                    r.m_runes.m_utf32.m_end = r.m_runes.m_utf32.m_bos + view.to;
                    break;
            }
            return r;
        }

        static inline runes_t get_runes(xstring::data* data, xstring::range view)
        {
            runes_t r;
            r.m_type = data->m_str_type;
            switch (data->m_str_type)
            {
                case ascii::TYPE:
                    r.m_runes.m_ascii.m_bos = data->m_str_ptr.m_ptr.m_ascii;
                    r.m_runes.m_ascii.m_eos = r.m_runes.m_ascii.m_bos + data->m_str_len;
                    r.m_runes.m_ascii.m_str = r.m_runes.m_ascii.m_bos + view.from;
                    r.m_runes.m_ascii.m_end = r.m_runes.m_ascii.m_bos + view.to;
                    break;
                case utf32::TYPE:
                    r.m_runes.m_utf32.m_bos = data->m_str_ptr.m_ptr.m_utf32;
                    r.m_runes.m_utf32.m_eos = r.m_runes.m_utf32.m_bos + data->m_str_len;
                    r.m_runes.m_utf32.m_str = r.m_runes.m_utf32.m_bos + view.from;
                    r.m_runes.m_utf32.m_end = r.m_runes.m_utf32.m_bos + view.to;
                    break;
            }
            return r;
        }

        static xstring::data* allocdata(alloc_t* _alloc, runes_alloc_t* _stralloc, s32 _strlen, s32 _strtype)
        {
            xstring::data* data = _alloc->construct<xstring::data>();
            data->m_alloc       = _alloc;
            data->m_str_alloc   = _stralloc;
            data->m_str_type    = _strtype;
            data->m_str_len     = _strlen;

            runes_t strdata               = _stralloc->allocate(_strlen, _strlen, _strtype);
            data->m_str_ptr.m_ptr.m_ascii = strdata.m_runes.m_ascii.m_bos;

            return data;
        }

        static void deallocdata(xstring::data* data, alloc_t* _alloc, runes_alloc_t* _stralloc)
        {
            runes_t runes = get_runes(data, xstring::range());
            _stralloc->deallocate(runes);
            _alloc->deallocate(data);
        }

        static void resize(xstring& str, s32 new_size)
        {
            if (new_size > str.cap())
            {
                xstring::data* data = allocdata(str.m_data->m_alloc, str.m_data->m_str_alloc, new_size, str.m_data->m_str_type);

                runes_t trunes = get_runes(str.m_data, xstring::range(0, str.m_data->m_str_len));
                runes_t nrunes = get_runes(data, xstring::range(0, data->m_str_len));
                copy(trunes, nrunes);

                // deallocate old data and string
                deallocdata(str.m_data, str.m_data->m_alloc, str.m_data->m_str_alloc);

                // need to update the new data pointer for each view
                xstring* list = &str;
                xstring* iter = list;
                do {
                    iter->m_data = data;
                    iter = iter->m_next;
                } while (iter != list);


            }
            else
            {
                // TODO: What about shrinking ?
            }
        }

        static bool is_part_of(xstring const& str, xstring const& part)
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
        static bool str_has_view(xstring const& str, xstring const& part) { return is_part_of(str, part); }

        static bool narrow_view(xstring& v, s32 move)
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

        static bool move_view(xstring const& str, xstring& view, s32 move)
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

        static xstring select_before(const xstring& str, const xstring& selection)
        {
            xstring v(selection);
            v.m_view.to   = v.m_view.from;
            v.m_view.from = 0;
            return v;
        }
        static xstring select_before_included(const xstring& str, const xstring& selection)
        {
            xstring v(selection);
            v.m_view.to   = v.m_view.to;
            v.m_view.from = 0;
            return v;
        }

        static xstring select_after(const xstring& str, const xstring& sel)
        {
            xstring v(sel);
            v.m_view.to   = str.m_view.to;
            v.m_view.from = sel.m_view.from;
            return v;
        }

        static xstring select_after_excluded(const xstring& str, const xstring& sel)
        {
            xstring v(sel);
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
                    r.m_runes.m_ascii.m_str[dst--] = r.m_runes.m_ascii.m_str[src--];
                }
                r.m_runes.m_ascii.m_end += len;
                r.m_runes.m_ascii.m_end[0] = '\0';
                break;
            case utf32::TYPE:
                while (src >= pos)
                {
                    r.m_runes.m_utf32.m_str[dst--] = r.m_runes.m_utf32.m_str[src--];
                }
                r.m_runes.m_utf32.m_end += len;
                r.m_runes.m_utf32.m_end[0] = '\0';
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
                    r.m_runes.m_ascii.m_str[dst++] = r.m_runes.m_ascii.m_str[src++];
                }
                r.m_runes.m_ascii.m_end -= len;
                r.m_runes.m_ascii.m_end[0] = '\0';
                break;
            case utf32::TYPE:
                while (src < r.size())
                {
                    r.m_runes.m_utf32.m_str[dst++] = r.m_runes.m_utf32.m_str[src++];
                }
                r.m_runes.m_utf32.m_end -= len;
                r.m_runes.m_utf32.m_end[0] = '\0';
                break;
            }
        }

        static void insert(xstring& str, xstring const& pos, xstring const& insert)
        {
            if (insert.is_empty() || (str.m_data!=pos.m_data))
                return;

            s32 dst = pos.m_view.from;

            xstring::range insert_range(dst, dst + insert.size());
            ustring::adjust_active_views(&str, ustring::INSERTION, insert_range);

            s32 const current_len = str.m_data->m_str_len;
            ustring::resize(str, str.m_data->m_str_len + insert.size() + 1);
            {
                //@TODO: it should be better to get an actual full view from the list of strings, currently we
                //       take the easy way and just take the whole allocated size as the full view.
                runes_t str_runes = ustring::get_runes(str.m_data, xstring::range(0, current_len));
                insert_space(str_runes, dst, insert.size());
            }
            s32 src = 0;
            while (src < insert.size())
            {
                uchar32 const c = ustring::get_char_unsafe(insert, src);
                ustring::set_char_unsafe(str, dst, c);
                ++src;
                ++dst;
            }
        }

        static void remove(xstring& str, xstring const& selection)
        {
            if (selection.is_empty() || (str.m_data!=selection.m_data))
                return;
            if (ustring::str_has_view(str, selection))
            {
                //@TODO: it should be better to get an actual full view from the list of strings, currently we
                //       take the easy way and just take the whole allocated size as the full view.
                runes_t str_runes = get_runes(str.m_data, xstring::range(0, str.m_data->m_str_len));
                remove_space(str_runes, selection.m_view.from, selection.size());

                // TODO: Decision to shrink the allocated memory of m_runes ?

                xstring::range remove_range(selection.m_view.from, selection.m_view.to);
                ustring::adjust_active_views(&str, ustring::REMOVAL, remove_range);
            }
        }

        static void find_remove(xstring& _str, const xstring& _find)
        {
            xstring strvw(_str);
            xstring sel = find(strvw, _find);
            if (sel.is_empty() == false)
            {
                remove(_str, sel);
            }
        }

        static void find_replace(xstring& str, const xstring& _find, const xstring& replace)
        {
            xstring strvw(str);
            xstring remove = find(strvw, _find);
            if (remove.is_empty() == false)
            {
                s32 const remove_from = remove.m_view.from;
                s32 const remove_len  = remove.size();
                s32 const diff        = remove_len - replace.size();
                if (diff > 0)
                {
                    // The string to replace the selection with is smaller, so we have to remove
                    // some space from the string.
                    runes_t str_runes = ustring::get_runes(str.m_data, str.m_view);
                    remove_space(str_runes, remove_from, diff);

                    // TODO: Decision to shrink the allocated memory of m_runes ?

                    xstring::range remove_range(remove_from, remove_from + diff);
                    ustring::adjust_active_views(&str, ustring::REMOVAL, remove_range);
                }
                else if (diff < 0)
                {
                    // The string to replace the selection with is longer, so we have to insert some
                    // space into the string.
                    ustring::resize(str, str.size() + (-diff));
                    runes_t str_runes = ustring::get_runes(str.m_data, str.m_view);
                    insert_space(str_runes, remove_from, -diff);

                    xstring::range insert_range(remove_from, remove_from + -diff);
                    ustring::adjust_active_views(&str, ustring::INSERTION, insert_range);
                }
                // Copy string 'remove' into the (now) same size selection space
                s32       src = 0;
                s32       dst = remove_from;
                s32 const end = remove_from + replace.size();
                switch (str.m_data->m_str_type)
                {
                    case ascii::TYPE:
                    {
                        ascii::prune pdst = str.m_data->m_str_ptr.m_ptr.m_ascii;
                        while (src < replace.size())
                        {
                            pdst[dst++] = replace[src++];
                        }
                    }
                    case utf32::TYPE:
                    {
                        utf32::prune pdst = str.m_data->m_str_ptr.m_ptr.m_utf32;
                        while (src < replace.size())
                        {
                            pdst[dst++] = replace[src++];
                        }
                    }
                }
            }
        }

        static void remove_any(xstring& str, const xstring& any)
        {
            // Remove any of the characters in @charset from @str
            s32       d   = 0;
            s32       i   = 0;
            s32 const end = str.size();
            s32       r   = -1;
            while (i < end)
            {
                uchar32 const c = ustring::get_char_unsafe(str, i);
                if (contains(any, c))
                {
                    r = i;
                    i++;
                }
                else
                {
                    if (r >= 0)
                    { // This might not be the first character(s)/range removed, if not then the views already
                        // have been adjusted according to the previous range removal. So here we have to adjust
                        // this removal range by shifting it left with 'gap'.
                        s32 const      gap = i - d;
                        xstring::range removal_range(r, i);
                        ustring::shift_range(removal_range, gap);
                        ustring::adjust_active_views(&str, ustring::REMOVAL, removal_range);
                        r = -1;
                    }

                    if (i > d)
                    {
                        ustring::set_char_unsafe(str, d, c);
                    }
                    i++;
                    d++;
                }
            }
            s32 const l = i - d;
            if (l > 0)
            {
                str.m_data->m_str_len -= l;
                switch (str.m_data->m_str_type)
                {
                    case ascii::TYPE:
                    {
                        ascii::prune pdst           = str.m_data->m_str_ptr.m_ptr.m_ascii;
                        pdst[str.m_data->m_str_len] = '\0';
                    }
                    case utf32::TYPE:
                    {
                        utf32::prune pdst           = str.m_data->m_str_ptr.m_ptr.m_utf32;
                        pdst[str.m_data->m_str_len] = '\0';
                    }
                }
            }
        }

        static xstring get_default() { return xstring(); }

        static const s32 NONE     = 0;
        static const s32 LEFT     = 0x0800; // binary(0000,1000,0000,0000);
        static const s32 RIGHT    = 0x0010; // binary(0000,0000,0001,0000);
        static const s32 INSIDE   = 0x0180; // binary(0000,0001,1000,0000);
        static const s32 MATCH    = 0x03C0; // binary(0000,0011,1100,0000);
        static const s32 OVERLAP  = 0x07E0; // binary(0000,0111,1110,0000);
        static const s32 ENVELOPE = 0x37EC; // binary(0011,0111,1110,1100);

        static s32 compare(xstring::range const& lhs, xstring::range const& rhs)
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
        static inline s32 compute_range_overlap(xstring::range const& lhs, xstring::range const& rhs)
        {
            if (rhs.from < lhs.from && rhs.to < lhs.to)
                return rhs.to - lhs.from;
            else if (rhs.from > lhs.from && rhs.to > lhs.to)
                return lhs.to - rhs.from;
            return 0;
        }
        static inline void shift_range(xstring::range& v, s32 distance)
        {
            v.from += distance;
            v.to += distance;
        }
        static inline void extend_range(xstring::range& v, s32 distance)
        {
            if (distance > 0)
            { // Extend right side
                v.to += distance;
            }
            else
            { // Extend left side
                v.from += distance;
            }
        }
        static inline void narrow_range(xstring::range& v, s32 distance)
        {
            if (distance > 0)
            { // Narrow right side
                v.to -= distance;
            }
            else
            { // Narrow left side
                v.from -= distance;
            }
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
        static void      adjust_active_view(xstring* v, s32 op_code, xstring::range op_range)
        {
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
                    s32 const c = compare(v->m_view, op_range);
                    switch (c)
                    {
                        case LEFT: shift_range(v->m_view, -op_range.size()); break;
                        case INSIDE: narrow_range(v->m_view, op_range.size()); break;
                        case LEFT | INSIDE:
                        case RIGHT | INSIDE: extend_range(v->m_view, -op_range.size()); break;
                        case MATCH:
                        case ENVELOPE:
                        case LEFT | ENVELOPE:
                        case RIGHT | ENVELOPE: v->m_view.reset(); break;
                        case LEFT | OVERLAP:
                            extend_range(v->m_view, -(compute_range_overlap(v->m_view, op_range)));
                            shift_range(v->m_view, -(op_range.size() - compute_range_overlap(v->m_view, op_range)));
                            break;
                        case RIGHT | OVERLAP: extend_range(v->m_view, compute_range_overlap(v->m_view, op_range)); break;
                        case RIGHT: break;
                    }
                }
                break;

                case INSERTION:
                {
                    // --------| lhs |--------[ rhs ]--------        RIGHT = DO NOTHING
                    // --------[ rhs ]--------| lhs |--------        LEFT = SHIFT RIGHT by rhs.size()
                    // --------[ rhs ]          lhs |--------        LEFT INSIDE = SHIFT RIGHT by rhs.size()
                    // --------|     lhs      [ rhs ]--------        RIGHT INSIDE = EXTEND RIGHT by rhs.size()
                    // --------|    [ rhs ]     lhs |--------        INSIDE = EXTEND RIGHT by rhs.size()
                    // --------[     rhs  lhs       ]--------        MATCH = SHIFT RIGHT by rhs.size()
                    // --------[ rhs    |    ]  lhs |--------        LEFT OVERLAP = SHIFT RIGHT by rhs.size()
                    // --------| lhs    [    |  rhs ]--------        RIGHT OVERLAP = EXTEND RIGHT by rhs.size()
                    // --------[    | lhs |     rhs ]--------        ENVELOPE = SHIFT RIGHT by rhs.size()
                    // --------[ lhs |          rhs ]--------        LEFT ENVELOPE = SHIFT RIGHT by rhs.size()
                    // --------[ rhs         |  lhs ]--------        RIGHT ENVELOPE = SHIFT RIGHT by rhs.size()
                    s32 const c = compare(v->m_view, op_range);
                    switch (c)
                    {
                        case MATCH:
                        case LEFT:
                        case ENVELOPE:
                        case (LEFT | ENVELOPE):
                        case (RIGHT | ENVELOPE):
                        case (LEFT | OVERLAP):
                        case (LEFT | INSIDE): shift_range(v->m_view, op_range.size()); break;
                        case INSIDE:
                        case (RIGHT | INSIDE):
                        case (RIGHT | OVERLAP): extend_range(v->m_view, op_range.size()); break;
                        case RIGHT: break;
                    }
                }
                break;

                case CLEARED:
                case RELEASED:
                {
                    v->m_view.reset();
                }
                break;
            }
        }

        static void adjust_active_views(xstring* list, s32 op_code, xstring::range op_range)
        {
            xstring* iter = list;
            do {
                adjust_active_view(iter, op_code, op_range);
                iter = iter->m_next;
            } while (iter != list);
        }

        static xstring::data s_default_data;
        static xstring::data* get_default_string_data()
        {
            static xstring::data* s_default_data_ptr = nullptr;
            if (s_default_data_ptr == nullptr)
            {
                s_default_data_ptr = &s_default_data;
                s_default_data_ptr->m_alloc = alloc_t::get_main();
                s_default_data_ptr->m_str_alloc = runes_alloc_t::get_main();
                s_default_data_ptr->m_str_len = 0;
                s_default_data_ptr->m_str_ptr.m_ptr.m_ascii = "\0\0\0\0";
                s_default_data_ptr->m_str_type = ascii::TYPE;
            }
            return s_default_data_ptr;
        }

        static inline bool is_default_string_data(xstring::data* data)
        {
            return data == &s_default_data;
        }
    };
    xstring::data ustring::s_default_data;

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------

    bool xstring::is_empty() const { return m_view.size() == 0; }

    void xstring::add_to_list(xstring const* node)
    {
        if (node != nullptr)
        {
            xstring* prev = node->m_next;
            xstring* next = prev->m_next;
            prev->m_next  = this;
            next->m_prev  = this;
            this->m_prev  = prev;
            this->m_next  = next;
        }
        else
        {
            m_next = this;
            m_prev = this;
        }
    }

    void xstring::rem_from_list()
    {
        if (m_next != this)
        {
            m_prev->m_next = m_next;
            m_next->m_prev = m_prev;
        }
        else
        {
            m_next = nullptr;
            m_prev = nullptr;
        }
    }

    void xstring::invalidate()
    {
        rem_from_list();
        m_data      = nullptr;
        m_view.from = 0;
        m_view.to   = 0;
    }

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------

    xstring xstring::clone() const
    {
        if (m_data->m_str_alloc != nullptr && m_data->m_str_len > 0)
        {
            runes_t  dstrunes = m_data->m_str_alloc->allocate(0, m_view.size(), m_data->m_str_type);
            crunes_t srcrunes = ustring::get_crunes(this->m_data, this->m_view);
            copy(srcrunes, dstrunes);

            xstring str;
            str.m_data                          = m_data->m_alloc->construct<xstring::data>();
            str.m_data->m_str_alloc             = m_data->m_str_alloc;
            str.m_data->m_alloc                 = m_data->m_alloc;
            str.m_data->m_str_ptr.m_ptr.m_ascii = dstrunes.m_runes.m_ascii.m_bos;
            str.m_data->m_str_len               = m_view.size();
            str.m_data->m_str_type              = m_data->m_str_type;
            str.m_view                          = m_view;
            str.m_next                          = &str;
            str.m_prev                          = &str;
            return str;
        }
        return xstring();
    }

    xstring::xstring() : m_data(nullptr), m_view()
    {
        s32 strlen  = 0;
        s32 strtype = ascii::TYPE;

        m_data = ustring::get_default_string_data();
        m_view = range(0, strlen);
        add_to_list(nullptr);
    }

    xstring::~xstring() { release(); }

    xstring::xstring(const char* str) : m_data(nullptr), m_view()
    {
        crunes_t srcrunes(str);

        s32 strlen  = srcrunes.size();
        s32 strtype = ascii::TYPE;

        if (strlen > 0)
        {
            m_data = ustring::allocdata(alloc_t::get_main(), runes_alloc_t::get_main(), strlen, ascii::TYPE);
            runes_t runes = ustring::get_runes(m_data, m_view);
            copy(srcrunes, runes);
        }
        else
        {
            m_data = ustring::get_default_string_data();
        }

        m_view = range(0, strlen);
        add_to_list(nullptr);
    }

    xstring::xstring(alloc_t* _alloc, runes_alloc_t* _stralloc, const char* str) : m_data(nullptr), m_view()
    {
        crunes_t srcrunes(str);

        s32 strlen  = srcrunes.size();
        s32 strtype = ascii::TYPE;

        if (strlen > 0)
        {
            m_data = ustring::allocdata(_alloc, _stralloc, strlen, srcrunes.m_type);
            runes_t runes = ustring::get_runes(m_data, m_view);
            copy(srcrunes, runes);
        }
        else
        {
            m_data = ustring::get_default_string_data();
        }

        m_view.from = 0;
        m_view.to   = strlen;

        add_to_list(nullptr);
    }

    xstring::xstring(alloc_t* _alloc, runes_alloc_t* _stralloc, s32 _len, s32 _type) : m_data(nullptr), m_view()
    {
        s32 strlen  = _len;
        s32 strtype = _type;

        if (strlen > 0)
        {
            m_data = ustring::allocdata(_alloc, _stralloc, _len, _type);
        }
        else
        {
            m_data = ustring::get_default_string_data();
        }

        m_view.from = 0;
        m_view.to   = 0;

        add_to_list(nullptr);
    }

    xstring::xstring(const xstring& other) : m_data(other.m_data), m_view(other.m_view)
    {
        add_to_list(&other);
    }

    xstring::xstring(const xstring& left, const xstring& right)
    {
        s32 strlen  = left.size() + right.size() + 1;
        s32 strtype = xmax(left.m_data->m_str_type, right.m_data->m_str_type);

        m_data              = left.m_data->m_alloc->construct<xstring::data>();
        m_data->m_alloc     = left.m_data->m_alloc;
        m_data->m_str_alloc = left.m_data->m_str_alloc;
        m_data->m_str_type  = left.m_data->m_str_type;

        crunes_t leftrunes  = ustring::get_crunes(left.m_data, left.m_view);
        crunes_t rightrunes = ustring::get_crunes(right.m_data, right.m_view);

        runes_t strdata;
        concatenate(strdata, leftrunes, rightrunes, m_data->m_str_alloc, 16);

        m_data->m_str_len               = strdata.cap();
        m_data->m_str_ptr.m_ptr.m_ascii = strdata.m_runes.m_ascii.m_bos;

        m_view.from = 0;
        m_view.to   = m_data->m_str_len;

        add_to_list(nullptr);
    }

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------

    s32 xstring::size() const { return m_view.size(); }
    s32 xstring::cap() const { return m_data->m_str_len; }

    void xstring::clear()
    {
        m_view.from = 0;
        m_view.to   = 0;
    }

    xstring xstring::operator()(s32 to)
    {
        xstring v;
        v.m_data      = m_data;
        v.m_view.from = m_view.from;
        v.m_view.to   = xmin(m_view.from + to, m_view.to);
        v.add_to_list(this);
        return v;
    }

    xstring xstring::operator()(s32 from, s32 to)
    {
        xsort(from, to);
        to   = xmin(m_view.from + to, m_view.to);
        from = m_view.from + xmin(from, to);
        xstring v;
        v.m_data      = m_data;
        v.m_view.from = from;
        v.m_view.to   = to;
        v.add_to_list(this);
        return v;
    }

    xstring xstring::operator()(s32 to) const
    {
        xstring v;
        v.m_data      = m_data;
        v.m_view.from = m_view.from;
        v.m_view.to   = xmin(m_view.from + to, m_view.to);
        v.add_to_list(this);
        return v;
    }

    xstring xstring::operator()(s32 from, s32 to) const
    {
        xsort(from, to);
        to   = xmin(m_view.from + to, m_view.to);
        from = m_view.from + xmin(from, to);
        xstring v;
        v.m_data      = m_data;
        v.m_view.from = from;
        v.m_view.to   = to;
        v.add_to_list(this);
        return v;
    }

    uchar32 xstring::operator[](s32 index) const
    {
        if (index >= m_view.size())
            return '\0';
        return ustring::get_char_unsafe(*this, index);
    }

    xstring& xstring::operator=(const xstring& other)
    {
        if (this != &other)
        {
            release();
            clone(other);
        }
        return *this;
    }

    bool xstring::operator==(const xstring& other) const
    {
        if (size() != other.size())
            return false;
        for (s32 i = 0; i < size(); i++)
        {
            uchar32 const lc = ustring::get_char_unsafe(*this, i);
            uchar32 const rc = ustring::get_char_unsafe(other, i);
            if (lc != rc)
                return false;
        }
        return true;
    }

    bool xstring::operator!=(const xstring& other) const
    {
        if (size() != other.size())
            return true;
        for (s32 i = 0; i < size(); i++)
        {
            uchar32 const lc = ustring::get_char_unsafe(*this, i);
            uchar32 const rc = ustring::get_char_unsafe(other, i);
            if (lc != rc)
                return true;
        }
        return false;
    }

    void xstring::attach(xstring& str)
    {
        xstring* next = str.m_next;
        str.m_next    = this;
        this->m_next  = next;
        this->m_prev  = &str;
        next->m_prev  = this;
    }

    void xstring::release()
    {
        if (m_data == nullptr)
            return;

        // If we are the only one in the list then it means we can deallocate 'string_data'
        if (m_next == this && m_prev == this)
        {
            if (m_data->m_alloc != nullptr)
            {
                if (!ustring::is_default_string_data(m_data))
                {
                    runes_t str = ustring::get_runes(this->m_data, this->m_view);
                    m_data->m_str_alloc->deallocate(str);
                    m_data->m_alloc->deallocate(m_data);
                }
            }
        }
        rem_from_list();
        m_data = nullptr;
    }

    void xstring::clone(xstring const& str)
    {
        release();
        m_data = ustring::allocdata(str.m_data->m_alloc, str.m_data->m_str_alloc, str.m_data->m_str_len, str.m_data->m_str_type);
        m_view = str.m_view;
        add_to_list(nullptr);
    }

    //------------------------------------------------------------------------------
    xstring selectUntil(const xstring& str, uchar32 find)
    {
        for (s32 i = 0; i < str.size(); i++)
        {
            uchar32 const c = ustring::get_char_unsafe(str, i);
            if (c == find)
            {
                return str(0, i);
            }
        }
        return ustring::get_default();
    }

    xstring selectUntil(const xstring& str, const xstring& find)
    {
        xstring v = str(0, find.size());
        while (!v.is_empty())
        {
            if (v == find)
            {
                // So here we have a view with the size of the @find string on
                // string @str that matches the string @find and we need to return
                // a string view that exist before view @v.
                return ustring::select_before(str, v);
            }
            if (!ustring::move_view(str, v, 1))
                break;
        }
        return ustring::get_default();
    }

    xstring selectUntilLast(const xstring& str, uchar32 find)
    {
        for (s32 i = str.size() - 1; i >= 0; --i)
        {
            uchar32 const c = str[i];
            if (c == find)
            {
                return str(0, i);
            }
        }
        return ustring::get_default();
    }

    xstring selectUntilLast(const xstring& str, const xstring& find)
    {
        xstring v = str(str.size() - find.size(), str.size());
        while (!v.is_empty())
        {
            if (v == find)
            {
                // So here we have a view with the size of the @find string on
                // string @str that matches the string @find and we need to return
                // a string view that exist before view @v.
                return ustring::select_before(str, v);
            }
            if (!ustring::move_view(str, v, -1))
                break;
        }
        return ustring::get_default();
    }

    xstring selectUntilIncluded(const xstring& str, const xstring& find)
    {
        xstring v = str(0, find.size());
        while (!v.is_empty())
        {
            if (v == find)
            {
                // So here we have a view with the size of the @find string on
                // string @str that matches the string @find and we need to return
                // a string view that exist before and includes view @v.
                return ustring::select_before_included(str, v);
            }
            if (!ustring::move_view(str, v, 1))
                break;
        }
        return ustring::get_default();
    }

    xstring selectUntilEndExcludeSelection(const xstring& str, const xstring& selection) { return ustring::select_after_excluded(str, selection); }

    xstring selectUntilEndIncludeSelection(const xstring& str, const xstring& selection) { return ustring::select_after(str, selection); }

    bool isUpper(const xstring& str)
    {
        for (s32 i = 0; i < str.size(); i++)
        {
            uchar32 const c = str[i];
            if (is_lower(c))
                return false;
        }
        return true;
    }

    bool isLower(const xstring& str)
    {
        for (s32 i = 0; i < str.size(); i++)
        {
            uchar32 const c = ustring::get_char_unsafe(str, i);
            if (is_upper(c))
                return false;
        }
        return true;
    }

    bool isCapitalized(const xstring& str)
    {
        s32 i = 0;
        while (i < str.size())
        {
            uchar32 c = '\0';
            while (i < str.size())
            {
                c = ustring::get_char_unsafe(str, i);
                if (!is_space(c))
                    break;
                i += 1;
            }

            if (is_upper(c))
            {
                i += 1;
                while (i < str.size())
                {
                    c = ustring::get_char_unsafe(str, i);
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

    bool isQuoted(const xstring& str) { return isQuoted(str, '"'); }

    bool isQuoted(const xstring& str, uchar32 inQuote) { return isDelimited(str, inQuote, inQuote); }

    bool isDelimited(const xstring& str, uchar32 inLeft, uchar32 inRight)
    {
        if (str.is_empty())
            return false;
        s32 const first = 0;
        s32 const last  = str.size() - 1;
        return str[first] == inLeft && str[last] == inRight;
    }

    uchar32 firstChar(const xstring& str)
    {
        s32 const first = 0;
        return str[first];
    }

    uchar32 lastChar(const xstring& str)
    {
        s32 const last = str.size() - 1;
        return str[last];
    }

    bool startsWith(const xstring& str, xstring const& start)
    {
        xstring v = str(0, start.size());
        if (!v.is_empty())
            return (v == start);
        return false;
    }

    bool endsWith(const xstring& str, xstring const& end)
    {
        xstring v = str(str.size() - end.size(), str.size());
        if (!v.is_empty())
            return (v == end);
        return false;
    }

    xstring find(xstring& str, uchar32 find)
    {
        for (s32 i = 0; i < str.size(); i++)
        {
            uchar32 const c = str[i];
            if (c == find)
                return str(i, i + 1);
        }
        return ustring::get_default();
    }

    xstring find(xstring& str, const xstring& find)
    {
        xstring v = str(0, find.size());
        while (!v.is_empty())
        {
            if (v == find)
            {
                // So here we have a view with the size of the @find string on
                // string @str that matches the string @find.
                return v;
            }
            if (!ustring::move_view(str, v, 1))
                break;
        }
        return ustring::get_default();
    }

    xstring findLast(const xstring& str, const xstring& find)
    {
        xstring v = str(str.size() - find.size(), str.size());
        while (!v.is_empty())
        {
            if (v == find)
            {
                // So here we have a view with the size of the @find string on
                // string @str that matches the string @find.
                return v;
            }
            if (!ustring::move_view(str, v, -1))
                break;
        }
        return ustring::get_default();
    }

    xstring findOneOf(const xstring& str, const xstring& charset)
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
        return ustring::get_default();
    }

    xstring findOneOfLast(const xstring& str, const xstring& charset)
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
        return ustring::get_default();
    }

    s32 compare(const xstring& lhs, const xstring& rhs)
    {
        if (lhs.size() < rhs.size())
            return -1;
        if (lhs.size() > rhs.size())
            return 1;

        for (s32 i = 0; i < lhs.size(); i++)
        {
            uchar32 const lc = ustring::get_char_unsafe(lhs, i);
            uchar32 const rc = ustring::get_char_unsafe(rhs, i);
            if (lc < rc)
                return -1;
            else if (lc > rc)
                return 1;
        }
        return 0;
    }

    bool isEqual(const xstring& lhs, const xstring& rhs) { return compare(lhs, rhs) == 0; }

    bool contains(const xstring& str, const xstring& contains)
    {
        xstring v = str(0, contains.size());
        while (!v.is_empty())
        {
            if (v == contains)
            {
                // So here we have a view with the size of the @find string on
                // string @str that matches the string @find.
                return true;
            }
            if (!ustring::move_view(str, v, 1))
                break;
        }
        return false;
    }

    bool contains(const xstring& str, uchar32 contains)
    {
        for (s32 i = 0; i < str.size(); i++)
        {
            uchar32 const sc = ustring::get_char_unsafe(str, i);
            if (sc == contains)
            {
                return true;
            }
        }
        return false;
    }

    void concatenate_repeat(xstring& str, xstring const& con, s32 ntimes)
    {
        s32 const len = str.size() + (con.size() * ntimes) + 1;
        ustring::resize(str, len);
        for (s32 i = 0; i < ntimes; ++i)
        {
            concatenate(str, con);
        }
    }

    s32 xstring::format(xstring const& format, const va_list_t& args)
    {
        release();

        s32 len = vcprintf(ustring::get_crunes(format.m_data, format.m_view), args);

        data* ndata = ustring::allocdata(format.m_data->m_alloc, format.m_data->m_str_alloc, len, format.m_data->m_str_type);
        m_next      = this;
        m_prev      = this;

        runes_t str = ustring::get_runes(ndata, range(0, 0));
        vsprintf(str, ustring::get_crunes(format.m_data, format.m_view), args);
        switch (m_data->m_str_type)
        {
            case ascii::TYPE: m_view.to = str.m_runes.m_ascii.m_end - str.m_runes.m_ascii.m_bos; break;
            case utf32::TYPE: m_view.to = str.m_runes.m_utf32.m_end - str.m_runes.m_utf32.m_bos; break;
        }
        return len;
    }

    s32 xstring::formatAdd(xstring const& format, const va_list_t& args)
    {
        s32 len = vcprintf(ustring::get_crunes(format.m_data, format.m_view), args);
        ustring::resize(*this, len);
        runes_t str               = ustring::get_runes(m_data, m_view);
        str.m_runes.m_ascii.m_str = str.m_runes.m_ascii.m_end;
        vsprintf(str, ustring::get_crunes(format.m_data, format.m_view), args);
        switch (m_data->m_str_type)
        {
            case ascii::TYPE: m_view.to = str.m_runes.m_ascii.m_end - str.m_runes.m_ascii.m_bos; break;
            case utf32::TYPE: m_view.to = str.m_runes.m_utf32.m_end - str.m_runes.m_utf32.m_bos; break;
        }
        return len;
    }

    void insert(xstring& str, xstring const& pos, xstring const& insert) { ustring::insert(str, pos, insert); }

    void insert_after(xstring& str, xstring const& pos, xstring const& insert)
    {
        xstring after = ustring::select_after_excluded(str, pos);
        ustring::insert(str, after, insert);
    }

    void remove(xstring& str, xstring const& selection) { ustring::remove(str, selection); }

    void find_remove(xstring& str, const xstring& _find)
    {
        xstring sel = find(str, _find);
        if (sel.is_empty() == false)
        {
            ustring::remove(str, sel);
        }
    }

    void find_replace(xstring& str, const xstring& find, const xstring& replace) { ustring::find_replace(str, find, replace); }

    void remove_any(xstring& str, const xstring& any) { ustring::remove_any(str, any); }

    void replace_any(xstring& str, const xstring& any, uchar32 with)
    {
        // Replace any of the characters in @charset from @str with character @with
        for (s32 i = 0; i < str.size(); ++i)
        {
            uchar32 const c = ustring::get_char_unsafe(str, i);
            if (contains(any, c))
            {
                ustring::set_char_unsafe(str, i, with);
            }
        }
    }

    void upper(xstring& str)
    {
        for (s32 i = 0; i < str.size(); i++)
        {
            uchar32 c = ustring::get_char_unsafe(str, i);
            c         = to_upper(c);
            ustring::set_char_unsafe(str, i, c);
        }
    }

    void lower(xstring& str)
    {
        for (s32 i = 0; i < str.size(); i++)
        {
            uchar32 c = ustring::get_char_unsafe(str, i);
            c         = to_lower(c);
            ustring::set_char_unsafe(str, i, c);
        }
    }

    void capitalize(xstring& str)
    {
        // Standard separator is ' '
        bool prev_is_space = true;
        s32  i             = 0;
        while (i < str.size())
        {
            uchar32 c = ustring::get_char_unsafe(str, i);
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
                ustring::set_char_unsafe(str, i, c);
            }
            i++;
        }
    }

    void capitalize(xstring& str, xstring const& separators)
    {
        bool prev_is_space = false;
        s32  i             = 0;
        while (i < str.size())
        {
            uchar32 c = ustring::get_char_unsafe(str, i);
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
                ustring::set_char_unsafe(str, i, c);
            }
            i++;
        }
    }

    // Trim does nothing more than narrowing the <from, to>, nothing is actually removed
    // from the actual underlying string string_data.
    void trim(xstring& str)
    {
        trimLeft(str);
        trimRight(str);
    }

    void trimLeft(xstring& str)
    {
        for (s32 i = 0; i < str.size(); ++i)
        {
            uchar32 c = ustring::get_char_unsafe(str, i);
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
            {
                if (i > 0)
                {
                    ustring::narrow_view(str, -i);
                }
                return;
            }
        }
    }

    void trimRight(xstring& str)
    {
        s32 const last = str.size() - 1;
        for (s32 i = 0; i < str.size(); ++i)
        {
            uchar32 c = ustring::get_char_unsafe(str, last - i);
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
            {
                if (i > 0)
                {
                    ustring::narrow_view(str, i);
                }
                return;
            }
        }
    }

    void trim(xstring& str, uchar32 r)
    {
        trimLeft(str, r);
        trimRight(str, r);
    }

    void trimLeft(xstring& str, uchar32 r)
    {
        for (s32 i = 0; i < str.size(); ++i)
        {
            uchar32 c = ustring::get_char_unsafe(str, i);
            if (c != r)
            {
                if (i > 0)
                {
                    ustring::narrow_view(str, -i);
                }
                return;
            }
        }
    }

    void trimRight(xstring& str, uchar32 r)
    {
        s32 const last = str.size() - 1;
        for (s32 i = 0; i < str.size(); ++i)
        {
            uchar32 c = ustring::get_char_unsafe(str, last - i);
            if (c != r)
            {
                if (i > 0)
                {
                    ustring::narrow_view(str, i);
                }
                return;
            }
        }
    }

    void trim(xstring& str, xstring const& set)
    {
        trimLeft(str, set);
        trimRight(str, set);
    }

    void trimLeft(xstring& str, xstring const& set)
    {
        for (s32 i = 0; i < str.size(); ++i)
        {
            uchar32 c = ustring::get_char_unsafe(str, i);
            if (!contains(set, c))
            {
                if (i > 0)
                {
                    ustring::narrow_view(str, -i);
                }
                return;
            }
        }
    }

    void trimRight(xstring& str, xstring const& set)
    {
        s32 const last = str.size() - 1;
        for (s32 i = 0; i < str.size(); ++i)
        {
            uchar32 c = ustring::get_char_unsafe(str, last - i);
            if (!contains(set, c))
            {
                if (i > 0)
                {
                    ustring::narrow_view(str, i);
                }
                return;
            }
        }
    }

    void trimQuotes(xstring& str) { trimDelimiters(str, '"', '"'); }

    void trimQuotes(xstring& str, uchar32 quote) { trimDelimiters(str, quote, quote); }

    void trimDelimiters(xstring& str, uchar32 left, uchar32 right)
    {
        trimLeft(str, left);
        trimRight(str, right);
    }

    void reverse(xstring& str)
    {
        s32 const last = str.size() - 1;
        for (s32 i = 0; i < (last - i); ++i)
        {
            uchar32 l = ustring::get_char_unsafe(str, i);
            uchar32 r = ustring::get_char_unsafe(str, last - i);
            ustring::set_char_unsafe(str, i, r);
            ustring::set_char_unsafe(str, last - i, l);
        }
    }

    bool splitOn(xstring& str, uchar32 inChar, xstring& outLeft, xstring& outRight)
    {
        outLeft = selectUntil(str, inChar);
        if (outLeft.is_empty())
            return false;
        outRight = str(outLeft.size(), str.size());
        trimRight(outLeft, inChar);
        return true;
    }

    bool splitOn(xstring& str, xstring& inStr, xstring& outLeft, xstring& outRight)
    {
        outLeft = selectUntil(str, inStr);
        if (outLeft.is_empty())
            return false;
        outRight = str(outLeft.size() + inStr.size(), str.size());
        return true;
    }

    bool splitOnLast(xstring& str, uchar32 inChar, xstring& outLeft, xstring& outRight)
    {
        outLeft = selectUntilLast(str, inChar);
        if (outLeft.is_empty())
            return false;
        outRight = str(outLeft.size(), str.size());
        trimRight(outLeft, inChar);
        return true;
    }

    bool splitOnLast(xstring& str, xstring& inStr, xstring& outLeft, xstring& outRight)
    {
        outLeft = selectUntilLast(str, inStr);
        if (outLeft.is_empty())
            return false;
        outRight = str(outLeft.size() + inStr.size(), str.size());
        return true;
    }

    void concatenate(xstring& str, const xstring& con) { ustring::resize(str, str.size() + con.size() + 1); }

    //------------------------------------------------------------------------------
    static void user_case_for_string()
    {
        xstring str("This is an ascii string that will be converted to UTF-32");

        xstring strvw  = str;
        xstring substr = find(strvw, xstring("ascii"));
        upper(substr);

        find_remove(str, xstring("converted "));
    }

} // namespace xcore
