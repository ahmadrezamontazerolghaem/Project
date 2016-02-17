package sip

import(
  "io"
  "strings"
  "net/textproto"
)

type Header map[string][]string

// writes a new SIP header
func (h Header) Add(key, value string) {
	textproto.MIMEHeader(h).Add(key, value)
}

// modifies an existing SIP header
func (h Header) Set(key, value string) {
	textproto.MIMEHeader(h).Set(key, value)
}

// retrieves an exsiting SIP header
func (h Header) Get(key string) string {
	return textproto.MIMEHeader(h).Get(key)
}

// removes an existing SIP header
func (h Header) Del(key string) {
	textproto.MIMEHeader(h).Del(key)
}

// copies an existing SIP header
func (h Header) clone() Header {
	h2 := make(Header, len(h))
	for k, vv := range h {
		vv2 := make([]string, len(vv))
		copy(vv2, vv)
		h2[k] = vv2
	}
	return h2
}

var headerNewlineToSpace = strings.NewReplacer("\n", " ", "\r", " ")

type writeStringer interface {
	WriteString(string) (int, error)
}

// stringWriter implements WriteString on a Writer.
type stringWriter struct {
	w io.Writer
}

func (w stringWriter) WriteString(s string) (n int, err error) {
	return w.w.Write([]byte(s))
}

type keyValues struct {
	key    string
	values []string
}
