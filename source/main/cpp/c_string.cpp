#include "cbase/c_allocator.h"
#include "cbase/c_context.h"
#include "cbase/c_integer.h"
#include "cbase/c_memory.h"
#include "cbase/c_printf.h"
#include "cbase/c_runes.h"
#include "cstring/c_string.h"

namespace ncore
{
    // Strings are stored in memory in UTF-16 format.
    // All code here assumes 2 bytes is one codepoint so only the Basic Multilingual Plane (BMP) is supported.
    // This internal encoding is more correctly described as UCS-2.
    // Strings are stored in the endian-ness appropriate for the current platform.
    struct stritem_t;

    struct strdata_t  // 32 bytes
    {
        u16*       m_ptr;  // UCS-2
        stritem_t* m_slices;
        s32        m_len;
        s32        m_ref;
        arena_t*   m_arena;

        inline s64 cap() const { return m_len; }
        strdata_t* attach()
        {
            m_ref++;
            return this;
        }
    };

    struct stritem_t  // 32 bytes
    {
        s32        m_from;
        s32        m_to;
        stritem_t* m_next;
        strdata_t* m_data;
        arena_t*   m_arena;

        inline bool is_empty() const { return m_from == m_to; }
        inline s64  cap() const { return m_data->cap(); }
        inline s64  len() const { return m_to - m_from; }
        inline s64  size() const { return m_to - m_from; }

        void release(strdata_t* data);
        void clone(strdata_t* data, const stritem_t* str);

        void add(stritem_t* node);
        void rem();

        void invalidate();
    };

    stritem_t* make_item(arena_t* arena, s32 from, s32 to, strdata_t* data)
    {
        stritem_t* v = (stritem_t*)arena->allocate(sizeof(stritem_t));
        v->m_from    = from;
        v->m_to      = to;
        v->m_data    = data->attach();
        v->m_arena   = arena;
        return v;
    }

    static inline void get_from_to(crunes_t const& runes, u32& from, u32& to)
    {
        from = (u32)(runes.m_ascii.m_str);
        to   = (u32)(runes.m_ascii.m_end);
    }

    static inline crunes_t get_crunes(strdata_t const* data, s32 from, s32 to)
    {
        crunes_t r;
        r.m_utf16.m_flags = utf16::TYPE;
        r.m_utf16.m_bos   = (utf16::prune)data->m_ptr;
        r.m_utf16.m_eos   = data->m_len;
        r.m_utf16.m_str   = from;
        r.m_utf16.m_end   = to;
        return r;
    }
    static inline runes_t get_runes(strdata_t* data, s32 from, s32 to)
    {
        runes_t r;
        r.m_utf16.m_flags = utf16::TYPE;
        r.m_utf16.m_bos   = (utf16::prune)data->m_ptr;
        r.m_utf16.m_eos   = data->m_len;
        r.m_utf16.m_str   = from;
        r.m_utf16.m_end   = to;
        return r;
    }

    class arena_t
    {
        utf16::prune m_bos;
        utf16::prune m_eos;

    public:
        arena_t(void* buffer, s32 _size)
            : m_bos((utf16::prune)buffer)
            , m_eos(m_bos + (_size / sizeof(utf16::rune)))
        {
        }

        ~arena_t() {}

        void* allocate(s32 size)
        {
            void* ptr = m_bos;
            m_bos += size;
            return ptr;
        }

        void deallocate(void* ptr) {}
    };

    static strdata_t* alloc_data(arena_t* arena, s32 _strlen)
    {
        strdata_t* data = (strdata_t*)arena->allocate(sizeof(strdata_t));
        data->m_len     = _strlen;
        data->m_ref     = 1;

        utf16::prune strdata = (utf16::prune)arena->allocate(_strlen);
        data->m_ptr          = strdata;
        data->m_arena        = arena;

        return data;
    }

    static void resize_data(strdata_t* data, s32 new_size)
    {
        if (data->m_len != new_size)
        {
            data->m_len         = new_size;
            utf16::prune newptr = (utf16::prune)data->m_arena->allocate(new_size);

            runes_t trunes = get_runes(data, 0, data->m_len);
            runes_t nrunes(newptr, newptr + new_size);
            copy(trunes, nrunes);
        }
    }

    static strdata_t* unique_data(strdata_t* data, s32 from, s32 to)
    {
        s32        len      = to - from;
        strdata_t* newdata  = (strdata_t*)data->m_arena->allocate(len);
        newdata->m_len      = len;
        newdata->m_ref      = 1;
        newdata->m_arena    = data->m_arena;
        utf16::prune newptr = (utf16::prune)data->m_arena->allocate(len);
        newdata->m_ptr      = newptr;
        newdata->m_slices   = nullptr;

        runes_t trunes = get_runes(data, 0, data->m_len);
        runes_t nrunes(newptr, newptr + len);
        copy(trunes, nrunes);
        return newdata;
    }

    static void dealloc_data(strdata_t* data)
    {
        runes_t runes = get_runes(data, 0, 0);
        context_t::string_alloc()->deallocate(runes);
        context_t::system_alloc()->deallocate(data);
    }

