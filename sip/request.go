package sip

import (
  "bufio"
  "io"
  "fmt"
)

// SIP request parsing errors.
type ProtocolError struct {
	ErrorString string 
}

func (err *ProtocolError) Error() string { return err.ErrorString }

var (
   RecipientMissing = &ProtocolError{"recipient cannot be empty"}
   InitiatorMissing = &ProtocolError{"initiator cannot be empty"}
   ToBranchMissing  = &ProtocolError{"branch required for 'To:' header"}
   FromTagMissing   = &ProtocolError{"tag required for 'From:' header"}
   CallIDMissing    = &ProtocolError{"callid cannot be empty"}
)

type Request struct {
  // INVITE sip:user2@server2.com SIP/2.0
  Method string // eg. INVITE
  Recipient string // eg. user2@server2.com
  Initiator string // eg. user1@server1.com
  // Via: SIP/2.0/UDP pc.server.com;branch=1234567890asdf Max-Forwards: 70
  Branch string
  Transport string // eg. UDP
  Tag int64
  // Call-ID: zxcvb12345@pc.server1.com
  CallID string
  MaxForwards string
  Header Header
}

// Return value if nonempty, def otherwise
func valueOrDefault(value, def string) string {
  if value != "" {
    return value
  }
  return def
}

func (r *Request) Write(w io.Writer) error {
  return r.write(w, nil)
}

func (req *Request) write(w io.Writer, extraHeaders Header) error {
  var bw *bufio.Writer

  if _, ok := w.(io.ByteWriter); !ok {
    bw = bufio.NewWriter(w)
    w = bw
  }

  rversion := "2.0"

  fmt.Fprintf(w, "%s sip:%s SIP/%s", valueOrDefault(req.Method, "INVITE"), req.Recipient, rversion)

  fmt.Fprintf(w, "Via: SIP/%s/%s %s;branch=%s Max-Forwards: %d", rversion, valueOrDefault(req.Transport, "UDP"), valueOrDefault(req.MaxForwards, "70"))

  return nil
}


func NewRequest(method, rec string, ini string, branch string, tag int64, callid string, trans string) (*Request, error) {

  // ensure the critical fields are passed, otherwise return error
  if rec == "" {
    return nil, RecipientMissing
  }

  if ini == "" {
    return nil, InitiatorMissing
  }

  if branch == "" {
    return nil, ToBranchMissing
  }

  if callid == "" {
    return nil, CallIDMissing
  }

  req := &Request{
          Method:     method,
          Recipient:  rec,
          Initiator:  ini,
          Branch:     branch,
          Tag:        tag,
          Transport:  trans,
          CallID:     callid,
          Header:     make(Header),
  }

  return req, nil
}
