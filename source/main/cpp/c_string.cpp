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

    struct string_t::data_t  // 32 bytes
    {
        u16*                  m_ptr;   // UCS-2
        string_t::instance_t* m_head;  // The first view of this string, single linked list of instances
        u32                   m_len;
        s32                   m_ref;
        arena_t*              m_arena;

        inline s64 cap() const { return m_len; }

        string_t::data_t* attach()
        {
            m_ref++;
            return this;
        }

        string_t::data_t* detach();
    };

    struct string_t::instance_t  // 32 bytes
    {
        string_t::range_t     m_range;
        string_t::instance_t* m_next;
        string_t::data_t*     m_data;
        arena_t*              m_arena;

        inline bool is_empty() const { return m_range.is_empty(); }
        inline s64  cap() const { return m_data->cap(); }
        inline s64  size() const { return m_range.get_length(); }

        string_t::instance_t* clone_full(arena_t* instance_arena) const;
        string_t::instance_t* clone_slice(arena_t* instance_arena, arena_t* data_arena) const;

        void add(string_t::instance_t* node);
        void rem();

        string_t::instance_t* release();
        void                  invalidate();
    };

    static string_t::instance_t s_default_item;
    static string_t::data_t     s_default_data;
    static uchar16              s_default_str[4] = {0, 0, 0, 0};

    static inline string_t::data_t* get_default_data() { return &s_default_data; }

    static string_t::instance_t* get_default_instance()
    {
        static string_t::instance_t* s_default_item_ptr = nullptr;
        if (s_default_item_ptr == nullptr)
        {
            s_default_item_ptr = &s_default_item;

            s_default_data.m_ptr   = (utf16::prune)s_default_str;
            s_default_data.m_len   = 0;
            s_default_data.m_ref   = 1;
            s_default_data.m_arena = nullptr;
            s_default_data.m_head  = s_default_item_ptr;

            s_default_item.m_arena = nullptr;
            s_default_item.m_data  = get_default_data();
            s_default_item.m_range = {0, 0};
            s_default_item.m_next  = s_default_item_ptr;
        }
        return s_default_item_ptr;
    }

    static inline bool is_default_instance(string_t::instance_t* item) { return item == &s_default_item; }
    static inline bool is_default_data(string_t::data_t* data) { return data == &s_default_data; }

    string_t::instance_t* alloc_instance(arena_t* arena, string_t::range_t range, string_t::data_t* data)
    {
        if (data == nullptr)
            data = get_default_data();

        string_t::instance_t* v = (string_t::instance_t*)arena->allocate(sizeof(string_t::instance_t));
        v->m_range              = range;
        v->m_data               = data->attach();
        v->m_arena              = arena;
        return v;
    }

    static inline void get_from_to(crunes_t const& runes, u32& from, u32& to)
    {
        from = runes.m_ascii.m_str;
        to   = runes.m_ascii.m_end;
    }

    static inline crunes_t get_crunes(string_t::data_t const* data, u32 from, u32 to)
    {
        crunes_t r;
        r.m_utf16.m_flags = utf16::TYPE;
        r.m_utf16.m_bos   = (utf16::prune)data->m_ptr;
        r.m_utf16.m_eos   = data->m_len;
        r.m_utf16.m_str   = from;
        r.m_utf16.m_end   = to;
        return r;
    }
    static inline crunes_t get_crunes(string_t::instance_t const* inst, u32 from, u32 to) { return get_crunes(inst->m_data, from, to); }

    static inline runes_t get_runes(string_t::data_t* data, u32 from, u32 to)
    {
        runes_t r;
        r.m_utf16.m_flags = utf16::TYPE;
        r.m_utf16.m_bos   = (utf16::prune)data->m_ptr;
        r.m_utf16.m_eos   = data->m_len;
        r.m_utf16.m_str   = from;
        r.m_utf16.m_end   = to;
        return r;
    }
    static inline runes_t get_runes(string_t::instance_t* inst, u32 from, u32 to) { return get_runes(inst->m_data, from, to); }

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

    static string_t::data_t* alloc_data(arena_t* arena, s32 _strlen)
    {
        string_t::data_t* data = (string_t::data_t*)arena->allocate(sizeof(string_t::data_t));
        data->m_len            = _strlen;
        data->m_ref            = 1;

        utf16::prune strdata = (utf16::prune)arena->allocate(_strlen);
        data->m_ptr          = strdata;
        data->m_arena        = arena;

        return data;
    }

    static void resize_data(string_t::data_t* data, s32 new_size)
    {
        if (data->m_len != new_size)
        {
            data->m_len         = new_size;
            utf16::prune newptr = (utf16::prune)data->m_arena->allocate(new_size);

            runes_t trunes = get_runes(data, 0, data->m_len);
            runes_t nrunes(newptr, newptr + new_size);
            nrunes::copy(trunes, nrunes);
        }
    }

    static string_t::data_t* unique_data(string_t::data_t* data, s32 from, s32 to)
    {
        s32               len     = to - from;
        string_t::data_t* newdata = (string_t::data_t*)data->m_arena->allocate(len);
        newdata->m_len            = len;
        newdata->m_ref            = 1;
        newdata->m_arena          = data->m_arena;
        utf16::prune newptr       = (utf16::prune)data->m_arena->allocate(len);
        newdata->m_ptr            = newptr;
        newdata->m_head           = nullptr;

        runes_t trunes = get_runes(data, 0, data->m_len);
        runes_t nrunes(newptr, newptr + len);
        nrunes::copy(trunes, nrunes);
        return newdata;
    }

    static void dealloc_data(string_t::data_t* data)
    {
        data->m_arena->deallocate(data->m_ptr);
        data->m_arena->deallocate(data);
    }

    static void resize(string_t::instance_t* str, s32 new_size)
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

    static bool is_view_of(string_t::instance_t const* parent, string_t::instance_t const* slice) { return (parent->m_data == slice->m_data) && (slice->m_range.is_inside(parent->m_range)); }

    static bool narrow_view(string_t::instance_t* v, s32 move)
    {
        if (v->size() > 0)
        {
            // if negative then narrow the left side
            if (move < 0)
            {
                move = -move;
                if (move <= v->size())
                {
                    v->m_range.m_from += move;
                    return true;
                }
            }
            else
            {
                if (move <= v->size())
                {
                    v->m_range.m_to -= move;
                    return true;
                }
            }
        }
        return false;
    }

    static bool move_view(string_t::instance_t const* str, string_t::range_t& view, s32 move)
    {
        s32 const from = view.m_from + move;

        // Check if the move doesn't result in an out-of-bounds
        if (from < str->m_range.m_from, str->m_range.m_from)
            return false;

        // Check if the movement doesn't invalidate this view
        s32 const to = view.m_to + move;
        if (to > str->m_range.m_from, str->m_range.m_to)
            return false;

        // Movement is ok, new view is valid
        view.m_from = from;
        view.m_to   = to;
        return true;
    }

    static void insert_space(runes_t& r, s32 pos, s32 len)
    {
        s32 src = r.size() - 1;
        s32 dst = src + len;
        while (src >= pos)
        {
            r.m_utf16.m_bos[dst--] = r.m_utf16.m_bos[src--];
        }
        r.m_utf16.m_end += len;
        r.m_utf16.m_bos[r.m_utf16.m_end] = '\0';
    }

    static void remove_space(runes_t& r, s32 pos, s32 len)
    {
        s32       src = pos + len;
        s32       dst = pos;
        s32 const end = pos + len;
        while (src < r.size())
        {
            r.m_utf16.m_bos[dst++] = r.m_utf16.m_bos[src++];
        }
        r.m_utf16.m_end -= len;
        r.m_utf16.m_bos[r.m_utf16.m_end] = '\0';
    }

    static inline uchar16 get_char_unsafe(string_t::data_t* data, u32 from, u32 to, u32 index) { return data->m_ptr[from + index]; }
    static inline void    set_char_unsafe(string_t::data_t* data, u32 from, u32 to, u32 index, uchar16 c) { data->m_ptr[from + index] = c; }

    // forward declare
    static void adjust_active_views(string_t::instance_t* list, s32 op_code, s32 op_range_from, s32 op_range_to);

    static void string_insert(string_t::instance_t* str, string_t::range_t location, string_t::instance_t const* insert)
    {
        if (insert->is_empty())
            return;

        s32 const len = location.get_length();
        s32       pos = location.m_from;
        if (len < insert->size())
        {
            // The string to insert is larger than the selection, so we have to insert some
            // space into the string.
            resize(str, str->m_data->m_len + (insert->size() - len));
            runes_t str_runes = get_runes(str->m_data, 0, str->m_data->m_len);
            insert_space(str_runes, pos, insert->size() - len);

            adjust_active_views(str, INSERTION, pos, pos + insert->size() - len);
        }
        else if (len > insert->size())
        {
            // The string to insert is smaller than the selection, so we have to remove some
            // space from the string.
            runes_t str_runes = get_runes(str->m_data, 0, str->m_data->m_len);
            remove_space(str_runes, pos, len - insert->size());

            adjust_active_views(str, REMOVAL, pos, pos + len - insert->size());
        }
        else
        {
            // The string to insert is the same size as the selection, so we can just replace the
            // selection with the new string.
        }

        s32 src = 0;
        while (src < insert->size())
        {
            uchar32 const c = get_char_unsafe(insert->m_data, insert->m_range.m_from, insert->m_range.m_to, src);
            set_char_unsafe(str->m_data, str->m_range.m_from, str->m_range.m_to, pos, c);
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
            runes_t str_runes = get_runes(str->m_data, 0, str->m_data->m_len);
            remove_space(str_runes, selection.m_from, selection.get_length());

            // TODO: Decision to shrink the allocated memory of m_runes ?
            adjust_active_views(str, REMOVAL, selection.m_from, selection.m_to);
        }
    }

    static string_t::range_t find(string_t::instance_t* str, const string_t::instance_t* _find)
    {
        if (_find->is_empty() == false)
        {
            s32 const strfrom  = str->m_range.m_from;
            s32 const strto    = str->m_range.m_to;
            s32 const strsize  = str->m_range.m_to - str->m_range.m_from;
            s32 const findfrom = _find->m_range.m_from;
            s32 const findto   = _find->m_range.m_to;
            u32 const findsize = _find->m_range.m_to - _find->m_range.m_from;

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
                return {(u32)r, (u32)r + findsize};
        }
        return {0, 0};
    }

    static void find_remove(string_t::instance_t* _str, const string_t::instance_t* _find)
    {
        string_t::instance_t* strvw = alloc_instance(_str->m_arena, {0, 0}, _str->m_data);
        string_t::range_t sel   = find(strvw, _find);
        if (sel.is_empty() == false)
        {
            string_remove(_str, sel);
        }
    }

    static void find_replace(string_t::instance_t* str, const string_t::instance_t* _find, const string_t::instance_t* replace)
    {
        string_t::range_t remove = find(str, _find);
        if (remove.is_empty() == false)
        {
            s32 const remove_from = remove.m_from;
            s32 const remove_len  = remove.get_length();
            s32 const diff        = remove_len - replace->size();
            if (diff > 0)
            {
                // The string to replace the selection with is smaller, so we have to remove
                // some space from the string.
                runes_t str_runes = get_runes(str->m_data, str->m_range.m_from, str->m_range.m_to);
                remove_space(str_runes, remove_from, diff);

                // TODO: Decision to shrink the allocated memory of runes ?

                adjust_active_views(str, REMOVAL, remove_from, remove_from + diff);
            }
            else if (diff < 0)
            {
                // The string to replace the selection with is longer, so we have to insert some
                // space into the string.
                resize(str, str->size() + (-diff));
                runes_t str_runes = get_runes(str->m_data, str->m_range.m_from, str->m_range.m_to);
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
                pdst[dst++] = get_char_unsafe(replace->m_data, replace->m_range.m_from, replace->m_range.m_to, src++);
            }
        }
    }

    static bool contains(const string_t::instance_t* str, uchar16 find)
    {
        s32 len = str->size();
        s32 i   = 0;
        while (i < len)
        {
            uchar32 const c = get_char_unsafe(str->m_data, str->m_range.m_from, str->m_range.m_to, i);
            if (c == find)
                return true;
        }
        return false;
    }

    static void remove_any(string_t::instance_t* str, const string_t::instance_t* any)
    {
        // Remove any of the characters in @charset from @str
        s32 const strfrom = str->m_range.m_from;
        s32 const strto   = str->m_range.m_to;
        s32 const strsize = str->m_range.m_to - str->m_range.m_from;

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
    static void      adjust_active_view(string_t::instance_t* v, s32 op_code, s32 rhs_from, s32 rhs_to)
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

    static void adjust_active_views(string_t::instance_t* list, s32 op_code, s32 op_range_from, s32 op_range_to)
    {
        string_t::instance_t* iter = list;
        do
        {
            adjust_active_view(iter, op_code, op_range_from, op_range_to);
            iter = iter->m_next;
        } while (iter != list);
    }

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    string_t::data_t* string_t::data_t::detach()
    {
        m_ref--;
        if (m_ref == 0)
        {
            dealloc_data(this);
            return get_default_data();
        }
        return this;
    }

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    string_t::instance_t* string_t::instance_t::clone_full(arena_t* instance_arena) const
    {
        string_t::instance_t* v = alloc_instance(instance_arena, m_range, m_data->attach());
        return v;
    }

    string_t::instance_t* string_t::instance_t::clone_slice(arena_t* instance_arena, arena_t* data_arena) const
    {
        u32 const             strlen = m_range.get_length();
        string_t::data_t*     data   = unique_data(m_data, m_range.m_from, m_range.m_to);
        string_t::instance_t* v      = alloc_instance(instance_arena, {0, strlen}, data);
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
        if (m_next != nullptr)
        {
            string_t::instance_t* head = m_data->m_head;
            if (head == this)
            {
                m_data->m_head = head->m_next;
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

    string_t::instance_t* string_t::instance_t::release()
    {
        if (!is_default_instance(this))
        {
            if (!is_default_data(m_data))
            {
                if (m_data->m_ref <= 1)
                {
                    dealloc_data(m_data);
                    invalidate();
                }
                else
                {
                    m_data->m_ref--;
                }
            }
            m_arena->deallocate(this);
        }
        return get_default_instance();
    }

    void string_t::instance_t::invalidate()
    {
        m_data  = get_default_data();
        m_range = {0, 0};
    }

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------

    string_t string_t::clone() const
    {
        if (m_item->size() > 0)
        {
            string_t::data_t*     data = unique_data(m_item->m_data, m_item->m_range.m_from, m_item->m_range.m_to);
            string_t::instance_t* item = alloc_instance(m_item->m_arena, {0, data->m_len}, data);
            return string_t(item);
        }
        return string_t();
    }

    string_t::string_t() { m_item = get_default_instance(); }

    string_t::string_t(string_t::instance_t* item) { m_item = item; }

    string_t::string_t(arena_t* arena, const char* str)
    {
        crunes_t srcrunes(str);

        u32 const strlen = srcrunes.size();

        u32 from = 0;
        u32 to   = strlen;

        if (strlen > 0)
        {
            string_t::data_t* data = alloc_data(arena, strlen);
            m_item                 = alloc_instance(arena, {from, to}, data);
            runes_t runes          = get_runes(m_item->m_data, from, to);
            nrunes::copy(srcrunes, runes);
        }
        else
        {
            m_item = get_default_instance();
        }
    }

    string_t::string_t(arena_t* arena, s32 _len)
        : m_item(nullptr)
    {
        const s32 strlen = _len;

        if (strlen > 0)
        {
            string_t::data_t* data = alloc_data(arena, strlen);
            m_item                 = alloc_instance(arena, {0, 0}, data);
        }
        else
        {
            m_item = get_default_instance();
        }
    }

    string_t::string_t(arena_t* arena, const string_t& other) { m_item = other.m_item->clone_full(arena); }

    string_t::string_t(arena_t* arena, const string_t& left, const string_t& right)
    {
        const u32 strlen = left.size() + right.size();

        crunes_t leftrunes  = get_crunes(left.m_item, left.m_item->m_range.m_from, left.m_item->m_range.m_to);
        crunes_t rightrunes = get_crunes(right.m_item, right.m_item->m_range.m_from, right.m_item->m_range.m_to);

        string_t::data_t* data = alloc_data(arena, strlen);
        m_item                 = alloc_instance(arena, {0, strlen}, data);

        runes_t result = get_runes(m_item->m_data, m_item->m_range.m_from, m_item->m_range.m_to);
    }

    string_t::~string_t() { release(); }

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    s32  string_t::size() const { return m_item->size(); }
    s32  string_t::cap() const { return m_item->cap(); }
    bool string_t::is_empty() const { return m_item->is_empty(); }

    void string_t::clear() { release(); }

    string_t string_t::slice() const
    {
        string_t str;
        str.m_item->m_range.m_from = m_item->m_range.m_from;
        str.m_item->m_range.m_to   = m_item->m_range.m_to;
        str.m_item->m_data         = m_item->m_data->attach();
        m_item->add(str.m_item);
        return str;
    }

    string_t string_t::operator()(s32 _to) const
    {
        const s32 from = m_item->m_range.m_from + _to;
        const s32 to   = m_item->m_data->m_len;

        string_t str;
        str.m_item->m_data         = m_item->m_data->attach();
        str.m_item->m_range.m_from = m_item->m_range.m_from;
        str.m_item->m_range.m_to   = math::min(from, to);
        m_item->add(str.m_item);
        return str;
    }
    string_t string_t::operator()(s32 _from, s32 _to) const
    {
        math::sort(_from, _to);
        const s32 from = math::min(m_item->m_range.m_from + _from, m_item->m_range.m_to);
        const s32 to   = math::min(m_item->m_range.m_from + _to, m_item->m_range.m_to);

        string_t str;
        str.m_item->m_data         = m_item->m_data;
        str.m_item->m_range.m_from = m_item->m_range.m_from;
        str.m_item->m_range.m_to   = m_item->m_range.m_to;
        m_item->add(str.m_item);
        return str;
    }
    uchar32 string_t::operator[](s32 index) const
    {
        if (index >= size())
            return '\0';
        return get_char_unsafe(m_item->m_data, m_item->m_range.m_from, m_item->m_range.m_to, index);
    }

    string_t& string_t::operator=(const char* other)
    {
        crunes_t srcrunes(other);

        arena_t* arena  = m_item->m_arena;
        u32      strlen = srcrunes.size();

        release();

        if (strlen != 0)
        {
            string_t::data_t*     data  = alloc_data(arena, strlen);
            string_t::instance_t* item  = alloc_instance(arena, {0, strlen}, data);
            runes_t               runes = get_runes(m_item, m_item->m_range.m_from, m_item->m_range.m_to);
            nrunes::copy(srcrunes, runes);
        }
        else
        {
            m_item = get_default_instance();
        }

        m_item->m_range.m_from = 0;
        m_item->m_range.m_to   = strlen;

        return *this;
    }

    string_t& string_t::operator=(const string_t& other)
    {
        if (this != &other && m_item != other.m_item)
        {
            if (this->m_item->m_data == other.m_item->m_data)
            {
                m_item->m_range.m_from = other.m_item->m_range.m_from;
                m_item->m_range.m_to   = other.m_item->m_range.m_to;
            }
            else
            {
                m_item->m_data         = m_item->m_data->detach();
                m_item->m_data         = other.m_item->m_data->attach();
                m_item->m_range.m_from = other.m_item->m_range.m_from;
                m_item->m_range.m_to   = other.m_item->m_range.m_to;
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
            uchar32 const lc = get_char_unsafe(m_item->m_data, this->m_item->m_range.m_from, this->m_item->m_range.m_to, i);
            uchar32 const rc = get_char_unsafe(other.m_item->m_data, other.m_item->m_range.m_from, other.m_item->m_range.m_to, i);
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
            uchar32 const lc = get_char_unsafe(m_item->m_data, m_item->m_range.m_from, m_item->m_range.m_to, i);
            uchar32 const rc = get_char_unsafe(other.m_item->m_data, other.m_item->m_range.m_from, other.m_item->m_range.m_to, i);
            if (lc != rc)
                return true;
        }
        return false;
    }

    void string_t::release()
    {
        // If we are the only one in the list then it means we can deallocate 'string_data'
        if (!is_default_data(m_item->m_data))
        {
            dealloc_data(m_item->m_data);
        }
        m_item->rem();
        m_item = m_item->release();
    }

    string_t string_t::clone() const
    {
        crunes_t srcrunes = get_crunes(m_item->m_data, m_item->m_range.m_from, m_item->m_range.m_to);

        string_t::data_t* data     = alloc_data(m_item->m_data->m_arena, srcrunes.size());
        runes_t           dstrunes = get_runes(data, 0, srcrunes.size());
        nrunes::copy(srcrunes, dstrunes);

        string_t::instance_t* item = alloc_instance(m_item->m_arena, {0, srcrunes.size()}, data);
        item->m_range.m_from       = 0;
        item->m_range.m_to         = srcrunes.size();
        item->add(this->m_item);

        return string_t(item);
    }

    //------------------------------------------------------------------------------
    //------------ range -----------------------------------------------------------
    //------------------------------------------------------------------------------
    class string_functions_t : public string_t
    {
    public:
        static inline uchar32 get_char_unsafe(string_t const& str, u32 index) { return ncore::get_char_unsafe(str.m_item->m_data, str.m_item->m_range.m_from, str.m_item->m_range.m_to, index); }

        // select parst of the string, return local view range
        static range_t selectBefore(const instance_t* str, const instance_t* sel) { return {str->m_range.m_from - str->m_range.m_from, sel->m_range.m_from - str->m_range.m_from}; }
        static range_t selectBeforeIncluded(const instance_t* str, const instance_t* sel) { return {str->m_range.m_from - str->m_range.m_from, sel->m_range.m_to - str->m_range.m_from}; }
        static range_t selectAfter(const instance_t* str, const instance_t* sel) { return {sel->m_range.m_to - str->m_range.m_from, str->m_range.m_to - str->m_range.m_from}; }
        static range_t selectAfterIncluded(const instance_t* str, const instance_t* sel) { return {sel->m_range.m_from - str->m_range.m_from, str->m_range.m_to - str->m_range.m_from}; }

        static bool isEqual(string_t const& str, string_t::range_t strview, string_t const& rhs)
        {
            if (strview.get_length() != rhs.size())
                return false;

            for (s32 i = 0; i < strview.get_length(); i++)
            {
                uchar32 const lc = get_char_unsafe(str, strview.m_from + i);
                uchar32 const rc = get_char_unsafe(rhs, i);
                if (lc != rc)
                    return false;
            }
            return true;
        }

        static string_t::range_t selectUntil(string_t const& str, uchar32 find)
        {
            for (u32 i = 0; i < str.size(); i++)
            {
                uchar32 const c = get_char_unsafe(str, i);
                if (c == find)
                {
                    return {(u32)0, i};
                }
            }
            return {0, 0};
        }

        static string_t::range_t selectUntil(const string_t& str, const string_t& find)
        {
            string_t::range_t v = {0, find.size()};
            while (!v.is_empty())
            {
                if (string_functions_t::isEqual(str, v, find))
                {
                    // So here we have a view with the size of the @find string on
                    // string @str that matches the string @find and we need to return
                    // a string view that exist before view @v.
                    return {0, v.m_from};
                }
                if (!move_view(str.m_item, v, 1))
                    break;
            }
            return {0, 0};
        }

        static string_t::range_t selectUntilLast(string_t const& str, uchar32 find)
        {
            for (u32 i = str.size() - 1; i >= 0; --i)
            {
                uchar32 const c = get_char_unsafe(str, i);
                if (c == find)
                {
                    return {str.m_item->m_range.m_from, str.m_item->m_range.m_from + i};
                }
            }
            return {0, 0};
        }

        static string_t::range_t selectUntilLast(string_t const& str, const string_t& find)
        {
            string_t::range_t v = {str.size() - find.size(), str.size()};
            while (!v.is_empty())
            {
                if (isEqual(str, v, find))
                {
                    return {0, v.m_from};
                }
                if (!move_view(str.m_item, v, -1))
                    break;
            }
            return {0, 0};
        }
    };

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    string_t string_t::select(u32 from, u32 to) const
    {
        string_t::instance_t* item = m_item->clone_full(m_item->m_arena);
        // TODO: clamp from and to to the size of the string
        item->m_range.m_from = m_item->m_range.m_from + from;
        item->m_range.m_to   = m_item->m_range.m_from + to;
        return string_t(item);
    }

    string_t string_t::selectUntil(uchar32 find) const
    {
        string_t::range_t view = string_functions_t::selectUntil(*this, find);
        if (view.is_empty())
            return string_t(get_default_instance());
        return select(0, view.m_to);
    }

    string_t string_t::selectUntil(const string_t& find) const
    {
        string_t::range_t view = string_functions_t::selectUntil(*this, find);
        if (view.is_empty())
            return string_t(get_default_instance());
        return select(view.m_from, view.m_to);
    }

    string_t string_t::selectUntilLast(uchar32 find) const
    {
        string_t::range_t view = string_functions_t::selectUntilLast(*this, find);
        if (view.is_empty())
            return string_t(get_default_instance());
        return select(view.m_from, view.m_to);
    }

    string_t string_t::selectUntilLast(const string_t& find) const
    {
        string_t::range_t view = string_functions_t::selectUntilLast(*this, find);
        if (view.is_empty())
            return string_t(get_default_instance());
        return select(view.m_from, view.m_to);
    }

    string_t string_t::selectUntilIncluded(const string_t& find) const
    {
        string_t::range_t view = string_functions_t::selectUntil(*this, find);
        if (view.is_empty())
            return string_t(get_default_instance());
        return select(view.m_from, view.m_to + find.size());
    }

    string_t string_t::selectUntilEndExcludeSelection(const string_t& selection) const
    {
        string_t::range_t range = string_functions_t::selectAfter(m_item, selection.m_item);
        return string_t(range, m_item);
    }
    string_t string_t::selectUntilEndIncludeSelection(const string_t& selection) const
    {
        string_t::range_t range = string_functions_t::selectAfterIncluded(m_item, selection.m_item);
        return string_t(range, m_item);
    }

    bool string_t::isUpper() const
    {
        for (s32 i = 0; i < size(); i++)
        {
            uchar32 const c = get_char_unsafe(m_item->m_data, m_item->m_range.m_from, m_item->m_range.m_to, i);
            if (nrunes::is_lower(c))
                return false;
        }
        return true;
    }

    bool string_t::isLower() const
    {
        for (s32 i = 0; i < size(); i++)
        {
            uchar32 const c = get_char_unsafe(m_item->m_data, m_item->m_range.m_from, m_item->m_range.m_to, i);
            if (nrunes::is_upper(c))
                return false;
        }
        return true;
    }

    static bool isCapitalized(const string_t& s)
    {
        string_t::instance_t* str = s.m_item;

        s32 i = 0;
        while (i < str->size())
        {
            uchar32 c = '\0';
            while (i < str->size())
            {
                c = get_char_unsafe(str->m_data, str->m_range.m_from, str->m_range.m_to, i);
                if (!nrunes::is_space(c))
                    break;
                i += 1;
            }

            if (nrunes::is_upper(c))
            {
                i += 1;
                while (i < str->size())
                {
                    c = get_char_unsafe(str->m_data, str->m_range.m_from, str->m_range.m_to, i);
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
        s32 const first = 0;
        return get_char_unsafe(m_item->m_data, m_item->m_range.m_from, m_item->m_range.m_to, first);
    }

    uchar32 string_t::lastChar() const
    {
        s32 const last = size() - 1;
        return get_char_unsafe(m_item->m_data, m_item->m_range.m_from, m_item->m_range.m_to, last);
    }

    bool string_t::startsWith(string_t const& start) const
    {
        string_t v = select(0, start.size());
        if (!v.is_empty())
            return (v == start);
        return false;
    }

    bool string_t::endsWith(string_t const& end) const
    {
        string_t v = select(size() - end.size(), size());
        if (!v.is_empty())
            return (v == end);
        return false;
    }

    string_t string_t::find(uchar32 find) const
    {
        for (s32 i = 0; i < size(); i++)
        {
            uchar32 const c = get_char_unsafe(m_item->m_data, m_item->m_range.m_from, m_item->m_range.m_to, i);
            if (c == find)
                return select(i, i + 1);
        }
        return string_t(get_default_instance());
    }

    string_t string_t::find(const char* inFind) const
    {
        crunes_t strfind(inFind);
        crunes_t strstr = get_crunes(m_item, m_item->m_range.m_from, m_item->m_range.m_to);

        crunes_t strfound = nrunes::find(strstr, strfind);
        if (strfound.is_empty() == false)
        {
            u32 from, to;
            get_from_to(strfound, from, to);
            return select(from, to);
        }
        return string_t(get_default_instance());
    }

    string_t string_t::find(const string_t& find) const
    {
        string_t::range_t v = {0, find.size()};
        while (!v.is_empty())
        {
            if (string_functions_t::isEqual(*this, v, find))
            {
                // So here we have a view with the size of the @find string on
                // string @str that matches the string @find.
                return select(v.m_from, v.m_to);
            }
            if (!move_view(m_item, v, 1))
                break;
        }
        return string_t(get_default_instance());
    }

    string_t string_t::findLast(const string_t& find) const
    {
        string_t::range_t v = {size() - find.size(), size()};
        while (!v.is_empty())
        {
            if (string_functions_t::isEqual(*this, v, find))
            {
                // So here we have a view with the size of the @find string on
                // string @str that matches the string @find.
                return select(v.m_from, v.m_to);
            }
            if (!move_view(m_item, v, -1))
                break;
        }
        return string_t(get_default_instance());
    }

    string_t string_t::findOneOf(const string_t& charset) const
    {
        for (s32 i = 0; i < size(); i++)
        {
            uchar32 const sc = get_char_unsafe(m_item->m_data, m_item->m_range.m_from, m_item->m_range.m_to, i);
            if (charset.contains(sc))
            {
                return select(i, i + 1);
            }
        }
        return string_t(get_default_instance());
    }

    string_t string_t::findOneOfLast(const string_t& charset) const
    {
        for (s32 i = size() - 1; i >= 0; --i)
        {
            uchar32 const sc = get_char_unsafe(m_item->m_data, m_item->m_range.m_from, m_item->m_range.m_to, i);
            if (charset.contains(sc))
            {
                return select(i, i + 1);
            }
        }
        return string_t(get_default_instance());
    }

    s32 string_t::compare(const string_t& rhs) const
    {
        if (size() < rhs.size())
            return -1;
        if (size() > rhs.size())
            return 1;

        for (s32 i = 0; i < size(); i++)
        {
            uchar32 const lc = get_char_unsafe(m_item->m_data, m_item->m_range.m_from, m_item->m_range.m_to, i);
            uchar32 const rc = get_char_unsafe(rhs.m_item->m_data, rhs.m_item->m_range.m_from, rhs.m_item->m_range.m_to, i);
            if (lc < rc)
                return -1;
            else if (lc > rc)
                return 1;
        }
        return 0;
    }

    bool string_t::isEqual(const string_t& rhs) const { return compare(rhs) == 0; }

    bool string_t::contains(const string_t& contains) const
    {
        string_t::range_t v = {0, contains.size()};
        while (!v.is_empty())
        {
            if (string_functions_t::isEqual(*this, v, contains))
            {
                return true;
            }
            if (!move_view(m_item, v, 1))
                break;
        }
        return false;
    }

    bool string_t::contains(uchar32 contains) const
    {
        for (s32 i = 0; i < size(); i++)
        {
            uchar32 const sc = get_char_unsafe(m_item->m_data, m_item->m_range.m_from, m_item->m_range.m_to, i);
            if (sc == contains)
            {
                return true;
            }
        }
        return false;
    }

    void string_t::concatenate_repeat(string_t const& con, s32 ntimes)
    {
        s32 const len = size() + (con.size() * ntimes) + 1;

        // @TODO: resize the string to the new size

        for (s32 i = 0; i < ntimes; ++i)
        {
            concatenate(con);
        }
    }

    s32 string_t::format(string_t const& format, const va_t* argv, s32 argc)
    {
        release();

        const u32 len = cprintf_(get_crunes(format.m_item, format.m_item->m_range.m_from, format.m_item->m_range.m_to), argv, argc);

        string_t::data_t*     data = alloc_data(format.m_item->m_arena, len);
        string_t::instance_t* item = alloc_instance(format.m_item->m_arena, {0, len}, data);

        runes_t str = get_runes(item, 0, 0);
        sprintf_(str, get_crunes(format.m_item, format.m_item->m_range.m_from, format.m_item->m_range.m_to), argv, argc);
        item->m_range.m_to = str.m_utf16.m_end;
        return len;
    }

    s32 string_t::formatAdd(string_t const& format, const va_t* argv, s32 argc)
    {
        const s32 len = cprintf_(get_crunes(format.m_item, format.m_item->m_range.m_from, format.m_item->m_range.m_to), argv, argc);
        resize(this->m_item, len);
        runes_t str       = get_runes(m_item->m_data, m_item->m_range.m_from, m_item->m_range.m_to);
        str.m_ascii.m_str = str.m_ascii.m_end;
        sprintf_(str, get_crunes(format.m_item, format.m_item->m_range.m_from, format.m_item->m_range.m_to), argv, argc);
        m_item->m_range.m_to = str.m_utf16.m_end;
        return len;
    }

    void string_t::insert_replace(string_t const& pos, string_t const& insert)
    {
        string_t::range_t range = pos.m_item->m_range;
        string_insert(m_item, range, insert.m_item);
    }

    void string_t::insert_before(string_t const& pos, string_t const& insert)
    {
        string_t::range_t range = string_functions_t::selectAfter(m_item, pos.m_item);
        range.m_from            = range.m_to;
        string_insert(m_item, range, insert.m_item);
    }

    void string_t::insert_after(string_t const& pos, string_t const& insert)
    {
        string_t::range_t range = string_functions_t::selectAfter(m_item, pos.m_item);
        range.m_from            = range.m_to;
        string_insert(m_item, range, insert.m_item);
    }

    void string_t::remove_selection(string_t const& selection)
    {
        // @TODO: remove the selection from the string
    }

    s32 string_t::find_remove(const string_t& _find, s32 ntimes)
    {
        string_t sel = find(_find);
        if (sel.is_empty() == false)
        {
            remove_selection(sel);
        }
        return ntimes;
    }

    void find_replace(string_t& str, const string_t& find, const string_t& replace) { find_replace(str, find, replace); }

    void remove_any(string_t& str, const string_t& any) { remove_any(str, any); }

    void replace_any(string_t& str, const string_t& any, uchar32 with)
    {
        // Replace any of the characters in @charset from @str with character @with
        for (s32 i = 0; i < str.size(); ++i)
        {
            uchar32 const c = get_char_unsafe(str.m_item->m_data, str.m_item->m_range.m_from, str.m_item->m_range.m_to, i);
            if (any.contains(c))
            {
                set_char_unsafe(str.m_item->m_data, str.m_item->m_range.m_from, str.m_item->m_range.m_to, i, with);
            }
        }
    }

    void upper(string_t& str)
    {
        for (s32 i = 0; i < str.size(); i++)
        {
            uchar32 c = get_char_unsafe(str.m_item->m_data, str.m_item->m_range.m_from, str.m_item->m_range.m_to, i);
            c         = nrunes::to_upper(c);
            set_char_unsafe(str.m_item->m_data, str.m_item->m_range.m_from, str.m_item->m_range.m_to, i, c);
        }
    }

    void lower(string_t& str)
    {
        for (s32 i = 0; i < str.size(); i++)
        {
            uchar32 c = get_char_unsafe(str.m_item->m_data, str.m_item->m_range.m_from, str.m_item->m_range.m_to, i);
            c         = nrunes::to_lower(c);
            set_char_unsafe(str.m_item->m_data, str.m_item->m_range.m_from, str.m_item->m_range.m_to, i, c);
        }
    }

    void capitalize(string_t& str)
    {
        // Standard separator is ' '
        bool prev_is_space = true;
        s32  i             = 0;
        while (i < str.size())
        {
            uchar32 c = get_char_unsafe(str.m_item->m_data, str.m_item->m_range.m_from, str.m_item->m_range.m_to, i);
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
                set_char_unsafe(str.m_item->m_data, str.m_item->m_range.m_from, str.m_item->m_range.m_to, i, c);
            }
            i++;
        }
    }

    void capitalize(string_t& str, string_t const& separators)
    {
        bool prev_is_space = false;
        s32  i             = 0;
        while (i < str.size())
        {
            uchar32 c = get_char_unsafe(str.m_item->m_data, str.m_item->m_range.m_from, str.m_item->m_range.m_to, i);
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
                set_char_unsafe(str.m_item->m_data, str.m_item->m_range.m_from, str.m_item->m_range.m_to, i, c);
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
        for (s32 i = 0; i < str.size(); ++i)
        {
            uchar32 c = get_char_unsafe(str.m_item->m_data, str.m_item->m_range.m_from, str.m_item->m_range.m_to, i);
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
            {
                if (i > 0)
                {
                    narrow_view(str.m_item, -i);
                }
                return;
            }
        }
    }

    void trimRight(string_t& str)
    {
        s32 const last = str.size() - 1;
        for (s32 i = 0; i < str.size(); ++i)
        {
            uchar32 c = get_char_unsafe(str.m_item->m_data, str.m_item->m_range.m_from, str.m_item->m_range.m_to, last - i);
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
            {
                if (i > 0)
                {
                    narrow_view(str.m_item, i);
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
        for (s32 i = 0; i < str.size(); ++i)
        {
            uchar32 c = get_char_unsafe(str.m_item->m_data, str.m_item->m_range.m_from, str.m_item->m_range.m_to, i);
            if (c != r)
            {
                if (i > 0)
                {
                    narrow_view(str.m_item, -i);
                }
                return;
            }
        }
    }

    void trimRight(string_t& str, uchar32 r)
    {
        s32 const last = str.size() - 1;
        for (s32 i = 0; i < str.size(); ++i)
        {
            uchar32 c = get_char_unsafe(str.m_item->m_data, str.m_item->m_range.m_from, str.m_item->m_range.m_to, last - i);
            if (c != r)
            {
                if (i > 0)
                {
                    narrow_view(str.m_item, i);
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
        for (s32 i = 0; i < str.size(); ++i)
        {
            uchar32 c = get_char_unsafe(str.m_item->m_data, str.m_item->m_range.m_from, str.m_item->m_range.m_to, i);
            if (!set.contains(c))
            {
                if (i > 0)
                {
                    narrow_view(str.m_item, -i);
                }
                return;
            }
        }
    }

    void trimRight(string_t& str, string_t const& set)
    {
        s32 const last = str.size() - 1;
        for (s32 i = 0; i < str.size(); ++i)
        {
            uchar32 c = get_char_unsafe(str.m_item->m_data, str.m_item->m_range.m_from, str.m_item->m_range.m_to, last - i);
            if (!set.contains(c))
            {
                if (i > 0)
                {
                    narrow_view(str.m_item, i);
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
        s32 const last = str.size() - 1;
        for (s32 i = 0; i < (last - i); ++i)
        {
            uchar32 l = get_char_unsafe(str.m_item->m_data, str.m_item->m_range.m_from, str.m_item->m_range.m_to, i);
            uchar32 r = get_char_unsafe(str.m_item->m_data, str.m_item->m_range.m_from, str.m_item->m_range.m_to, last - i);
            set_char_unsafe(str.m_item->m_data, str.m_item->m_range.m_from, str.m_item->m_range.m_to, i, r);
            set_char_unsafe(str.m_item->m_data, str.m_item->m_range.m_from, str.m_item->m_range.m_to, last - i, l);
        }
    }

    bool splitOn(string_t& str, uchar32 inChar, string_t& outLeft, string_t& outRight)
    {
        string_t::range_t range = string_functions_t::selectUntil(str, inChar);
        if (outLeft.is_empty())
            return false;
        outRight = str(outLeft.size(), str.size());
        trimRight(outLeft, inChar);
        return true;
    }

    bool splitOn(string_t& str, string_t& inStr, string_t& outLeft, string_t& outRight)
    {
        string_t::range_t range = string_functions_t::selectUntil(str, inStr);
        if (outLeft.is_empty())
            return false;
        outRight = str(outLeft.size() + inStr.size(), str.size());
        return true;
    }

    bool splitOnLast(string_t& str, uchar32 inChar, string_t& outLeft, string_t& outRight)
    {
        string_t::range_t range = string_functions_t::selectUntilLast(str, inChar);
        if (outLeft.is_empty())
            return false;
        outRight = str(outLeft.size(), str.size());
        trimRight(outLeft, inChar);
        return true;
    }

    bool splitOnLast(string_t& str, string_t& inStr, string_t& outLeft, string_t& outRight)
    {
        string_t::range_t range = string_functions_t::selectUntilLast(str, inStr);
        if (outLeft.is_empty())
            return false;
        outRight = str(outLeft.size() + inStr.size(), str.size());
        return true;
    }

    void concatenate(string_t& str, const string_t& con) { resize(str.m_item, str.size() + con.size() + 1); }

    //------------------------------------------------------------------------------
    static void user_case_for_string()
    {
        arena_t* a;

        string_t str(a, "This is an ascii string that will be converted to UTF-32");

        string_t strvw = str.slice();
        string_t ascii;
        ascii           = "ascii";
        string_t substr = strvw.find(ascii.slice());
        upper(substr);

        string_t converted(a, "converted ");
        strvw.find_remove(converted.slice());
    }

}  // namespace ncore
