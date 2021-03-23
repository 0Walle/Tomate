#include <stdbool.h>
#include <stdlib.h>

/*

Bad copy of Lua pattern matching for strings

Pattern: # ^ $ not implemented
  ^? PatternItem+ $?

PatternItem:
  Class
  Class *
  Class +
  Class -
  Class ?

Class:
  Char
  Any
  LineBreak
  Alphabet
  Digits
  Whitespace
  Lowercase 
  Uppercase
  Alphanum
  Set
*/

#define CLASS_CHAR       0b00000000
#define CLASS_ANY        0b00000001
#define CLASS_BREAK      0b00000010
#define CLASS_LETTER     0b00000011
#define CLASS_DIGITS     0b00000100
#define CLASS_WHITESPACE 0b00000101
#define CLASS_LOWERCASE  0b00000110
#define CLASS_UPPERCASE  0b00000111
#define CLASS_ALPHANUM   0b00001000
#define CLASS_SET        0b00001001
#define CLASS_BITMASK    0b00001111

#define ITEM_SINGLE  0b00000000
#define ITEM_MANY    0b00010000
#define ITEM_MANY1   0b00100000
#define ITEM_SOME    0b00110000
#define ITEM_MAYBE   0b01000000
#define ITEM_BITMASK 0b01110000

#define NEG_BITMASK  0b10000000

struct Item {
    char kind;
    short val;
    short end;
};

struct Pattern {
    bool start;
    bool end;
    char *original;
    int size;
    struct Item *seq;
};
typedef struct Pattern Pattern;

struct MatchResult {
    int start;
    int size;
};
typedef struct MatchResult MatchResult;

MatchResult MATCH_FAILURE = {-1, -1};

bool match_set(char* original, char chr, int start, int end, bool comp) {
    char *iter = &original[start];
    char *iter_end = &original[end];

    for (;iter != iter_end; ++iter) {
        bool cond;

        if (*iter == '%') {
            cond = chr == iter[1];
            iter += 1;
        } else if (iter + 2 < iter_end && iter[1] == '-') {
            cond = chr >= iter[0] && chr <= iter[2];
            iter += 2;
        } else {
            cond = chr == iter[0];
        }

        if (cond) {
            return true;
        }
    }
    return false;
}

bool match_class(struct Item item, char chr, char* original) {
    bool match;
    switch (item.kind & CLASS_BITMASK) {
        case CLASS_CHAR:
            match = chr == item.val;
            break;
        case CLASS_ANY:
            match = chr != '\0';
            break;
        case CLASS_BREAK:
            match = chr == '\n' || chr == '\r';
            break;
        case CLASS_LETTER:
            match = (chr >= 'a' && chr <= 'z') 
                 || (chr >= 'A' && chr <= 'Z');
            break;
        case CLASS_DIGITS:
            match = chr >= '0' && chr <= '9';
            break;
        case CLASS_WHITESPACE:
            match = chr == ' ' || chr == '\t' || chr == '\n' || chr == '\r';
            break;
        case CLASS_LOWERCASE:
            match = (chr >= 'a' && chr <= 'z');
            break;
        case CLASS_UPPERCASE:
            match = (chr >= 'A' && chr <= 'Z');
            break;
        case CLASS_ALPHANUM:
            match = (chr >= 'a' && chr <= 'z') 
                 || (chr >= 'A' && chr <= 'Z')
                 || (chr >= '0' && chr <= '9')
                 || chr == '_';
            break;
        case CLASS_SET:
            match = match_set(original, chr, item.val, item.end,
                (item.kind & NEG_BITMASK) != 0);
            break;
    }
    return ((item.kind & NEG_BITMASK) != 0) != match;
}

