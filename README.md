# xstring

Cross platform string library (still under construction)

This string has a UTF-32 encoding, it uses a reference-counted slice so that we have the
ability to create multiple views of the string. Since we now can have multiple views using the same slice we can do away with the user managing indices.

Instead we can do things like this:
``` c++
xstring str("This is an ascii converted to UTF-32 when constructed");

xstring ascii_substring = find(str, "ascii");
upper(ascii_substring);    // 'ascii' to 'ASCII'

// This actually inserts " string" into the main string
ascii_substring = ascii_substring + " string";

// So now @str = "This is an ASCII string converted to UTF-32 when constructed"
```

One thing to keep in mind is that 'views' can shift on the string when substrings are
inserted or removed, so know what you are doing.

You can clone a string (slice is copied):
``` c++
xstring a_cloned_copy = clone(str);
xstring to_remove = find(a_cloned_copy, " when constructed");
remove(to_remove);  // This remove the substring from the main string
// So now @a_cloned_copy = "This is an ASCII string converted to UTF-32"

// If you want to lower-case the first letter of 'This'
xstring to_lower = a_cloned_copy(1);
lower(to_lower);
```
