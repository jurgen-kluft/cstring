#include "xstr/x_cstr.h"

#include "xbase/x_integer.h"
#include "xbase/x_memory.h"
#include "xbase/x_runes.h"
#include "xbase/x_printf.h"

namespace xcore
{
    enum
    {
        NONE,
        ASCII,
        CASCII,
        UTF32,
        CUTF32
    };

    //==============================================================================
    class xview
    {
    public:
        static inline uchar32 get_char_unsafe(xstr::view const& str, s32 i)
        {
            if (str.is_valid())
            {
                switch (str.m_data->m_type)
                {
                    case CASCII: return str.m_data->m_runes._cascii.m_str[i];
                    case ASCII: return str.m_data->m_runes._ascii.m_str[i];
                    case CUTF32: return str.m_data->m_runes._cutf32.m_str[i];
                    case UTF32: return str.m_data->m_runes._utf32.m_str[i];
                }
            }
            return '\0';
        }

        static inline uchar32 get_char_unsafe(xstr const& str, s32 i)
        {
            if (str.is_valid())
            {
                switch (str.m_data.m_type)
                {
                    case CASCII: return str.m_data.m_runes._cascii.m_str[i];
                    case ASCII: return str.m_data.m_runes._ascii.m_str[i];
                    case CUTF32: return str.m_data.m_runes._cutf32.m_str[i];
                    case UTF32: return str.m_data.m_runes._utf32.m_str[i];
                }
            }
            return '\0';
        }

        static inline void set_char_unsafe(xstr& str, s32 i, uchar32 c)
        {
            if (str.is_valid())
            {
                switch (str.m_data.m_type)
                {
                    case ASCII: str.m_data.m_runes._ascii.m_str[i] = c; break;
                    case UTF32: str.m_data.m_runes._utf32.m_str[i] = c; break;
                }
            }
        }

        static inline void set_char_unsafe(xstr::view& str, s32 i, uchar32 c)
        {
            if (str.is_valid())
            {
                switch (str.m_data->m_type)
                {
                    case ASCII: str.m_data->m_runes._ascii.m_str[i] = c; break;
                    case UTF32: str.m_data->m_runes._utf32.m_str[i] = c; break;
                }
            }
        }

        static inline void set_char_end(xstr& str, s32 i)
        {
            if (str.is_valid())
            {
                switch (str.m_data.m_type)
                {
                    case ASCII:
                        i                                  = xmin(i, str.m_data.m_runes._ascii.cap());
                        str.m_data.m_runes._ascii.m_end    = &str.m_data.m_runes._ascii.m_str[i];
                        str.m_data.m_runes._ascii.m_end[i] = '\0';
                        break;
                    case UTF32:
                        i                                  = xmin(i, str.m_data.m_runes._utf32.cap());
                        str.m_data.m_runes._utf32.m_end    = &str.m_data.m_runes._utf32.m_str[i];
                        str.m_data.m_runes._utf32.m_end[i] = '\0';
                        break;
                }
            }
        }

        static inline void set_char_end(xstr::view& str, s32 i)
        {
            if (str.is_valid())
            {
                switch (str.m_data->m_type)
                {
                    case ASCII:
                        i                                   = xmin(i, str.m_data->m_runes._ascii.cap());
                        str.m_data->m_runes._ascii.m_end    = &str.m_data->m_runes._ascii.m_str[i];
                        str.m_data->m_runes._ascii.m_end[i] = '\0';
                        break;
                    case UTF32:
                        i                                   = xmin(i, str.m_data->m_runes._utf32.cap());
                        str.m_data->m_runes._utf32.m_end    = &str.m_data->m_runes._utf32.m_str[i];
                        str.m_data->m_runes._utf32.m_end[i] = '\0';
                        break;
                }
            }
        }

        static inline bool str_has_view(xstr const& str, xstr::view const& vw) { return (vw.m_data == &str.m_data); }

        static bool resize(xstr& str, s32 new_size)
        {
            if (str.cap() < new_size)
            {
                return false;
            }
            return true;
        }

        static bool narrow_view(xstr::view& v, s32 move)
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

        static bool move_view(xstr::view& v, s32 move)
        {
            // Check if the move doesn't result in a negative @from
            s32 const from = v.m_view.from + move;
            if (from < 0)
                return false;

            // Check if the movement doesn't invalidate this view
            s32 const to = v.m_view.to + move;
            if (to > v.m_data->m_runes.size())
                return false;

            // Movement is ok, new view is valid
            v.m_view.from = from;
            v.m_view.to   = to;
            return true;
        }