int match_item(
    struct Item *seq, int seq_size, int seq_index, 
    char *str, int str_size, int str_index,
    char *original
) {
    int size;
    int last_size;
    char *iter = str;
    struct Item item = seq[seq_index];
    switch (item.kind & ITEM_BITMASK) {
        case ITEM_SINGLE:
            if (match_class(item, *iter, original)) {
                return 1;
            }
            return -1;
        case ITEM_MANY:
        case ITEM_MANY1:
            size = 0;
            last_size = size;
            while (match_class(item, *iter, original)) {
                ++size;
                ++iter;

                if (seq_index + 1 >= seq_size || 
                    match_class(seq[seq_index+1], *iter, original))
                {
                    last_size = size;
                }
            }

            if (item.kind == ITEM_MANY1 && size == 0) {
                return -1;
            }
            return last_size;
        case ITEM_SOME:
            size = 0;
            while (match_class(item, *iter, original)) {
                ++size;
                ++iter;

                if (seq_index + 1 < seq_size && 
                    match_class(seq[seq_index+1], *iter, original))
                {
                    break;
                }
            }
            return size;
        case ITEM_MAYBE:
            if (match_class(item, *iter, original)) {
                return 1;
            }
            return 0;
    }
    return -1;
}

MatchResult match(Pattern pat, char *str, int str_size, int match_start) {
    if (match_start >= str_size) return MATCH_FAILURE;

    char *ptr = &str[match_start];
    if (*ptr == '\0') return MATCH_FAILURE;

    int pat_index = 0;

    int size = 0;

    while (pat_index < pat.size) {
        int res_size = match_item(
            pat.seq, pat.size, pat_index,
            ptr, str_size-match_start, match_start,
            pat.original
        );

        if (res_size == -1) {
            return MATCH_FAILURE;
        }

        size += res_size;
        ptr += res_size;

        pat_index += 1;
    }

    return (MatchResult){ match_start, size };
}

struct Item pchar(char *str, int *len, int index) {
    #define X (struct Item)

    if (*str == '.') {
        if (len) *len = 1;
        return X {CLASS_ANY};
    }

    if (*str == '[') {
        bool negate = false;
        int start = index + 1;
        int end = start;
        if (str[1] == '^') {
            negate = true;
            start += 1;
            end += 1;
        }
        while (str[end] != '\0') {
            if (str[end] == '%') {
                end += 1;
            }
            if (str[end] == ']') {
                break;
            }
            end += 1;
        }
        if (len) *len = end - index + 1;
        return X { CLASS_SET | (negate ? NEG_BITMASK : 0), start, end};
    }

    if (*str != '%') {
        if (len) *len = 1;
        return X {.val = *str};
    }

    if (len) *len = 2;
        
    switch (str[1]) {
        case 'n': return X { CLASS_BREAK };
        case 'N': return X { CLASS_BREAK | NEG_BITMASK };
        case 'a': return X { CLASS_LETTER };
        case 'A': return X { CLASS_LETTER | NEG_BITMASK };
        case 'd': return X { CLASS_DIGITS };
        case 'D': return X { CLASS_DIGITS | NEG_BITMASK };
        case 's': return X { CLASS_WHITESPACE };
        case 'S': return X { CLASS_WHITESPACE | NEG_BITMASK };
        case 'l': return X { CLASS_LOWERCASE };
        case 'u': return X { CLASS_UPPERCASE };
        case 'w': return X { CLASS_ALPHANUM };
        case 'W': return X { CLASS_ALPHANUM | NEG_BITMASK };
        default:
            return X {.val = str[1]};
    }

    #undef X
}

struct Item pitem(char *str, int *len, int index) {
    int len_;
    struct Item c = pchar(str, &len_, index);

    if (len) *len = len_ + 1;

    switch (str[len_]) {
        case '*': c.kind |= ITEM_MANY; break;
        case '+': c.kind |= ITEM_MANY1; break;
        case '-': c.kind |= ITEM_SOME; break;
        case '?': c.kind |= ITEM_MAYBE; break;
        default:
            if (len) *len = *len - 1;
            c.kind |= ITEM_SINGLE;
            break;
    }

    return c;
}

Pattern compile(char *str) {
    struct Item *buffer = NULL;
    int buffer_size = 0;
    int buffer_capacity = 8;

    char *original = str;

    int len_;
    int index = 0;
    buffer = realloc(buffer, buffer_capacity * sizeof(struct Item));

    while (*str != '\0') {
        struct Item item = pitem(str, &len_, index);
        str += len_;
        index += len_;

        buffer[buffer_size] = item;

        if (buffer_size + 1 == buffer_capacity) {
            buffer_capacity += 8;
            buffer = realloc(buffer, buffer_capacity * sizeof(struct Item));
        }

        ++buffer_size;
    }

    Pattern pat = {
        false, false, original, buffer_size, buffer
    };

    return pat;
}