    static void resize(stritem_t* str, s32 new_size)
    {
        if (new_size > str->cap())
        {
            resize_data(str->m_data, new_size);
        }
        else
        {
            // TODO: What about shrinking ?
        }
    }

    static bool is_view_of(stritem_t const* str, stritem_t const* part)
    {
        if (str->m_data == part->m_data)
        {
            if (part->m_from >= str->m_from)
            {
                if (part->m_to <= str->m_to)
                {
                    return true;
                }
            }
        }
        return false;
    }

    static bool narrow_view(stritem_t* v, s32 move)
    {
        if (v->size() > 0)
        {
            // if negative then narrow the left side
            if (move < 0)
            {
                move = -move;
                if (move <= v->size())
                {
                    v->m_from += move;
                    return true;
                }
            }
            else
            {
                if (move <= v->size())
                {
                    v->m_to -= move;
                    return true;
                }
            }
        }
        return false;
    }

    static bool move_view(stritem_t const* str, stritem_t* view, s32 move)
    {
        s32 const from = view->m_from + move;

        // Check if the move doesn't result in an out-of-bounds
        if (from < str->m_from, str->m_from)
            return false;

        // Check if the movement doesn't invalidate this view
        s32 const to = view->m_to + move;
        if (to > str->m_from, str->m_to)
            return false;

        // Movement is ok, new view is valid
        view->m_from = from;
        view->m_to   = to;
        return true;
    }

    static stritem_t* select_before(const stritem_t* str, const stritem_t* selection)
    {
        stritem_t* v = make_item(selection->m_arena, selection->m_from, selection->m_from, str->m_data);
        v->m_to      = v->m_from;
        v->m_from    = 0;
        return v;
    }
    static stritem_t* select_before_included(const stritem_t* str, const stritem_t* selection)
    {
        stritem_t* v = make_item(selection->m_arena, selection->m_from, selection->m_from, str->m_data);
        v->m_to      = v->m_to;
        v->m_from    = 0;
        return v;
    }

    static stritem_t* select_after(const stritem_t* str, const stritem_t* sel)
    {
        stritem_t* v = make_item(sel->m_arena, sel->m_to, sel->m_to, str->m_data);
        v->m_to      = str->m_from, str->m_to;
        v->m_from    = sel->m_from;
        return v;
    }

    static stritem_t* select_after_excluded(const stritem_t* str, const stritem_t* sel)
    {
        stritem_t* v = make_item(sel->m_arena, sel->m_to, sel->m_to, str->m_data);
        v->m_to      = str->m_from;
        v->m_from    = sel->m_to;
        return v;
    }

    static void insert_space(runes_t& r, s32 pos, s32 len)
    {
        s32 src = r.size() - 1;
        s32 dst = src + len;
        switch (r.get_type())
        {
            case ascii::TYPE:
                while (src >= pos)
                {
                    r.m_ascii.m_bos[dst--] = r.m_ascii.m_bos[src--];
                }
                r.m_ascii.m_end += len;
                r.m_ascii.m_bos[r.m_ascii.m_end] = '\0';
                break;
            case utf32::TYPE:
                while (src >= pos)
                {
                    r.m_utf32.m_bos[dst--] = r.m_utf32.m_bos[src--];
                }
                r.m_utf32.m_end += len;
                r.m_utf32.m_bos[r.m_utf32.m_end] = '\0';
                break;
        }
    }

    static void remove_space(runes_t& r, s32 pos, s32 len)
    {
        s32       src = pos + len;
        s32       dst = pos;
        s32 const end = pos + len;
        switch (r.get_type())
        {
            case ascii::TYPE:
                while (src < r.size())
                {
                    r.m_ascii.m_bos[dst++] = r.m_ascii.m_bos[src++];
                }
                r.m_ascii.m_end -= len;
                r.m_ascii.m_bos[r.m_ascii.m_end] = '\0';
                break;
            case utf32::TYPE:
                while (src < r.size())
                {
                    r.m_utf32.m_bos[dst++] = r.m_utf32.m_bos[src++];
                }
                r.m_utf32.m_end -= len;
                r.m_utf32.m_bos[r.m_utf32.m_end] = '\0';
                break;
        }
    }

    static inline uchar16 get_char_unsafe(strdata_t* data, s32 from, s32 to, s32 index) { return data->m_ptr[from + index]; }

    static inline void set_char_unsafe(strdata_t* data, s32 from, s32 to, s32 index, uchar16 c) { data->m_ptr[from + index] = c; }

    static void insert(stritem_t* str, stritem_t const* pos, stritem_t const* insert)
    {
        if (insert->is_empty() || (str->m_data != pos->m_data))
            return;

        s32 dst = pos->m_from;
        adjust_active_views(str, INSERTION, dst, dst + insert->size());

        s32 const current_len = str->m_data->m_len;
        resize(str, str->m_data->m_len + insert->size() + 1);
        {
            //@TODO: it should be better to get an actual full view from the list of strings, currently we
            //       take the easy way and just take the whole allocated size as the full
            runes_t str_runes = get_runes(str->m_data, 0, current_len);
            insert_space(str_runes, dst, insert->size());
        }
        s32 src = 0;
        while (src < insert->size())
        {
            uchar32 const c = get_char_unsafe(insert->m_data, insert->m_from, insert->m_to, src);
            set_char_unsafe(str->m_data, str->m_from, str->m_to, dst, c);
            ++src;
            ++dst;
        }
    }