        static xstr::view select_before(const xstr::view& str, const xstr::view& selection)
        {
            xstr::view v(selection);
            v.m_view.to   = v.m_view.from;
            v.m_view.from = 0;
            return v;
        }
        static xstr::view select_before_included(const xstr::view& str, const xstr::view& selection)
        {
            xstr::view v(selection);
            v.m_view.to   = v.m_view.to;
            v.m_view.from = 0;
            return v;
        }

        static xstr::view select_after(const xstr::view& str, const xstr::view& sel)
        {
            xstr::view v(sel);
            v.m_view.to   = str.m_view.to;
            v.m_view.from = sel.m_view.from;
            return v;
        }

        static xstr::view select_after_excluded(const xstr::view& str, const xstr::view& sel)
        {
            xstr::view v(sel);
            v.m_view.to   = str.m_view.to;
            v.m_view.from = sel.m_view.to;
            return v;
        }

        static void insert_newspace(xstr& r, s32 pos, s32 len)
        {
            s32 src = r.size() - 1;
            s32 dst = src + len;
            while (src >= pos)
            {
                uchar32 c = get_char_unsafe(r, src--);
                set_char_unsafe(r, dst--);
            }

            switch (str.m_data->m_type)
            {
                case ASCII:
                    r.m_data.m_runes._ascii.m_end += len;
                    r.m_data.m_runes._ascii.m_end[0] = '\0';
                    break case UTF32 : r.m_data.m_runes._utf32.m_end += len;
                    r.m_data.m_runes._utf32.m_end[0] = '\0';
                    break
            }
        }

        static void remove_selspace(xstr& r, s32 pos, s32 len)
        {
            s32       src = pos + len;
            s32       dst = pos;
            s32 const end = pos + len;
            while (src < r.size())
            {
                uchar32 c = get_char_unsafe(r, src++);
                set_char_unsafe(r, dst++);
            }
            switch (str.m_data->m_type)
            {
                case ASCII:
                    r.m_data.m_runes._ascii.m_end -= len;
                    r.m_data.m_runes._ascii.m_end[0] = '\0';
                    break case UTF32 : r.m_data.m_runes._utf32.m_end -= len;
                    r.m_data.m_runes._utf32.m_end[0] = '\0';
                    break
            }
        }

        static void insert(xstr& str, xstr::view const& pos, xstr::view const& insert)
        {
            s32 dst = pos.m_view.from;

            xstr::range insert_range(dst, dst + insert.size());
            xview::adjust_active_views(str, xview::INSERTION, insert_range);

            xview::resize(str, str.size() + insert.size() + 1);
            // TODO: See how much we can actually insert, since we cannot grow this string
            insert_newspace(xview::get_runes(str), dst, insert.size());
            s32 src = 0;
            while (src < insert.size())
            {
                uchar32 const c = xview::read_char_unsafe(insert, src);
                xview::write_char_unsafe(str, dst, c);
                ++src;
                ++dst;
            }
        }

        static void remove(xstr& str, xstr::view const& selection)
        {
            if (selection.is_empty())
                return;
            if (xview::str_has_view(str, selection))
            {
                remove_selspace(str.m_data.m_runes, selection.m_view.from, selection.size());
                // TODO: Decision to shrink the allocated memory of m_runes ?

                xstr::range remove_range(selection.m_view.from, selection.m_view.to);
                xview::adjust_active_views(str, xview::REMOVAL, remove_range);
            }
        }

        static void find_remove(xstr& str, const xstr::view& _find)
        {
            xstr::view strvw = str.full();
            xstr::view sel   = find(strvw, _find);
            if (sel.is_empty() == false)
            {
                remove(str, sel);
            }
        }

