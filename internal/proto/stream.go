package proto

// Stream mode: a TCP connection instead of HTTP polls, made for FujiNet's
// netstream (the SIO line becomes a raw serial link to the socket). The
// server pushes the binary state as it changes, as patches to the copy the
// client already has; the client sends actions as they happen.
//
// Framing: each frame is COBS-encoded and ends in a 0 byte, so a receiver
// that lost or garbled a byte finds the next frame at the next 0. A decoded
// frame is
//
//	type, payload..., sum1, sum2
//
// where sum1/sum2 are Fletcher-16 (mod 255) over type and payload. Frames
// are at most StreamMaxFrame bytes decoded.
//
// Flow control: the client owns a small receive ring, so the server keeps at
// most StreamWindow encoded bytes (0s included) in flight. SC_CREDIT carries
// the count of bytes the client has taken out of its ring since it
// connected, mod 256: a lost credit frame is made good by the next one.

// Client -> server frame types.
const (
	SCHello  = 'H' // ver, flags (bit 0: big-endian, bit 1: a slow link), table[8], player[8] (NUL padded)
	SCAct    = 'A' // seq, act (ACT_*), arg lo, arg hi, seen
	SCTables = 'T' // list the tables (dev: payload 1)
	SCResync = 'R' // send the whole state again
	SCBye    = 'B' // leave the table
	SCPing   = 'P' // keepalive
	SCCredit = 'C' // bytes taken from the receive ring so far, mod 256
)

// Server -> client frame types.
const (
	CSPatch  = 'P' // off lo, off hi, bytes...: into the state being built
	CSCommit = 'C' // ack seq: the state being built is whole (Err/Result answer ack seq)
	CSTable  = 'T' // index, count, BinTable (count 0: no tables, no entry)
	CSError  = 'E' // ERR_*: refused (unknown table)
)

const (
	StreamVersion  = 1
	StreamMaxFrame = 128 // decoded, sum included
	StreamMaxPatch = StreamMaxFrame - 5
	StreamWindow   = 192
	StreamPort     = 9902
)

// Act ids of SC_ACT (the clients' ACT_*; the HTTP routes they stand for).
const (
	ActNone = iota
	ActState
	ActReady
	ActUnready
	ActSpecies
	ActClaim
	ActPush // arg: 0 hold, 1 up, 2 down; seen: units shown
	ActDeclare
	ActCollude
	ActBegin
	ActBuyMule
	ActOutfit
	ActInstall
	ActReturn
	ActAssay
	ActSell
	ActUnsell
	ActWampus
	ActPub
	ActEnd
	ActLeave
	ActHold // arg: the price the line was released at; seen: units shown
)

// Fletcher16 is the frame checksum: sum1 = sum of bytes, sum2 = sum of
// sum1s, both mod 255.
func Fletcher16(b []byte) (uint8, uint8) {
	var s1, s2 uint32
	for _, c := range b {
		s1 = (s1 + uint32(c)) % 255
		s2 = (s2 + s1) % 255
	}
	return uint8(s1), uint8(s2)
}

// Frame builds one encoded frame: COBS of type, payload, sum; then 0.
func Frame(typ byte, payload []byte) []byte {
	raw := make([]byte, 0, len(payload)+3)
	raw = append(raw, typ)
	raw = append(raw, payload...)
	s1, s2 := Fletcher16(raw)
	raw = append(raw, s1, s2)
	return append(COBSEncode(raw), 0)
}

// COBSEncode encodes b (no trailing 0).
func COBSEncode(b []byte) []byte {
	out := make([]byte, 1, len(b)+len(b)/254+2)
	code, ci := byte(1), 0
	for _, c := range b {
		if c != 0 {
			out = append(out, c)
			code++
		}
		if c == 0 || code == 0xFF {
			out[ci] = code
			code, ci = 1, len(out)
			out = append(out, 0)
		}
	}
	out[ci] = code
	return out
}

// COBSDecode decodes one frame (without its trailing 0). ok is false if it
// is malformed.
func COBSDecode(b []byte) (out []byte, ok bool) {
	for i := 0; i < len(b); {
		code := int(b[i])
		if code == 0 || i+code > len(b) {
			return nil, false
		}
		out = append(out, b[i+1:i+code]...)
		i += code
		if code < 0xFF && i < len(b) {
			out = append(out, 0)
		}
	}
	return out, true
}

// Unframe checks and strips a decoded frame's sum: type and payload.
func Unframe(raw []byte) (typ byte, payload []byte, ok bool) {
	if len(raw) < 3 {
		return 0, nil, false
	}
	n := len(raw) - 2
	s1, s2 := Fletcher16(raw[:n])
	if s1 != raw[n] || s2 != raw[n+1] {
		return 0, nil, false
	}
	return raw[0], raw[1:n], true
}

// Patches returns the changed runs of cur against prev (nil prev: all of
// cur), each at most StreamMaxPatch bytes. Runs closer than 4 bytes are
// joined: a patch costs 4 bytes of header.
func Patches(prev, cur []byte) (runs [][2]int) {
	n := len(cur)
	for i := 0; i < n; {
		if prev != nil && prev[i] == cur[i] {
			i++
			continue
		}
		j, last := i, i
		for j < n && j-i < StreamMaxPatch {
			if prev == nil || prev[j] != cur[j] {
				last = j
			} else if j-last >= 4 {
				break
			}
			j++
		}
		runs = append(runs, [2]int{i, last + 1})
		i = last + 1
	}
	return runs
}