    static void remove(stritem_t* str, stritem_t const* selection)
    {
        if (selection->is_empty())
            return;

        if (is_view_of(str, selection))
        {
            //@TODO: it should be better to get an actual full view from the list of strings, currently we
            //       take the easy way and just take the whole allocated size as the full
            runes_t str_runes = get_runes(str->m_data, 0, str->m_data->m_len);
            remove_space(str_runes, selection->m_from, selection->size());

            // TODO: Decision to shrink the allocated memory of m_runes ?

            adjust_active_views(str, REMOVAL, selection->m_from, selection->m_to);
        }
    }

    static stritem_t* find(stritem_t* str, const stritem_t* _find)
    {
        stritem_t* sel = make_item(str->m_arena, 0, 0, str->m_data);
        if (_find->is_empty() == false)
        {
            s32 const strfrom  = str->m_from;
            s32 const strto    = str->m_to;
            s32 const strsize  = str->m_to - str->m_from;
            s32 const findfrom = _find->m_from;
            s32 const findto   = _find->m_to;
            s32 const findsize = _find->m_to - _find->m_from;

            s32 i = 0;
            s32 r = -1;
            while (i < strsize)
            {
                uchar32 const c = get_char_unsafe(str->m_data, strfrom, strto, i);
                if (c == get_char_unsafe(_find->m_data, findfrom, findto, 0))
                {
                    s32 j = 1;
                    while (j < findsize)
                    {
                        if (get_char_unsafe(str->m_data, strfrom, strto, i + j) != get_char_unsafe(_find->m_data, findfrom, findto, j))
                        {
                            break;
                        }
                        j++;
                    }
                    if (j == findsize)
                    {
                        r = i;
                        break;
                    }
                }
                i++;
            }

            if (r >= 0)
            {
                sel->m_from = r;
                sel->m_to   = r + findsize;
            }
            else
            {
                sel->m_from = sel->m_to = 0;
            }
        }
        return sel;
    }

    static void find_remove(stritem_t* _str, const stritem_t* _find)
    {
        stritem_t* strvw = make_item(_str->m_arena, 0, 0, _str->m_data);
        stritem_t* sel   = find(strvw, _find);
        if (sel->is_empty() == false)
        {
            remove(_str, sel);
        }
    }

    static void find_replace(stritem_t* str, const stritem_t* _find, const stritem_t* replace)
    {
        stritem_t* strvw  = str;
        stritem_t* remove = find(strvw, _find);
        if (remove->is_empty() == false)
        {
            s32 const remove_from = remove->m_from;
            s32 const remove_len  = remove->size();
            s32 const diff        = remove_len - replace->size();
            if (diff > 0)
            {
                // The string to replace the selection with is smaller, so we have to remove
                // some space from the string.
                runes_t str_runes = get_runes(str->m_data, str->m_from, str->m_to);
                remove_space(str_runes, remove_from, diff);

                // TODO: Decision to shrink the allocated memory of m_runes ?

                adjust_active_views(str, REMOVAL, remove_from, remove_from + diff);
            }
            else if (diff < 0)
            {
                // The string to replace the selection with is longer, so we have to insert some
                // space into the string.
                resize(str, str->size() + (-diff));
                runes_t str_runes = get_runes(str->m_data, str->m_from, str->m_to);
                insert_space(str_runes, remove_from, -diff);

                adjust_active_views(str, INSERTION, remove_from, remove_from + -diff);
            }
            // Copy string 'remove' into the (now) same size selection space
            s32          src  = 0;
            s32          dst  = remove_from;
            s32 const    end  = remove_from + replace->size();
            utf32::prune pdst = (utf32::prune)str->m_data->m_ptr;
            while (src < replace->size())
            {
                pdst[dst++] = get_char_unsafe(replace->m_data, replace->m_from, replace->m_to, src++);
            }
        }
    }

    static bool contains(const stritem_t* str, uchar16 find)
    {
        s32 len = str->len();
        s32 i   = 0;
        while (i < len)
        {
            uchar32 const c = get_char_unsafe(str->m_data, str->m_from, str->m_to, i);
            if (c == find)
                return true;
        }
        return false;
    }

