package proto

import (
	stdjson "encoding/json"
	"fmt"
	"strconv"
	"strings"
)

// MsgSame is a header-only binary reply for ?hash= when nothing changed.
const MsgSame = 3

// Flatten turns a JSON document into "path<delim>value<delim>" pairs, in
// document order, with paths like "pl/0/n" (the same form FujiNet's JSON
// query uses). Booleans become 1/0 and null becomes an empty value.
func Flatten(js, delim string, upper, lower bool) string {
	dec := stdjson.NewDecoder(strings.NewReader(js))
	dec.UseNumber()
	var sb strings.Builder
	type frame struct {
		arr  bool
		idx  int
		key  string
		path string
	}
	var stack []frame
	path := func() string {
		if len(stack) == 0 {
			return ""
		}
		f := stack[len(stack)-1]
		k := f.key
		if f.arr {
			k = strconv.Itoa(f.idx)
		}
		if f.path == "" {
			return k
		}
		return f.path + "/" + k
	}
	emit := func(val string) {
		sb.WriteString(path())
		sb.WriteString(delim)
		sb.WriteString(val)
		sb.WriteString(delim)
	}
	next := func() {
		if len(stack) > 0 && stack[len(stack)-1].arr {
			stack[len(stack)-1].idx++
		}
	}
	expectKey := false
	for {
		tok, err := dec.Token()
		if err != nil {
			break
		}
		switch t := tok.(type) {
		case stdjson.Delim:
			switch t {
			case '{', '[':
				p := path()
				stack = append(stack, frame{arr: t == '[', path: p})
				expectKey = t == '{'
				continue
			case '}', ']':
				stack = stack[:len(stack)-1]
				next()
			}
		case string:
			if expectKey {
				stack[len(stack)-1].key = t
				expectKey = false
				continue
			}
			emit(t)
			next()
		case stdjson.Number:
			emit(t.String())
			next()
		case bool:
			if t {
				emit("1")
			} else {
				emit("0")
			}
			next()
		case nil:
			emit("")
			next()
		default:
			emit(fmt.Sprint(t))
			next()
		}
		if len(stack) > 0 && !stack[len(stack)-1].arr {
			expectKey = true
		}
	}
	out := sb.String()
	if upper {
		out = strings.ToUpper(out)
	}
	if lower {
		out = strings.ToLower(out)
	}
	return out
}
