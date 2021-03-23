# Tomate 🍅

Tomate 🍅 is a very simple library for pattern matching strings in C, not as powerful as a regex engine. It's based on the patterns of the Lua programming language.

Tomate 🍅 has no alternation in patterns (`|` in regex), captures, groups and no suport for unicode. But may be useful enough for small things.

## Pattern Guide

A pattern is a sequence of pattern items, a pattern item is made by a character class and a quantifier.

A character class can be one of:

* `.` Matches any char, newlines included
* `%n` Matches line breaks
* `%N` Matches anything except line breaks
* `%a` Matches letters (a-zA-Z)
* `%A` Matches non-letters
* `%d` Matches digits (0-9)
* `%D` Matches non-digits
* `%s` Matches whitespace ( \n\t\r)
* `%S` Matches non-whitespace
* `%w` Matches alphanumeric characters (and `_`)
* `%W` Matches non-alphanumeric
* `%l` Matches lowercase letters
* `%u` Matches uppercase letters
* `%` followed by character x matches x (`%%` matches `%`, `%.` matches `.`)
* `[set]` Matches oneof the characters in the set (`[abcde]` matches any of these letters). Ranges are allowed inside a set (`[0-9a-f]` matches hexadecimal characters)
* `[^set]` Similar to normal set but matches if the character is not in the set
* a single character x matches x

A quantifier can be one of:

* `*` Matches 0 or more of the character class as much as possible
* `-` Matches 0 or more of the character class as few as possible
* `+` Matches 1 or more of the character class as much as possible
* `?` Matches 0 or 1 of the character class

Note: You cannot make groups in Tomate 🍅 like its possible with regex.

## Example

```c
#include <stdio.h>
#include "tomate.c"

int main(void) {

    // char* test_string = "1234-9003";
    // Pattern pat = compile("%d+-%d+"); // Match digits followed by a - and more digits

    char* test_string = "http://google.com";
    Pattern pat = compile("https?://.-%.%w+"); // Match http:// or https://, then many characters until a dot, then match alphanumeric characters

    MatchResult res = match(pat, test_string, sizeof(test_string), 0);

    printf("test results: start at %d with size %d\n", res.start, res.size);
    return 0;
}
```