    static void remove_any(stritem_t* str, const stritem_t* any)
    {
        // Remove any of the characters in @charset from @str
        s32 const strfrom = str->m_from;
        s32 const strto   = str->m_to;
        s32 const strsize = str->m_to - str->m_from;

        s32 d = 0;
        s32 i = 0;
        s32 r = -1;
        while (i < strsize)
        {
            uchar16 const c = get_char_unsafe(str->m_data, strfrom, strto, i);
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
                    s32 const gap = r - d;
                    adjust_active_views(str, REMOVAL, r - gap, i - gap);
                    r = -1;
                }

                if (i > d)
                {
                    set_char_unsafe(str->m_data, strfrom, strto, d, c);
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
            while (i < str->m_data->m_len)
            {
                uchar32 const c = get_char_unsafe(str->m_data, strfrom, strto, i);
                set_char_unsafe(str->m_data, strfrom, strto, d, c);
                i++;
                d++;
            }

            str->m_data->m_len -= l;
            utf32::prune pdst        = (utf32::prune)str->m_data->m_ptr;
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

    static s32 compare(s32 lhs_from, s32 lhs_to, s32 rhs_from, s32 rhs_to)
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

    static inline s32 compute_range_overlap(s32 lhs_from, s32 lhs_to, s32 rhs_from, s32 rhs_to)
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
    static const s32 REMOVAL   = 0;
    static const s32 INSERTION = 1;
    static const s32 CLEARED   = 2;
    static const s32 RELEASED  = 3;
    static void      adjust_active_view(stritem_t* v, s32 op_code, s32 rhs_from, s32 rhs_to)
    {
        s32 lhs_from = v->m_from;
        s32 lhs_to   = v->m_to;

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
                s32 const c = compare(lhs_from, lhs_to, rhs_from, rhs_to);
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
                        s32 const overlap = compute_range_overlap(lhs_from, lhs_to, rhs_from, rhs_to);
                        lhs_from += overlap;
                        lhs_to -= ((rhs_to - rhs_from) - overlap);
                        lhs_from -= ((rhs_to - rhs_from) - overlap);
                    }
                    break;
                    case RIGHT | OVERLAP: lhs_to -= compute_range_overlap(lhs_from, lhs_to, rhs_from, rhs_to); break;
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
                s32 const c = compare(lhs_from, lhs_to, rhs_from, rhs_to);
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

    static void adjust_active_views(stritem_t* list, s32 op_code, s32 op_range_from, s32 op_range_to)
    {
        stritem_t* iter = list;
        do
        {
            adjust_active_view(iter, op_code, op_range_from, op_range_to);
            iter = iter->m_next;
        } while (iter != list);
    }

    static stritem_t  s_default_item;
    static strdata_t  s_default_data;
    static uchar16    s_default_str[4] = {0, 0, 0, 0};
    static stritem_t* get_default_string_item()
    {
        static stritem_t* s_default_item_ptr = nullptr;
        if (s_default_item_ptr == nullptr)
        {
            s_default_item_ptr = &s_default_item;

            s_default_data.m_ptr    = (utf16::prune)s_default_str;
            s_default_data.m_len    = 0;
            s_default_data.m_ref    = 1;
            s_default_data.m_arena  = nullptr;
            s_default_data.m_slices = s_default_item_ptr;

            s_default_item.m_arena = nullptr;
            s_default_item.m_data  = &s_default_data;
            s_default_item.m_from  = 0;
            s_default_item.m_to    = 0;
            s_default_item.m_next  = s_default_item_ptr;
        }
        return s_default_item_ptr;
    }

    static inline bool is_default_string_item(stritem_t* item) { return item == &s_default_item; }
    static inline bool is_default_string_data(strdata_t* data) { return data == &s_default_data; }

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------

    void stritem_t::add(stritem_t* node)
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

    void stritem_t::rem()
    {
        if (m_next != nullptr)
        {
            stritem_t* head = m_data->m_slices;
            if (head == this)
            {
                m_data->m_slices = head->m_next;
            }
            else
            {
                while (head->m_next != this)
                {
                    head = head->m_next;
                }
                head->m_next = m_next;
            }
            m_next = nullptr;
        }
    }

    void stritem_t::invalidate()
    {
        rem();
        m_data = nullptr;
        m_from = 0;
        m_to   = 0;
    }

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------

    string_t string_t::clone() const
    {
        if (m_item->len() > 0)
        {
            strdata_t* data = unique_data(m_item->m_data, m_item->m_from, m_item->m_to);
            stritem_t* item = make_item(m_item->m_arena, 0, data->m_len, data);
            return string_t(item);
        }
        return string_t();
    }

    string_t::string_t()
    {
        m_item  = get_default_string_item();
    }

    string_t::~string_t() { release(); }

    string_t::string_t(const char* str)
    {
        crunes_t srcrunes(str);

        s32 const strlen  = srcrunes.size();
        s32 const strtype = ascii::TYPE;

        s32 from = 0;
        s32 to   = strlen;

        if (strlen > 0)
        {
            m_data        = alloc_data(strlen, strtype);
            runes_t runes = get_runes(m_data, from, to);
            copy(srcrunes, runes);
        }
        else
        {
            m_data = get_default_string_item();
        }

        add_to_list(nullptr);
    }

    string_t::string_t(s32 _len, s32 _type)
        : m_data(nullptr)
    {
        const s32 strlen  = _len;
        const s32 strtype = _type;

        if (strlen > 0)
        {
            m_data = alloc_data(_len, _type);
        }
        else
        {
            m_data = get_default_string_item();
        }

        add_to_list(nullptr);
    }

    string_t::string_t(const string_t& other)
        : m_from(other.m_from)
        , m_to(other.m_to)
        , m_data(nullptr)
        , m_next(nullptr)
        , m_prev(nullptr)
    {
        clone(other);
    }

    string_t::string_t(const string_t& left, const string_t& right)
    {
        const s32 strlen  = left.size() + right.size();
        const s32 strtype = math::max(left.m_data->m_type, right.m_data->m_type);

        crunes_t leftrunes  = get_crunes(left.m_data, left.m_from, left.m_to);
        crunes_t rightrunes = get_crunes(right.m_data, right.m_from, right.m_to);

        runes_t strdata;
        concatenate(strdata, leftrunes, rightrunes, context_t::string_alloc(), 16);

        m_data         = (str_data_t*)context_t::system_alloc()->allocate(sizeof(str_data_t), sizeof(void*));
        m_data->m_type = left.m_data->m_type;
        m_data->m_len  = strdata.cap();
        m_data->m_ptr  = strdata.m_ascii.m_bos;
    }

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    s32  string_t::size() const { return m_item->len(); }
    s32  string_t::cap() const { return m_item->cap(); }
    bool string_t::is_empty() const { return m_item->is_empty(); }

    void string_t::clear()
    {
        if (m_next == this && m_prev == this && !is_default_string_data(m_data))
        {
            dealloc_data(m_data);
            m_data = get_default_string_item();
        }
    }

    string_t string_t::slice() const
    {
        string_t str;
        str->m_from = m_from;
        str->m_to   = m_to;
        str->m_data = m_data;
        add_to_list(&str);
        return str;
    }

    string_t string_t::operator()(s32 _to) const
    {
        const s32 from = m_from + _to;
        const s32 to   = m_data->m_len;
        string_t  str;
        str->m_data = m_data;
        str->m_from = m_from;
        str->m_to   = math::min(from, to);
        add_to_list(&str);
        return str;
    }
    string_t string_t::operator()(s32 _from, s32 _to) const
    {
        math::sort(_from, _to);
        const s32 from = math::min(m_from + _from, m_to);
        const s32 to   = math::min(m_from + _to, m_to);
        string_t  str;
        str->m_data = m_data;
        str->m_from = m_from;
        str->m_to   = m_to;
        add_to_list(&str);
        return str;
    }
    uchar32 string_t::operator[](s32 index) const
    {
        if (index >= size())
            return '\0';
        return get_char_unsafe(this->m_data, m_from, m_to, index);
    }

    string_t& string_t::operator=(const char* other)
    {
        crunes_t srcrunes(other);

        release();

        s32 strlen  = srcrunes.size();
        s32 strtype = ascii::TYPE;

        if (strlen > 0)
        {
            m_data        = alloc_data(strlen, ascii::TYPE);
            runes_t runes = get_runes(m_data, m_from, m_to);
            copy(srcrunes, runes);
        }
        else
        {
            m_data = get_default_string_item();
        }

        m_from = 0;
        m_to   = strlen;
        add_to_list(nullptr);

        return *this;
    }

    string_t& string_t::operator=(const string_t& other)
    {
        if (this != &other)
        {
            if (this->m_data == other.m_data)
            {
                this->m_from = other.m_from;
                this->m_to   = other.m_to;
            }
            else
            {
                clone(other);
            }
        }
        return *this;
    }

    bool string_t::operator==(const string_t& other) const
    {
        if (size() != other.size())
            return false;
        for (s32 i = 0; i < size(); i++)
        {
            uchar32 const lc = get_char_unsafe(this->m_data, this->m_from, this->m_to, i);
            uchar32 const rc = get_char_unsafe(other.m_data, other.m_from, other.m_to, i);
            if (lc != rc)
                return false;
        }
        return true;
    }

    bool string_t::operator!=(const string_t& other) const
    {
        if (size() != other.size())
            return true;
        for (s32 i = 0; i < size(); i++)
        {
            uchar32 const lc = get_char_unsafe(this->m_data, this->m_from, this->m_to, i);
            uchar32 const rc = get_char_unsafe(other.m_data, other.m_from, other.m_to, i);
            if (lc != rc)
                return true;
        }
        return false;
    }

    void string_t::attach(string_t& str)
    {
        if (&str == this)
            return;
        string_t* next = str->m_next;
        str->m_next    = this;
        this->m_next   = next;
        this->m_prev   = &str;
        next->m_prev   = this;
    }

    void string_t::release()
    {
        // If we are the only one in the list then it means we can deallocate 'string_data'
        if (m_next == this && m_prev == this && !is_default_string_data(m_data))
        {
            dealloc_data(m_data);
        }
        rem_from_list();
        m_data = nullptr;
    }

    void string_t::clone(string_t const& str)
    {
        release();

        crunes_t srcrunes = get_crunes(str->m_data, str->m_from, str->m_to);

        m_data           = alloc_data(srcrunes.size(), str->m_data->m_type);
        runes_t dstrunes = get_runes(m_data, m_from, m_to);
        copy(srcrunes, dstrunes);

        m_from = 0;
        m_to   = srcrunes.size();
        add_to_list(nullptr);
    }

    //------------------------------------------------------------------------------
    string_t selectUntil(const string_t& str, uchar32 find)
    {
        for (s32 i = 0; i < str->size(); i++)
        {
            uchar32 const c = get_char_unsafe(str, i);
            if (c == find)
            {
                return str(0, i);
            }
        }
        return get_default();
    }

    string_t selectUntil(const string_t& str, const string_t& find)
    {
        string_t v = str(0, find.size());
        while (!v->is_empty())
        {
            if (v == find)
            {
                // So here we have a view with the size of the @find string on
                // string @str that matches the string @find and we need to return
                // a string view that exist before view @v->
                return select_before(str, v);
            }
            if (!move_view(str, v, 1))
                break;
        }
        return get_default();
    }

    string_t selectUntilLast(const string_t& str, uchar32 find)
    {
        for (s32 i = str->size() - 1; i >= 0; --i)
        {
            uchar32 const c = str[i];
            if (c == find)
            {
                return str(0, i);
            }
        }
        return get_default();
    }

    string_t selectUntilLast(const string_t& str, const string_t& find)
    {
        string_t v = str(str->size() - find.size(), str->size());
        while (!v->is_empty())
        {
            if (v == find)
            {
                // So here we have a view with the size of the @find string on
                // string @str that matches the string @find and we need to return
                // a string view that exist before view @v->
                return select_before(str, v);
            }
            if (!move_view(str, v, -1))
                break;
        }
        return get_default();
    }

    string_t selectUntilIncluded(const string_t& str, const string_t& find)
    {
        string_t v = str(0, find.size());
        while (!v->is_empty())
        {
            if (v == find)
            {
                // So here we have a view with the size of the @find string on
                // string @str that matches the string @find and we need to return
                // a string view that exist before and includes view @v->
                return select_before_included(str, v);
            }
            if (!move_view(str, v, 1))
                break;
        }
        return get_default();
    }

    string_t selectUntilEndExcludeSelection(const string_t& str, const string_t& selection) { return select_after_excluded(str, selection); }

    string_t selectUntilEndIncludeSelection(const string_t& str, const string_t& selection) { return select_after(str, selection); }

    bool isUpper(const string_t& str)
    {
        for (s32 i = 0; i < str->size(); i++)
        {
            uchar32 const c = str[i];
            if (is_lower(c))
                return false;
        }
        return true;
    }

    bool isLower(const string_t& str)
    {
        for (s32 i = 0; i < str->size(); i++)
        {
            uchar32 const c = get_char_unsafe(str, i);
            if (is_upper(c))
                return false;
        }
        return true;
    }

    bool isCapitalized(const string_t& s)
    {
        stritem_t* str = s->m_item;

        s32 i = 0;
        while (i < str->size())
        {
            uchar32 c = '\0';
            while (i < str->size())
            {
                c = get_char_unsafe(str, i);
                if (!is_space(c))
                    break;
                i += 1;
            }

            if (is_upper(c))
            {
                i += 1;
                while (i < str->size())
                {
                    c = get_char_unsafe(str, i);
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

    bool isQuoted(const string_t& str) { return isQuoted(str, '"'); }

    bool isQuoted(const string_t& str, uchar32 inQuote) { return isDelimited(str, inQuote, inQuote); }

    bool isDelimited(const string_t& str, uchar32 inLeft, uchar32 inRight)
    {
        if (str->is_empty())
            return false;
        s32 const first = 0;
        s32 const last  = str->size() - 1;
        return str[first] == inLeft && str[last] == inRight;
    }

    uchar32 firstChar(const string_t& str)
    {
        s32 const first = 0;
        return str[first];
    }

    uchar32 lastChar(const string_t& str)
    {
        s32 const last = str->size() - 1;
        return str[last];
    }

    bool startsWith(const string_t& str, string_t const& start)
    {
        string_t v = str(0, start.size());
        if (!v->is_empty())
            return (v == start);
        return false;
    }

    bool endsWith(const string_t& str, string_t const& end)
    {
        string_t v = str(str->size() - end.size(), str->size());
        if (!v->is_empty())
            return (v == end);
        return false;
    }

    string_t find(string_t& str, uchar32 find)
    {
        for (s32 i = 0; i < str->size(); i++)
        {
            uchar32 const c = str[i];
            if (c == find)
                return str(i, i + 1);
        }
        return get_default();
    }

    string_t find(string_t& inStr, const char* inFind)
    {
        crunes_t strfind(inFind);
        crunes_t strstr = get_crunes(inStr);

        crunes_t strfound = find(strstr, strfind);
        if (strfound.is_empty() == false)
        {
            s32 from, to;
            get_from_to(strfound, from, to);
            return inStr(from, to);
        }
        return get_default();
    }

    string_t find(string_t& str, const string_t& find)
    {
        string_t v = str(0, find.size());
        while (!v->is_empty())
        {
            if (v == find)
            {
                // So here we have a view with the size of the @find string on
                // string @str that matches the string @find.
                return v(0, v->size());
            }
            if (!move_view(str, v, 1))
                break;
        }
        return get_default();
    }

    string_t findLast(const string_t& str, const string_t& find)
    {
        string_t v = str(str->size() - find.size(), str->size());
        while (!v->is_empty())
        {
            if (v == find)
            {
                // So here we have a view with the size of the @find string on
                // string @str that matches the string @find.
                return v(0, v->size());
            }
            if (!move_view(str, v, -1))
                break;
        }
        return get_default();
    }

    string_t findOneOf(const string_t& str, const string_t& charset)
    {
        for (s32 i = 0; i < str->size(); i++)
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
        return get_default();
    }

    string_t findOneOfLast(const string_t& str, const string_t& charset)
    {
        for (s32 i = str->size() - 1; i >= 0; i--)
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
        return get_default();
    }

    s32 compare(const string_t& lhs, const string_t& rhs)
    {
        if (lhs.size() < rhs.size())
            return -1;
        if (lhs.size() > rhs.size())
            return 1;

        for (s32 i = 0; i < lhs.size(); i++)
        {
            uchar32 const lc = get_char_unsafe(lhs, i);
            uchar32 const rc = get_char_unsafe(rhs, i);
            if (lc < rc)
                return -1;
            else if (lc > rc)
                return 1;
        }
        return 0;
    }

    bool isEqual(const string_t& lhs, const string_t& rhs) { return compare(lhs, rhs) == 0; }

    bool contains(const string_t& str, const string_t& contains)
    {
        string_t v = str(0, contains.size());
        while (!v->is_empty())
        {
            if (v == contains)
            {
                // So here we have a view with the size of the @find string on
                // string @str that matches the string @find.
                return true;
            }
            if (!move_view(str, v, 1))
                break;
        }
        return false;
    }

    bool contains(const string_t& str, uchar32 contains)
    {
        for (s32 i = 0; i < str->size(); i++)
        {
            uchar32 const sc = get_char_unsafe(str, i);
            if (sc == contains)
            {
                return true;
            }
        }
        return false;
    }

    void concatenate_repeat(string_t& str, string_t const& con, s32 ntimes)
    {
        s32 const len = str->size() + (con.size() * ntimes) + 1;
        resize(str, len);
        for (s32 i = 0; i < ntimes; ++i)
        {
            concatenate(str, con);
        }
    }

    s32 string_t::format(string_t const& format, const va_t* argv, s32 argc)
    {
        release();

        const s32 len = cprintf_(get_crunes(format.m_data, format.m_from, format.m_to), argv, argc);

        str_data_t* ndata = alloc_data(len, format.m_data->m_type);
        m_next            = this;
        m_prev            = this;

        runes_t str = get_runes(ndata, 0, 0);
        sprintf_(str, get_crunes(format.m_data, format.m_from, format.m_to), argv, argc);
        switch (m_data->m_type)
        {
            case ascii::TYPE: m_to = (s32)(str->m_ascii.m_end); break;
            case utf32::TYPE: m_to = (s32)(str->m_utf32.m_end); break;
        }
        return len;
    }

    s32 string_t::formatAdd(string_t const& format, const va_t* argv, s32 argc)
    {
        const s32 len = cprintf_(get_crunes(format.m_data, format.m_from, format.m_to), argv, argc);
        resize(*this, len);
        runes_t str        = get_runes(m_data, m_from, m_to);
        str->m_ascii.m_str = str->m_ascii.m_end;
        sprintf_(str, get_crunes(format.m_data, format.m_from, format.m_to), argv, argc);
        switch (m_data->m_type)
        {
            case ascii::TYPE: m_to = (s32)(str->m_ascii.m_end); break;
            case utf32::TYPE: m_to = (s32)(str->m_utf32.m_end); break;
        }
        return len;
    }

    void insert(string_t& str, string_t const& pos, string_t const& insert) { insert(str, pos, insert); }

    void insert_after(string_t& str, string_t const& pos, string_t const& insert)
    {
        string_t after = select_after_excluded(str, pos);
        insert(str, after, insert);
    }

    void remove(string_t& str, string_t const& selection) { remove(str, selection); }

    void find_remove(string_t& str, const string_t& _find)
    {
        string_t sel = find(str, _find);
        if (sel->is_empty() == false)
        {
            remove(str, sel);
        }
    }

    void find_replace(string_t& str, const string_t& find, const string_t& replace) { find_replace(str, find, replace); }

    void remove_any(string_t& str, const string_t& any) { remove_any(str, any); }

    void replace_any(string_t& str, const string_t& any, uchar32 with)
    {
        // Replace any of the characters in @charset from @str with character @with
        for (s32 i = 0; i < str->size(); ++i)
        {
            uchar32 const c = get_char_unsafe(str, i);
            if (contains(any, c))
            {
                set_char_unsafe(str, i, with);
            }
        }
    }

    void upper(string_t& str)
    {
        for (s32 i = 0; i < str->size(); i++)
        {
            uchar32 c = get_char_unsafe(str, i);
            c         = to_upper(c);
            set_char_unsafe(str, i, c);
        }
    }

    void lower(string_t& str)
    {
        for (s32 i = 0; i < str->size(); i++)
        {
            uchar32 c = get_char_unsafe(str, i);
            c         = to_lower(c);
            set_char_unsafe(str, i, c);
        }
    }

    void capitalize(string_t& str)
    {
        // Standard separator is ' '
        bool prev_is_space = true;
        s32  i             = 0;
        while (i < str->size())
        {
            uchar32 c = get_char_unsafe(str, i);
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
                set_char_unsafe(str, i, c);
            }
            i++;
        }
    }

    void capitalize(string_t& str, string_t const& separators)
    {
        bool prev_is_space = false;
        s32  i             = 0;
        while (i < str->size())
        {
            uchar32 c = get_char_unsafe(str, i);
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
                set_char_unsafe(str, i, c);
            }
            i++;
        }
    }

    // Trim does nothing more than narrowing the <from, to>, nothing is actually removed
    // from the actual underlying string string_data.
    void trim(string_t& str)
    {
        trimLeft(str);
        trimRight(str);
    }

    void trimLeft(string_t& str)
    {
        for (s32 i = 0; i < str->size(); ++i)
        {
            uchar32 c = get_char_unsafe(str, i);
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
            {
                if (i > 0)
                {
                    narrow_view(str, -i);
                }
                return;
            }
        }
    }

    void trimRight(string_t& str)
    {
        s32 const last = str->size() - 1;
        for (s32 i = 0; i < str->size(); ++i)
        {
            uchar32 c = get_char_unsafe(str, last - i);
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
            {
                if (i > 0)
                {
                    narrow_view(str, i);
                }
                return;
            }
        }
    }

    void trim(string_t& str, uchar32 r)
    {
        trimLeft(str, r);
        trimRight(str, r);
    }

    void trimLeft(string_t& str, uchar32 r)
    {
        for (s32 i = 0; i < str->size(); ++i)
        {
            uchar32 c = get_char_unsafe(str, i);
            if (c != r)
            {
                if (i > 0)
                {
                    narrow_view(str, -i);
                }
                return;
            }
        }
    }

    void trimRight(string_t& str, uchar32 r)
    {
        s32 const last = str->size() - 1;
        for (s32 i = 0; i < str->size(); ++i)
        {
            uchar32 c = get_char_unsafe(str, last - i);
            if (c != r)
            {
                if (i > 0)
                {
                    narrow_view(str, i);
                }
                return;
            }
        }
    }

    void trim(string_t& str, string_t const& set)
    {
        trimLeft(str, set);
        trimRight(str, set);
    }

    void trimLeft(string_t& str, string_t const& set)
    {
        for (s32 i = 0; i < str->size(); ++i)
        {
            uchar32 c = get_char_unsafe(str, i);
            if (!contains(set, c))
            {
                if (i > 0)
                {
                    narrow_view(str, -i);
                }
                return;
            }
        }
    }

    void trimRight(string_t& str, string_t const& set)
    {
        s32 const last = str->size() - 1;
        for (s32 i = 0; i < str->size(); ++i)
        {
            uchar32 c = get_char_unsafe(str, last - i);
            if (!contains(set, c))
            {
                if (i > 0)
                {
                    narrow_view(str, i);
                }
                return;
            }
        }
    }

    void trimQuotes(string_t& str) { trimDelimiters(str, '"', '"'); }

    void trimQuotes(string_t& str, uchar32 quote) { trimDelimiters(str, quote, quote); }

    void trimDelimiters(string_t& str, uchar32 left, uchar32 right)
    {
        trimLeft(str, left);
        trimRight(str, right);
    }

    void reverse(string_t& str)
    {
        s32 const last = str->size() - 1;
        for (s32 i = 0; i < (last - i); ++i)
        {
            uchar32 l = get_char_unsafe(str, i);
            uchar32 r = get_char_unsafe(str, last - i);
            set_char_unsafe(str, i, r);
            set_char_unsafe(str, last - i, l);
        }
    }

    bool splitOn(string_t& str, uchar32 inChar, string_t& outLeft, string_t& outRight)
    {
        outLeft = selectUntil(str, inChar);
        if (outLeft.is_empty())
            return false;
        outRight = str(outLeft.size(), str->size());
        trimRight(outLeft, inChar);
        return true;
    }

    bool splitOn(string_t& str, string_t& inStr, string_t& outLeft, string_t& outRight)
    {
        outLeft = selectUntil(str, inStr);
        if (outLeft.is_empty())
            return false;
        outRight = str(outLeft.size() + inStr.size(), str->size());
        return true;
    }

    bool splitOnLast(string_t& str, uchar32 inChar, string_t& outLeft, string_t& outRight)
    {
        outLeft = selectUntilLast(str, inChar);
        if (outLeft.is_empty())
            return false;
        outRight = str(outLeft.size(), str->size());
        trimRight(outLeft, inChar);
        return true;
    }

    bool splitOnLast(string_t& str, string_t& inStr, string_t& outLeft, string_t& outRight)
    {
        outLeft = selectUntilLast(str, inStr);
        if (outLeft.is_empty())
            return false;
        outRight = str(outLeft.size() + inStr.size(), str->size());
        return true;
    }

    void concatenate(string_t& str, const string_t& con) { resize(str, str->size() + con.size() + 1); }

    //------------------------------------------------------------------------------
    static void user_case_for_string()
    {
        string_t str("This is an ascii string that will be converted to UTF-32");

        string_t strvw  = str->slice();
        string_t ascii  = ("ascii");
        string_t substr = find(strvw, ascii.slice());
        upper(substr);

        string_t converted("converted ");
        find_remove(strvw, converted.slice());
    }

}  // namespace ncore
