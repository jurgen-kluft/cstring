#ifndef __XTEXT_STRING_WRITER_H__
#define __XTEXT_STRING_WRITER_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

#include "xbase/x_runes.h"

namespace xcore
{
    class string_reader;

    class string_writer
    {
    protected:
        friend class string_reader;
        union runes {
            inline runes() : _ascii() {}
            ascii::runes _ascii;
            utf8::runes  _utf8;
            utf16::runes _utf16;
            utf32::runes _utf32;
        };
        runes m_runes;
        s32   m_type;

    public:
        string_writer();
        string_writer(char* str, char* end);
        string_writer(utf32::rune* str, utf32::rune* end);
        string_writer(ascii::runes const&);
        string_writer(utf32::runes const&);
        string_writer(const string_writer& chars);

        bool valid() const;
        void reset();

        bool write(uchar32 c);
        bool write(string_reader const&);

        bool operator<(const string_writer&) const;
        bool operator>(const string_writer&) const;
        bool operator<=(const string_writer&) const;
        bool operator>=(const string_writer&) const;
        bool operator==(const string_writer&) const;
        bool operator!=(const string_writer&) const;

        string_writer& operator=(const string_writer&);
    };
} // namespace xcore

#endif