        static void find_replace(xstr& str, const xstr::view& _find, const xstr::view& replace)
        {
            xstr::view strvw  = str.full();
            xstr::view remove = find(strvw, _find);
            if (remove.is_empty() == false)
            {
                s32 const remove_from = remove.m_view.from;
                s32 const remove_len  = remove.size();
                s32 const diff        = remove_len - replace.size();
                if (diff > 0)
                {
                    // The string to replace the selection with is smaller, so we have to remove
                    // some space from the string.
                    remove_selspace(str, remove_from, diff);
                    // TODO: Decision to shrink the allocated memory of m_runes ?

                    xstr::range remove_range(remove_from, remove_from + diff);
                    xview::adjust_active_views(str, xview::REMOVAL, remove_range);
                }
                else if (diff < 0)
                {
                    // The string to replace the selection with is longer, so we have to insert some
                    // space into the string.
                    xview::resize(str, str.size() + (-diff));
                    // TODO: See how much we can actually insert, since we cannot grow this string
                    insert_newspace(str, remove_from, -diff);

                    xstr::range insert_range(remove_from, remove_from + -diff);
                    xview::adjust_active_views(str, xview::INSERTION, insert_range);
                }
                // Copy string 'remove' into the (now) same size selection space
                s32       src = 0;
                s32       dst = remove_from;
                s32 const end = remove_from + replace.size();
                while (src < replace.size())
                {
                    xview::set_char_unsafe(str, dst, replace[src++]);
                }
            }
        }

