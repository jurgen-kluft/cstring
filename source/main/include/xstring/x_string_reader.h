#ifndef __XTEXT_STRING_READER_H__
#define __XTEXT_STRING_READER_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

#include "xbase/x_runes.h"

namespace xcore
{
    class string_writer;

    class string_reader
    {
    protected:
        friend class string_writer;

        union crunes {
            inline crunes() : _ascii() {}
            ascii::crunes _ascii;
            utf8::crunes  _utf8;
            utf16::crunes _utf16;
            utf32::crunes _utf32;
        };
        crunes m_runes;
        s32    m_type;

    public:
        string_reader();
        string_reader(const char* str);
        string_reader(utf32::pcrune str);
        string_reader(const string_writer& chars);
        string_reader(const string_reader& chars);
        string_reader(string_reader const& from, string_reader const& until);

        bool valid() const;
        void reset();

        uchar32 read();
        uchar32 peek() const;
        void    skip();

        void select(string_reader const& from, string_reader const& until);

        bool operator<(const string_reader&) const;
        bool operator>(const string_reader&) const;
        bool operator<=(const string_reader&) const;
        bool operator>=(const string_reader&) const;
        bool operator==(const string_reader&) const;
        bool operator!=(const string_reader&) const;

        string_reader& operator=(const string_reader&);
    };
} // namespace xcore

#endif