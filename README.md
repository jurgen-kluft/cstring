# string_t

Cross platform string library (BETA; still under construction)

This string internally uses UTF-16 and only the Basic Multilingual Plane (BMP) 
is supported, it is reference counted and supports modifying whereby other reference 
strings will be corrected.
Since we now can have multiple views on the string we can do away with the user
managing indices. However the string may re-allocate when an operation is causing
it to resize; this includes 'insert', 'replace' and 'remove'.
When this happens all of the active views are automatically 'corrected'. A slice
might get invalidated if the operation is 'removing' the part of the string that
the slice is watching.

Instead we can do things like this:

``` c++
string_t str("This is an ascii converted to UTF-16 when constructed");
string_t sl = str.slice();

string_t tofind = sl(11,16); // select "ascii"
// or
string_t tofind = s1.find("ascii");

string_t ascii_subslice = find(sl, tofind);
upper(ascii_subslice);    // 'ascii' to 'ASCII'

// This inserts " string" into the main string at the location indicated by @ascii_subslice
// which gets the string to resize, the view @ascii_subslice will still be correct.
string_t toinsert(" string");
ascii_subslice = insert_after(sl, ascii_subslice, toinsert.slice());

// So now @str = "This is an ASCII string converted to UTF-16 when constructed"
// And @ascii_subslice = "ASCII"
```

One thing to keep in mind is that a slice can be invalidated when the actual string is
resized due to insert, replace or remove calls, so know what you are doing. It is just
something to be aware of, invalidated views can still be used and will not cause any
crashes, they will just be empty.

You can clone a string (string data is copied):

``` c++
string_t a_copy = str.clone();
string_t a_slice = a_copy.slice();
string_t tofind2(" when constructed")
string_t to_remove = find(a_slice, tofind2.slice());
remove(a_slice, to_remove);  // This removes the substring from the main string
// So now @a_copy = "This is an ASCII string converted to UTF-16"
// @to_remove is invalidated since that part does not exist anymore.

// If you want to lower-case the first letter of 'This'
string_t to_lower = a_slice(1);
lower(to_lower);
```