        static void remove_any(xstr& str, const xstr::view& any)
        {
            // Remove any of the characters in @charset from @str
            s32       d   = 0;
            s32       i   = 0;
            s32 const end = str.size();
            s32       r   = -1;
            while (i < end)
            {
                uchar32 const c = xview::get_char_unsafe(str, i);
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
                        s32 const   gap = i - d;
                        xstr::range removal_range(r, i);
                        xview::shift_range(removal_range, gap);
                        xview::adjust_active_views(str, xview::REMOVAL, removal_range);
                        r = -1;
                    }

                    if (i > d)
                    {
                        xview::set_char_unsafe(str, d, c);
                    }
                    i++;
                    d++;
                }
            }
            s32 const l = i - d;
            if (l > 0)
            {
                switch (str.m_data->m_type)
                {
                    case ASCII:
                        r.m_data.m_runes._ascii.m_end -= l;
                        r.m_data.m_runes._ascii.m_end[0] = '\0';
                        break case UTF32 : r.m_data.m_runes._utf32.m_end -= l;
                        r.m_data.m_runes._utf32.m_end[0] = '\0';
                        break
                }
            }
        }

        static xstr::view get_default() { return xstr::view(nullptr); }

        enum
        {
            NONE     = 0,
            LEFT     = BU16(0000, 1000, 0000, 0000),
            RIGHT    = BU16(0000, 0000, 0001, 0000),
            INSIDE   = BU16(0000, 0001, 1000, 0000),
            MATCH    = BU16(0000, 0011, 1100, 0000),
            OVERLAP  = BU16(0000, 0111, 1110, 0000),
            ENVELOPE = BU16(0011, 0111, 1110, 1100),
        };

        static s32 compare(xstr::range const& lhs, xstr::range const& rhs)
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
        static inline s32 compute_range_overlap(xstr::range const& lhs, xstr::range const& rhs)
        {
            if (rhs.from < lhs.from && rhs.to < lhs.to)
                return rhs.to - lhs.from;
            else if (rhs.from > lhs.from && rhs.to > lhs.to)
                return lhs.to - rhs.from;
            return 0;
        }
        static inline void shift_range(xstr::range& v, s32 distance)
        {
            v.from += distance;
            v.to += distance;
        }
        static inline void extend_range(xstr::range& v, s32 distance)
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
        static inline void narrow_range(xstr::range& v, s32 distance)
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
        static inline void invalidate_range(xstr::range& v)
        {
            v.from = 0;
            v.to   = 0;
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
        static void      adjust_active_views(xstr::view* v, s32 op_code, xstr::range op_range)
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
                    // --------[ rhs    |    ]  lhs |--------        LEFT OVERLAP = NARROW LEFT by overlap & SHIFT LEFT by
                    // rhs.size()-overlap
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
                        case RIGHT | ENVELOPE: invalidate_range(v->m_view); break;
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
                    invalidate_range(v->m_view);
                    v->m_data = nullptr;
                }
                break;
            }
        }

        static void adjust_active_views(xstr& str, s32 op_code, xstr::range op_range)
        {
            xstr::view* list = str.m_data.m_views;
            if (list != nullptr)
            {
                xstr::view* iter = list;
                do
                {
                    adjust_active_views(iter, op_code, op_range);
                    iter = iter->m_next;
                } while (iter != list);
            }
        }
    };

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    xstr::view::view(xstr::data* d) : m_data(d), m_next(nullptr), m_prev(nullptr) {}

    xstr::view::view(const view& other) : m_data(other.m_data), m_view(other.m_view), m_next(nullptr), m_prev(nullptr)
    {
        add();
    }

    xstr::view::~view() { rem(); }

    s32 xstr::view::size() const { return m_view.size(); }

    bool xstr::view::is_valid() const { return m_data != nullptr; }

    bool xstr::view::is_empty() const { return m_view.size() == 0; }

    xstr::view xstr::view::operator()(s32 to)
    {
        to = xmin(0, m_view.size());
        xstr::view v(m_data);
        v.m_view.from = m_view.from;
        v.m_view.to   = m_view.from + to;
        v.add();
        return v;
    }

    xstr::view xstr::view::operator()(s32 from, s32 to)
    {
        xsort(from, to);
        to   = xmin(to, m_view.size());
        from = xmin(from, to);
        xstr::view v(m_data);
        v.m_view.from = m_view.from + from;
        v.m_view.to   = m_view.from + to;
        v.add();
        return v;
    }

    xstr::view xstr::view::operator()(s32 to) const
    {
        to = xmin(0, m_view.size());
        xstr::view v(m_data);
        v.m_view.from = m_view.from;
        v.m_view.to   = m_view.from + to;
        v.add();
        return v;
    }

    xstr::view xstr::view::operator()(s32 from, s32 to) const
    {
        xsort(from, to);
        to   = xmin(to, m_view.size());
        from = xmin(from, to);
        xstr::view v(m_data);
        v.m_view.from = m_view.from + from;
        v.m_view.to   = m_view.from + to;
        v.add();
        return v;
    }

    uchar32 xstr::view::operator[](s32 index) const
    {
        if (index < 0 || index >= m_view.size())
            return '\0';
        return xview::get_char_unsafe(*this, m_view.from + index);
    }

    xstr::view& xstr::view::operator=(xstr::view const& other)
    {
        rem();
        m_data = other.m_data;
        m_view = other.m_view;
        add();
        return *this;
    }

    bool xstr::view::operator==(xstr::view const& other) const
    {
        if (m_view.size() == other.m_view.size())
        {
            for (s32 i = 0; i < m_view.size(); ++i)
            {
                uchar32 const c1 = xview::get_char_unsafe(*this, i);
                uchar32 const c2 = xview::get_char_unsafe(other, i);
                if (c1 != c2)
                    return false;
            }
            return true;
        }
        return false;
    }

    bool xstr::view::operator!=(xstr::view const& other) const
    {
        if (m_view.size() == other.m_view.size())
        {
            for (s32 i = 0; i < m_view.size(); ++i)
            {
                uchar32 const c1 = xview::get_char_unsafe(*this, i);
                uchar32 const c2 = xview::get_char_unsafe(other, i);
                if (c1 == c2)
                    return false;
            }
            return false;
        }
        return true;
    }

    void xstr::view::add()
    {
        if (m_data != nullptr)
        {
            xstr::view*& list = m_data->m_views;
            if (list == nullptr)
            {
                list   = this;
                m_next = this;
                m_prev = this;
            }
            else
            {
                xstr::view* prev = list->m_prev;
                xstr::view* next = list;
                prev->m_next     = this;
                next->m_prev     = this;
                m_next           = next;
                m_prev           = prev;
                list             = this;
            }
        }
    }

    void xstr::view::rem()
    {
        if (m_data != nullptr && m_data->m_views != nullptr)
        {
            xstr::view*& list = m_data->m_views;

            xstr::view* prev = m_prev;
            xstr::view* next = m_next;
            prev->m_next     = next;
            next->m_prev     = prev;

            if (list == this)
            {
                list = next;
                if (list == this)
                {
                    list = nullptr;
                }
            }
        }
        m_data = nullptr;
        m_next = nullptr;
        m_prev = nullptr;
    }

    void xstr::view::invalidate()
    {
        m_data      = nullptr;
        m_view.from = 0;
        m_view.to   = 0;
    }

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------

    xstr::xstr(void) : m_data(nullptr) {}

    xstr::xstr(char* str, char* end, char* eos) : m_data()
    {
        m_data.m_type         = ASCII;
        m_data.m_runes._ascii = ascii::runes(str, end, eos);
    }

    xstr::xstr(const char* str, const char* end) : m_data()
    {
        m_data.m_type          = CASCII;
        m_data.m_runes._cascii = ascii::crunes(str, end);
    }

    xstr::xstr(utf32::rune* str, utf32::rune* end, utf32::rune* eos) : m_data()
    {
        m_data.m_type         = UTF32;
        m_data.m_runes._utf32 = utf32::runes(str, end, eos);
    }

    xstr::xstr(const utf32::rune* str, const utf32::rune* end) : m_data()
    {
        m_data.m_type          = CUTF32;
        m_data.m_runes._cutf32 = utf32::crunes(str, end);
    }

    xstr::xstr(const xstr& other) : m_data(other.m_data) {}

    xstr::xstr(xstr const& str, const xstr::view& left, const xstr::view& right) : m_data(str.m_data)
    {
        s32 const cap = left.size() + right.size() + 1;
        xview::resize(*this, cap);

        s32 d = 0;
        for (s32 i = 0; i < left.size(); i++, d++)
        {
            uchar32 const lc = xview::get_char_unsafe(left, i);
            if (xview::set_char_unsafe(*this, d))
                break;
        }
        for (s32 i = 0; i < right.size(); i++, d++)
        {
            uchar32 const lc = xview::get_char_unsafe(right, i);
            if (xview::set_char_unsafe(*this, d))
                break;
        }
        xview::set_char_end(*this, d)
    }

    xstr::~xstr() { xview::adjust_active_views(*this, xview::CLEARED, xstr::range(0, 0)); }

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------

    bool xstr::is_empty() const
    {
        switch (m_data.m_type)
        {
            case ASCII: return m_data.m_runes._ascii.size() == 0;
            case CASCII: return m_data.m_runes._cascii.size() == 0;
            case UTF32: return m_data.m_runes._utf32.size() == 0;
            case CUTF32: return m_data.m_runes._cutf32.size() == 0;
        }
        return true;
    }

    s32 xstr::size() const
    {
        switch (m_data.m_type)
        {
            case ASCII: return m_data.m_runes._ascii.size();
            case CASCII: return m_data.m_runes._cascii.size();
            case UTF32: return m_data.m_runes._utf32.size();
            case CUTF32: return m_data.m_runes._cutf32.size();
        }
        return 0;
    }

    s32 xstr::cap() const
    {
        switch (m_data.m_type)
        {
            case ASCII: return m_data.m_runes._ascii.cap();
            case UTF32: return m_data.m_runes._utf32.cap();
        }
        return 0;
    }

    void xstr::clear()
    {
        m_data.m_runes.clear();
        xview::adjust_active_views(*this, xview::CLEARED, xstr::range(0, 0));
    }

    xstr::view xstr::full()
    {
        xstr::view v(&m_data);
        v.m_view.from = 0;
        v.m_view.to   = m_data.m_runes.size();
        v.add();
        return v;
    }

    xstr::view xstr::full() const
    {
        xstr::view v(&m_data);
        v.m_view.from = 0;
        v.m_view.to   = m_data.m_runes.size();
        v.add();
        return v;
    }

    xstr::view xstr::operator()(s32 to)
    {
        xstr::view v(&m_data);
        v.m_view.from = 0;
        v.m_view.to   = xmin(size(), xmax((s32)0, to));
        v.add();
        return v;
    }

    xstr::view xstr::operator()(s32 from, s32 to)
    {
        xsort(from, to);
        to   = xmin(to, size());
        from = xmin(from, to);
        xstr::view v(&m_data);
        v.m_view.from = from;
        v.m_view.to   = to;
        v.add();
        return v;
    }

    xstr::view xstr::operator()(s32 to) const
    {
        xstr::view v(&m_data);
        v.m_view.from = 0;
        v.m_view.to   = xmin(size(), xmax((s32)0, to));
        v.add();
        return v;
    }

    xstr::view xstr::operator()(s32 from, s32 to) const
    {
        xsort(from, to);
        to   = xmin(to, size());
        from = xmin(from, to);
        xstr::view v(&m_data);
        v.m_view.from = from;
        v.m_view.to   = to;
        v.add();
        return v;
    }

    uchar32 xstr::operator[](s32 index) const
    {
        if (index >= size())
            return '\0';
        return xview::get_char_unsafe(*this, index);
    }

    xstr& xstr::operator=(const xstr& other)
    {
        if (this != &other)
        {
            release();
        }
        return *this;
    }

    xstr& xstr::operator=(const xstr::view& other)
    {
        if (&m_data != other.m_data)
        {
            release();
        }
        return *this;
    }

    bool xstr::operator==(const xstr& other) const
    {
        if (size() != other.size())
            return false;
        for (s32 i = 0; i < size(); i++)
        {
            uchar32 const lc = xview::get_char_unsafe(*this, i);
            uchar32 const rc = xview::get_char_unsafe(other, i);
            if (lc != rc)
                return false;
        }
        return true;
    }

    bool xstr::operator!=(const xstr& other) const
    {
        if (size() != other.size())
            return true;
        for (s32 i = 0; i < size(); i++)
        {
            uchar32 const lc = xview::get_char_unsafe(*this, i);
            uchar32 const rc = xview::get_char_unsafe(other, i);
            if (lc != rc)
                return true;
        }
        return false;
    }

    void xstr::release()
    {
        if (m_data.m_alloc != nullptr)
        {
            xview::adjust_active_views(*this, xview::RELEASED, xstr::range(0, 0));
            m_data.m_alloc->deallocate(m_data.m_runes);
        }
    }

    void xstr::clone(utf32::runes const& str, utf32::alloc* allocator)
    {
        m_data.m_alloc = allocator;
        m_data.m_runes = m_data.m_alloc->allocate(0, str.size() + 1);
        utf32::copy(str, m_data.m_runes);
    }

    //------------------------------------------------------------------------------
    xstr::view selectUntil(const xstr::view& str, uchar32 find)
    {
        for (s32 i = 0; i < str.size(); i++)
        {
            uchar32 const c = xview::get_char_unsafe(str, i);
            if (c == find)
            {
                return str(0, i);
            }
        }
        return xview::get_default();
    }

    xstr::view selectUntil(const xstr::view& str, const xstr::view& find)
    {
        xstr::view v = str(0, find.size());
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
        return xview::get_default();
    }

    xstr::view selectUntilLast(const xstr::view& str, uchar32 find)
    {
        for (s32 i = str.size() - 1; i >= 0; --i)
        {
            uchar32 const c = str[i];
            if (c == find)
            {
                return str(0, i);
            }
        }
        return xview::get_default();
    }

    xstr::view selectUntilLast(const xstr::view& str, const xstr::view& find)
    {
        xstr::view v = str(str.size() - find.size(), str.size());
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
        return xview::get_default();
    }

    xstr::view selectUntilIncluded(const xstr::view& str, const xstr::view& find)
    {
        xstr::view v = str(0, find.size());
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
        return xview::get_default();
    }

    xstr::view selectUntilEndExcludeSelection(const xstr::view& str, const xstr::view& selection)
    {
        return xview::select_after_excluded(str, selection);
    }

    xstr::view selectUntilEndIncludeSelection(const xstr::view& str, const xstr::view& selection)
    {
        return xview::select_after(str, selection);
    }

    bool isUpper(const xstr::view& str)
    {
        for (s32 i = 0; i < str.size(); i++)
        {
            uchar32 const c = str[i];
            if (utf32::is_lower(c))
                return false;
        }
        return true;
    }

    bool isLower(const xstr::view& str)
    {
        for (s32 i = 0; i < str.size(); i++)
        {
            uchar32 const c = xview::get_char_unsafe(str, i);
            if (utf32::is_upper(c))
                return false;
        }
        return true;
    }

    bool isCapitalized(const xstr::view& str)
    {
        s32 i = 0;
        while (i < str.size())
        {
            uchar32 c = '\0';
            while (i < str.size())
            {
                c = xview::get_char_unsafe(str, i);
                if (!utf32::is_space(c))
                    break;
                i += 1;
            }

            if (utf32::is_upper(c))
            {
                i += 1;
                while (i < str.size())
                {
                    c = xview::get_char_unsafe(str, i);
                    if (utf32::is_space(c))
                        break;
                    if (utf32::is_upper(c))
                        return false;
                    i += 1;
                }
            }
            else if (utf32::is_alpha(c))
            {
                return false;
            }
            i += 1;
        }
        return true;
    }

    bool isQuoted(const xstr::view& str) { return isQuoted(str, '"'); }

    bool isQuoted(const xstr::view& str, uchar32 inQuote) { return isDelimited(str, inQuote, inQuote); }

    bool isDelimited(const xstr::view& str, uchar32 inLeft, uchar32 inRight)
    {
        if (str.is_empty())
            return false;
        s32 const first = 0;
        s32 const last  = str.size() - 1;
        return str[first] == inLeft && str[last] == inRight;
    }

    uchar32 firstChar(const xstr::view& str)
    {
        s32 const first = 0;
        return str[first];
    }

    uchar32 lastChar(const xstr::view& str)
    {
        s32 const last = str.size() - 1;
        return str[last];
    }

    bool startsWith(const xstr::view& str, xstr::view const& start)
    {
        xstr::view v = str(0, start.size());
        if (!v.is_empty())
            return (v == start);
        return false;
    }

    bool endsWith(const xstr::view& str, xstr::view const& end)
    {
        xstr::view v = str(str.size() - end.size(), str.size());
        if (!v.is_empty())
            return (v == end);
        return false;
    }

    xstr::view find(xstr& str, uchar32 find)
    {
        for (s32 i = 0; i < str.size(); i++)
        {
            uchar32 const c = str[i];
            if (c == find)
                return str(i, i + 1);
        }
        return xview::get_default();
    }

    xstr::view find(xstr::view& str, uchar32 find)
    {
        for (s32 i = 0; i < str.size(); i++)
        {
            uchar32 const c = str[i];
            if (c == find)
                return str(i, i + 1);
        }
        return xview::get_default();
    }

    xstr::view find(xstr& str, const xstr::view& find)
    {
        xstr::view v = str(0, find.size());
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
        return xview::get_default();
    }

    xstr::view find(xstr::view& str, const xstr::view& find)
    {
        xstr::view v = str(0, find.size());
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
        return xview::get_default();
    }

    xstr::view findLast(const xstr::view& str, const xstr::view& find)
    {
        xstr::view v = str(str.size() - find.size(), str.size());
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
        return xview::get_default();
    }

    xstr::view findOneOf(const xstr::view& str, const xstr::view& charset)
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
        return xview::get_default();
    }

    xstr::view findOneOfLast(const xstr::view& str, const xstr::view& charset)
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
        return xview::get_default();
    }

    s32 compare(const xstr::view& lhs, const xstr::view& rhs)
    {
        if (lhs.size() < rhs.size())
            return -1;
        if (lhs.size() > rhs.size())
            return 1;

        for (s32 i = 0; i < lhs.size(); i++)
        {
            uchar32 const lc = xview::get_char_unsafe(lhs, i);
            uchar32 const rc = xview::get_char_unsafe(rhs, i);
            if (lc < rc)
                return -1;
            else if (lc > rc)
                return 1;
        }
        return 0;
    }

    bool isEqual(const xstr::view& lhs, const xstr::view& rhs) { return compare(lhs, rhs) == 0; }

    bool contains(const xstr::view& str, const xstr::view& contains)
    {
        xstr::view v = str(0, contains.size());
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

    bool contains(const xstr::view& str, uchar32 contains)
    {
        for (s32 i = 0; i < str.size(); i++)
        {
            uchar32 const sc = xview::get_char_unsafe(str, i);
            if (sc == contains)
            {
                return true;
            }
        }
        return false;
    }

    void concatenate_repeat(xstr& str, xstr::view const& con, s32 ntimes)
    {
        s32 const len = str.size() + (con.size() * ntimes) + 1;
        xview::resize(str, len);
        for (s32 i = 0; i < ntimes; ++i)
        {
            concatenate(str, con);
        }
    }

    s32 format(xstr& str, xstr::view const& format, const x_va_list& args)
    {
        str.clear();
        s32 len = utf32::vcprintf(xview::get_runes(format), args);
        xview::resize(str, len);
        utf32::vsprintf(xview::get_runes(str), xview::get_runes(format), args);
        return 0;
    }

    s32 formatAdd(xstr& str, xstr::view const& format, const x_va_list& args)
    {
        s32 len = utf32::vcprintf(xview::get_runes(format), args);
        xview::resize(str, len);
        utf32::vsprintf(xview::get_runes(str), xview::get_runes(format), args);
        return 0;
    }

    void insert(xstr& str, xstr::view const& pos, xstr::view const& insert) { xview::insert(str, pos, insert); }

    void insert_after(xstr& str, xstr::view const& pos, xstr::view const& insert)
    {
        xstr::view after = xview::select_after_excluded(str.full(), pos);
        xview::insert(str, after, insert);
    }

    void remove(xstr& str, xstr::view const& selection) { xview::remove(str, selection); }

    void find_remove(xstr& str, const xstr::view& _find)
    {
        xstr::view strvw = str.full();
        xstr::view sel   = find(strvw, _find);
        if (sel.is_empty() == false)
        {
            xview::remove(str, sel);
        }
    }

    void find_replace(xstr& str, const xstr::view& find, const xstr::view& replace) { xview::find_replace(str, find, replace); }

    void remove_any(xstr& str, const xstr::view& any) { xview::remove_any(str, any); }

    void replace_any(xstr::view& str, const xstr::view& any, uchar32 with)
    {
        // Replace any of the characters in @charset from @str with character @with
        for (s32 i = 0; i < str.size(); ++i)
        {
            uchar32 const c = xview::get_char_unsafe(str, i);
            if (contains(any, c))
            {
                xview::set_char_unsafe(str, i, with);
            }
        }
    }

    void upper(xstr::view& str)
    {
        for (s32 i = 0; i < str.size(); i++)
        {
            uchar32* cp = xview::get_ptr_unsafe(str, i);
            *cp         = utf32::to_upper(*cp);
        }
    }

    void lower(xstr::view& str)
    {
        for (s32 i = 0; i < str.size(); i++)
        {
            uchar32* cp = xview::get_ptr_unsafe(str, i);
            *cp         = utf32::to_lower(*cp);
        }
    }

    void capitalize(xstr::view& str)
    {
        // Standard separator is ' '
        bool prev_is_space = true;
        s32  i             = 0;
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

    void capitalize(xstr::view& str, xstr::view const& separators)
    {
        bool prev_is_space = false;
        s32  i             = 0;
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
    void trim(xstr::view& str)
    {
        trimLeft(str);
        trimRight(str);
    }

    void trimLeft(xstr::view& str)
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

    void trimRight(xstr::view& str)
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

    void trim(xstr::view& str, uchar32 r)
    {
        trimLeft(str, r);
        trimRight(str, r);
    }

    void trimLeft(xstr::view& str, uchar32 r)
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

    void trimRight(xstr::view& str, uchar32 r)
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

    void trim(xstr::view& str, xstr::view const& set)
    {
        trimLeft(str, set);
        trimRight(str, set);
    }

    void trimLeft(xstr::view& str, xstr::view const& set)
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

    void trimRight(xstr::view& str, xstr::view const& set)
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

    void trimQuotes(xstr::view& str) { trimDelimiters(str, '"', '"'); }

    void trimQuotes(xstr::view& str, uchar32 quote) { trimDelimiters(str, quote, quote); }

    void trimDelimiters(xstr::view& str, uchar32 left, uchar32 right)
    {
        trimLeft(str, left);
        trimRight(str, right);
    }

    void reverse(xstr::view& str)
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

    bool splitOn(xstr::view& str, uchar32 inChar, xstr::view& outLeft, xstr::view& outRight)
    {
        outLeft = selectUntil(str, inChar);
        if (outLeft.is_empty())
            return false;
        outRight = str(outLeft.size(), str.size());
        trimRight(outLeft, inChar);
        return true;
    }

    bool splitOn(xstr::view& str, xstr::view& inStr, xstr::view& outLeft, xstr::view& outRight)
    {
        outLeft = selectUntil(str, inStr);
        if (outLeft.is_empty())
            return false;
        outRight = str(outLeft.size() + inStr.size(), str.size());
        return true;
    }

    bool splitOnLast(xstr::view& str, uchar32 inChar, xstr::view& outLeft, xstr::view& outRight)
    {
        outLeft = selectUntilLast(str, inChar);
        if (outLeft.is_empty())
            return false;
        outRight = str(outLeft.size(), str.size());
        trimRight(outLeft, inChar);
        return true;
    }

    bool splitOnLast(xstr::view& str, xstr::view& inStr, xstr::view& outLeft, xstr::view& outRight)
    {
        outLeft = selectUntilLast(str, inStr);
        if (outLeft.is_empty())
            return false;
        outRight = str(outLeft.size() + inStr.size(), str.size());
        return true;
    }

    void concatenate(xstr& str, const xstr::view& con) { xview::resize(str, str.size() + con.size() + 1); }

    //------------------------------------------------------------------------------
    static void user_case_for_string()
    {
        xstr str("This is an ascii string that will be converted to UTF-32");

        xstr::view strvw  = str.full();
        xstr::view substr = find(strvw, xstr("ascii"));
        upper(substr);

        find_remove(str, xstr("converted "));
    }

} // namespace xcore
