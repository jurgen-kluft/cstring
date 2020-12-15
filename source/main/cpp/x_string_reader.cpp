#include "xbase/x_runes.h"

#include "xstring/x_string_reader.h"
#include "xstring/x_string_writer.h"

namespace xcore
{
    enum
    {
        NONE,
        ASCII,
        UTF8,
        UTF16,
        UTF32
    };



    string_reader::string_reader()
        : m_type(NONE)
    {
    }

    string_reader::string_reader(const char* str)
        : m_type(ASCII)
    {
        if (str != nullptr)
        {
            m_runes._ascii = ascii::crunes_t(str);
        }
    }

    string_reader::string_reader(utf32::pcrune str)
        : m_type(UTF32)
    {
        if (str != nullptr)
        {
            m_runes._utf32 = utf32::crunes_t(str);
        }
    }

    string_reader::string_reader(const string_writer& t)
        : m_type(t.m_type)
    {
        m_runes._ascii = t.m_runes._ascii;
    }

    string_reader::string_reader(const string_reader& t)
        : m_type(t.m_type)
    {
        m_runes._ascii = t.m_runes._ascii;
    }

    string_reader::string_reader(string_reader const& from, string_reader const& until)
        : m_type(from.m_type)
    {
        m_runes._ascii.m_str = from.m_runes._ascii.m_cur;
        m_runes._ascii.m_cur = m_runes._ascii.m_str;
        m_runes._ascii.m_end = until.m_runes._ascii.m_cur;
    }

    bool string_reader::valid() const
    {
        switch (m_type)
        {
            case ASCII: return m_runes._ascii.is_valid();
            case UTF32: return m_runes._utf32.is_valid();
        }
        return false;
    }

    void string_reader::reset() { m_runes._ascii.reset(); }

    uchar32 string_reader::read()
    {
        switch (m_type)
        {
            case ASCII:
                if (m_runes._ascii.is_valid())
                    return *m_runes._ascii.m_cur++;
            case UTF32:
                if (m_runes._utf32.is_valid())
                    return *m_runes._utf32.m_cur++;
        }
        return '\0';
    }

    uchar32 string_reader::peek() const
    {
        switch (m_type)
        {
            case ASCII:
                if (m_runes._ascii.is_valid())
                    return *m_runes._ascii.m_cur;
            case UTF32:
                if (m_runes._utf32.is_valid())
                    return *m_runes._utf32.m_cur;
        }
        return '\0';
    }

    void string_reader::skip()
    {
        switch (m_type)
        {
            case ASCII:
                if (m_runes._ascii.is_valid())
                    m_runes._ascii.m_cur++;
                break;
            case UTF32:
                if (m_runes._utf32.is_valid())
                    m_runes._utf32.m_cur++;
                break;
        }
    }

    void string_reader::select(string_reader const& from, string_reader const& until)
    {
        m_type = from.m_type;
        m_runes._ascii.m_str = from.m_runes._ascii.m_cur;
        m_runes._ascii.m_cur = m_runes._ascii.m_str;
        m_runes._ascii.m_end = until.m_runes._ascii.m_cur;
    }

    string_reader& string_reader::operator=(const string_reader& t)
    {
        m_runes._ascii = t.m_runes._ascii;
        return *this;
    }

    bool string_reader::operator<(const string_reader& t) const { return m_runes._ascii.m_cur < t.m_runes._ascii.m_cur; }
    bool string_reader::operator>(const string_reader& t) const { return m_runes._ascii.m_cur > t.m_runes._ascii.m_cur; }
    bool string_reader::operator<=(const string_reader& t) const { return m_runes._ascii.m_cur <= t.m_runes._ascii.m_cur; }
    bool string_reader::operator>=(const string_reader& t) const { return m_runes._ascii.m_cur >= t.m_runes._ascii.m_cur; }
    bool string_reader::operator==(const string_reader& t) const
    {
        if (t.m_type == m_type)
        {
            if (t.m_runes._ascii.m_str == m_runes._ascii.m_str && t.m_runes._ascii.m_end == m_runes._ascii.m_end)
                return true;

            // Character by character comparison
            switch (m_type)
            {
                case ASCII: return ascii::compare(m_runes._ascii, t.m_runes._ascii) == 0;
                case UTF32: return utf32::compare(m_runes._utf32, t.m_runes._utf32) == 0;
            }
        }
        return false;
    }

    bool string_reader::operator!=(const string_reader& t) const
    {
        if (t.m_type == m_type)
        {
            if (t.m_runes._ascii.m_str == m_runes._ascii.m_str && t.m_runes._ascii.m_end == m_runes._ascii.m_end)
                return false;

            // Character by character comparison
            switch (m_type)
            {
                case ASCII: return ascii::compare(m_runes._ascii, t.m_runes._ascii) != 0;
                case UTF32: return utf32::compare(m_runes._utf32, t.m_runes._utf32) != 0;
            }
        }
        return true;
    }


} // namespace xcore
