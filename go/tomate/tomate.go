package tomate

type charClassKind int
type patternItemKind int

const (
	classChar charClassKind = iota
	classAny
	classBreak
	classLetter
	classDigits
	classWhitespace
	classLowercase
	classUppercase
	classAlphanum
	classSet
)

const (
	itemSingle patternItemKind = iota
	itemMany
	itemMany1
	itemSome
	itemMaybe
)

type patternItem struct {
	class charClassKind
	item patternItemKind
	negate bool
	val rune
	start int
	end int
}

type Pattern struct {
	original string
	seq []patternItem
}

type MatchResult struct {
	start int
	end int
}

func matchSet(char rune, original string, start int, end int) bool {
    var set = []rune(original[start:end])

    for len(set) > 0 {
        var cond bool

        if set[0] == '%' {
            cond = char == set[1]
            set = set[1:]
        } else if len(set) >= 3 && set[1] == '-' {
            cond = char >= set[0] && char <= set[2]
            set = set[2:]
        } else {
            cond = char == set[0]
        }

        if cond  {
            return true
        }

        set = set[1:]
    }
    return false
}

func matchClass(item patternItem, char rune, original string) bool {
	var match bool
	switch item.class {
	case classChar:
		match = char == item.val
	case classAny:
		match = char != '\x00'
	case classBreak:
		match = char == '\n' || char == '\r'
	case classLetter:
		match = (char >= 'a' && char <= 'z') || (char >= 'A' && char <= 'Z')
	case classDigits:
		match = char >= '0' && char <= '9'
	case classWhitespace:
		match = char == ' ' || char == '\t' || char == '\n' || char == '\r'
	case classLowercase:
		match = (char >= 'a' && char <= 'z')
	case classUppercase:
		match = (char >= 'A' && char <= 'Z')
	case classAlphanum:
		match = 
			(char >= 'a' && char <= 'z') ||
			(char >= 'A' && char <= 'Z') || 
			(char >= '0' && char <= '9') || 
			char == '_'

	case classSet:
	    match = matchSet(char, original, item.start, item.end)
	}
	return item.negate != match;
}

func matchItem(seq []patternItem, seqIndex int, str []rune, original string) int {
	var iter = str

	var item = seq[seqIndex]

	switch item.item {
	case itemSingle:
		if len(iter) > 0 && matchClass(item, iter[0], original) {
			return 1
		}
		return -1
	case itemMany, itemMany1:
		size := 0

		for len(iter) > 0 && matchClass(item, iter[0], original) {
			size += 1
			iter = iter[1:]
		}

		if item.item == itemMany1 && size == 0 {
			return -1;
		}

		return size
	case itemSome:
		size := 0
		for len(iter) > 0 && matchClass(item, iter[0], original) {
			size += 1
			iter = iter[1:]

			if seqIndex + 1 < len(seq) && len(iter) > 0 && matchClass(seq[seqIndex+1], iter[0], original) {
				break
			}
		}
		return size
	case itemMaybe:
		if len(iter) > 0 && matchClass(item, iter[0], original) {
			return 1
		}
		return 0
	}
	return -1
}

func Match(pat Pattern, str string, matchStart int) (bool, MatchResult) {
	var matchFailure = MatchResult { -1, -1 }

    var strSlice []rune = []rune(str)
    var strIter = matchStart

    if matchStart >= len(str) {
    	return false, matchFailure
    }

    var patIter = 0
    var size = 0

    for ;patIter < len(pat.seq); patIter += 1 {
        var resultSize = matchItem(pat.seq, patIter, strSlice[strIter:], pat.original)

        if resultSize == -1 {
            return false, MatchResult { matchStart, size }
        }

        size += resultSize
        strIter += resultSize
    }

    return true, MatchResult { matchStart, size }
}

func parseChar(str []rune, index int) (patternItem, int) {

	if str[0] == '.' {
		return patternItem { class: classAny }, 1
	}

    if (str[0] == '[') {
    	negate := false
        
        start :=  1
        end := start
        
        if str[1] == '^' {
            negate = true
            start += 1
            end += 1
        }
        for end < len(str) {
            if str[end] == '%' {
                end += 1
            }
            if str[end] == ']' {
                break;
            }
            end += 1
        }

        return patternItem {
        	class: classSet,
        	negate: negate,
        	start: index + start,
        	end: index + end,
        }, end - index + 1
    }

    if str[0] != '%' {
        return patternItem { class: classChar, val: str[0] }, 1
    }
        
    switch str[1] {
    case 'n': return patternItem { class: classBreak }, 2
    case 'N': return patternItem { class: classBreak, negate: true }, 2
    case 'a': return patternItem { class: classLetter }, 2
    case 'A': return patternItem { class: classLetter, negate: true }, 2
    case 'd': return patternItem { class: classDigits }, 2
    case 'D': return patternItem { class: classDigits, negate: true }, 2
    case 's': return patternItem { class: classWhitespace }, 2
    case 'S': return patternItem { class: classWhitespace, negate: true }, 2
    case 'l': return patternItem { class: classLowercase }, 2
    case 'u': return patternItem { class: classUppercase }, 2
    case 'w': return patternItem { class: classAlphanum }, 2
    case 'W': return patternItem { class: classAlphanum, negate: true }, 2
    default:  return patternItem { class: classChar, val: str[1] }, 2
    }
}

func parseItem(str []rune, index int) (patternItem, int) {
    item, parseLen := parseChar(str, index)

    if parseLen >= len(str) {
    	item.item = itemSingle
    	return item, parseLen
    }

    switch str[parseLen] {
        case '*': item.item = itemMany
        case '+': item.item = itemMany1
        case '-': item.item = itemSome
        case '?': item.item = itemMaybe
        default:
            item.item = itemSingle
            return item, parseLen
    }

    return item, parseLen + 1
}

func Compile(str string) Pattern {
    var buffer []patternItem = nil
    var strIter = []rune(str)

    var index = 0

    for len(strIter) > 0 {
        item, len := parseItem(strIter, index)

        strIter = strIter[len:]
        index += len

        buffer = append(buffer, item)
    }

    return Pattern {
    	original: str,
    	seq: buffer,
    }
}
