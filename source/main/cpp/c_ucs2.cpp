#include "cbase/c_integer.h"
#include "cbase/c_memory.h"
#include "cbase/c_runes.h"

#include "cstring/c_string.h"

namespace ncore
{
    void utf8CharToUcs2Char(utf8::pcrune utf8Str, u32& utf8Cursor, u32 utf8Eos, ucs2::prune ucs2Str, u32& ucs2Cursor, u32 ucs2Eos)
    {
        if (ucs2Cursor >= ucs2Eos)
            return;

        // Initialize return values for 'return false' cases.
        ucs2Str[ucs2Cursor].r = L'?';
        s32 utf8TokenLength   = 0;

        // Decode
        if (0x80 > utf8Str[utf8Cursor].r)
        {
            utf8TokenLength       = 1;
            ucs2Str[ucs2Cursor].r = static_cast<uchar16>(utf8Str[utf8Cursor].r);
            ucs2Cursor++;
        }
        else if (0xC0 == (utf8Str[utf8Cursor].r & 0xE0))
        {
            utf8TokenLength = 1;
            if (0x80 != (utf8Str[utf8Cursor + 1].r & 0xC0))
            {
                utf8Cursor += utf8TokenLength;
                return;
            }
            utf8TokenLength       = 2;
            ucs2Str[ucs2Cursor].r = static_cast<uchar16>((utf8Str[utf8Cursor].r & 0x1F) << 6 | (utf8Str[utf8Cursor + 1].r & 0x3F));
            ucs2Cursor++;
        }
        else if (0xE0 == (utf8Str[utf8Cursor].r & 0xF0))
        {
            utf8TokenLength = 1;
            if ((0x80 != (utf8Str[utf8Cursor + 1].r & 0xC0)) || (0x80 != (utf8Str[utf8Cursor + 2].r & 0xC0)))
            {
                utf8Cursor += utf8TokenLength;
                return;
            }
            utf8TokenLength       = 3;
            ucs2Str[ucs2Cursor].r = static_cast<uchar16>((utf8Str[utf8Cursor].r & 0x0F) << 12 | (utf8Str[utf8Cursor + 1].r & 0x3F) << 6 | (utf8Str[utf8Cursor + 2].r & 0x3F));
            ucs2Cursor++;
        }
        else if (0xF0 == (utf8Str[utf8Cursor].r & 0xF8))
        {
            utf8TokenLength = 4;  // Character exceeds the UCS-2 range (UCS-4 would be necessary)
        }
        else if (0xF8 == (utf8Str[utf8Cursor].r & 0xFC))
        {
            utf8TokenLength = 5;  // Character exceeds the UCS-2 range (UCS-4 would be necessary)
        }
        else if (0xFC == (utf8Str[utf8Cursor].r & 0xFE))
        {
            utf8TokenLength = 6;  // Character exceeds the UCS-2 range (UCS-4 would be necessary)
        }
        utf8Cursor += utf8TokenLength;
    }

    void ucs2CharToUtf8Char(ucs2::pcrune ucs2Str, u32& ucs2Cursor, u32 ucs2Eos, utf8::prune utf8Str, u32& utf8Cursor, u32 utf8Eos)
    {
        u32 ucs2CharValue = ucs2Str[ucs2Cursor].r;

        // Decode
        if (0x80 > ucs2CharValue)
        {
            // Tokensize: 1 byte
            utf8Str[utf8Cursor + 1].r = static_cast<unsigned char>(ucs2CharValue);
            utf8Str[utf8Cursor + 0].r = '\0';
            utf8Cursor += 1;
        }
        else if (0x800 > ucs2CharValue)
        {
            // Tokensize: 2 bytes
            utf8Str[utf8Cursor + 2].r = '\0';
            utf8Str[utf8Cursor + 1].r = static_cast<unsigned char>(0x80 | (ucs2CharValue & 0x3F));
            ucs2CharValue             = (ucs2CharValue >> 6);
            utf8Str[utf8Cursor + 0].r = static_cast<unsigned char>(0xC0 | ucs2CharValue);
            utf8Cursor += 2;
        }
        else
        {
            // Tokensize: 3 bytes
            utf8Str[utf8Cursor + 3].r = '\0';
            utf8Str[utf8Cursor + 2].r = static_cast<unsigned char>(0x80 | (ucs2CharValue & 0x3F));
            ucs2CharValue             = (ucs2CharValue >> 6);
            utf8Str[utf8Cursor + 1].r = static_cast<unsigned char>(0x80 | (ucs2CharValue & 0x3F));
            ucs2CharValue             = (ucs2CharValue >> 6);
            utf8Str[utf8Cursor + 0].r = static_cast<unsigned char>(0xE0 | ucs2CharValue);
            utf8Cursor += 3;
        }
    }

    void ToUcs2(utf8::pcrune utf8Str, u32& utf8Cursor, u32 utf8End, ucs2::prune ucs2Str, u32& ucs2Cursor, u32 ucs2Eos)
    {
        while (ucs2Cursor < ucs2Eos && utf8Cursor < utf8End)
        {
            utf8CharToUcs2Char(utf8Str, utf8Cursor, utf8End, ucs2Str, ucs2Cursor, ucs2Eos);
        }
    }

    void ToUtf8(ucs2::pcrune ucs2Str, u32& ucs2Cursor, u32 ucs2End, utf8::prune utf8Str, u32& utf8Cursor, u32 utf8Eos)
    {
        while (ucs2Cursor < ucs2End && utf8Cursor < utf8Eos)
        {
            ucs2CharToUtf8Char(ucs2Str, ucs2Cursor, ucs2End, utf8Str, utf8Cursor, utf8Eos);
        }
    }

}  // namespace ncore
