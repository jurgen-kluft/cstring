#include "xbase/x_runes.h"

#include "xstring/x_string_writer.h"
#include "xstring/x_string_reader.h"

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

    string_writer::string_writer()
        : m_type(NONE)
    {
    }

    string_writer::string_writer(char* str, char* end)
        : m_type(ASCII)
    {
        if (str != nullptr)
        {
            m_runes._ascii = ascii::runes(str, str, end);
        }
    }

    string_writer::string_writer(utf32::rune* str, utf32::rune* end)
        : m_type(UTF32)
    {
        if (str != nullptr)
        {
            m_runes._utf32 = utf32::runes(str, str, end);
        }
    }

    string_writer::string_writer(ascii::runes const& str)
        : m_type(ASCII)
    {
        if (str.m_str != nullptr)
        {
            m_runes._ascii = str;
        }
    }

    string_writer::string_writer(utf32::runes const& str)
        : m_type(UTF32)
    {
        if (str.m_str != nullptr)
        {
            m_runes._utf32 = str;
        }
    }


    string_writer::string_writer(const string_writer& t)
        : m_type(t.m_type)
    {
        m_runes._ascii = t.m_runes._ascii;
    }

    bool string_writer::valid() const
    {
        switch (m_type)
        {
            case ASCII: return m_runes._ascii.is_valid();
            case UTF32: return m_runes._utf32.is_valid();
        }
        return false;
    }

    void string_writer::reset() 
	{
		m_runes._ascii.reset(); 
        switch (m_type)
        {
            case ASCII:
                if (m_runes._ascii.is_valid())
					*m_runes._ascii.m_end = '\0';
                break;
            case UTF32:
                if (m_runes._utf32.is_valid())
					*m_runes._utf32.m_end = '\0';
                break;
        }
	}

    bool string_writer::write(uchar32 c)
    {
        switch (m_type)
        {
            case ASCII:
                if (m_runes._ascii.is_valid())
                {
                    *m_runes._ascii.m_end++ = c;
					*m_runes._ascii.m_end = '\0';
                    return true;
                }
                break;
            case UTF32:
                if (m_runes._utf32.is_valid())
                {
                    *m_runes._utf32.m_end++ = c;
					*m_runes._utf32.m_end = '\0';
                    return true;
                }
                break;
        }
        return false;
    }

    bool string_writer::write(string_reader const& r)
    {
        string_reader reader(r);
        while (reader.valid())
        {
            uchar32 const c = reader.read();
            if (write(c) == false)
                return false;
        }
        return true;
    }

    string_writer& string_writer::operator=(const string_writer& t)
    {
        m_runes._ascii = t.m_runes._ascii;
        return *this;
    }

    bool string_writer::operator<(const string_writer& t) const { return m_runes._ascii.m_end < t.m_runes._ascii.m_end; }
    bool string_writer::operator>(const string_writer& t) const { return m_runes._ascii.m_end > t.m_runes._ascii.m_end; }
    bool string_writer::operator<=(const string_writer& t) const { return m_runes._ascii.m_end <= t.m_runes._ascii.m_end; }
    bool string_writer::operator>=(const string_writer& t) const { return m_runes._ascii.m_end >= t.m_runes._ascii.m_end; }
    bool string_writer::operator==(const string_writer& t) const
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

    bool string_writer::operator!=(const string_writer& t) const
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
