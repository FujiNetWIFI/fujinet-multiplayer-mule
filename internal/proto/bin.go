package proto

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"reflect"
	"strings"

	"fujinet-multiplayer-mule/internal/rules"
)

// Binary message types.
const (
	MsgState  = 1
	MsgTables = 2
	MsgError  = 0xFF
)

// Binary layout. Every struct is packed (no padding) and uses only fixed-size
// fields so it can be read directly into a C struct on an 8-bit machine.
// The C header (clients/mule_proto.h) is generated from these types by CHeader.
// The `c` tag holds the field comment emitted into the header.

type BinHeader struct {
	Magic     uint8  `c:"'M'"`
	Version   uint8  `c:"protocol version"`
	MsgType   uint8  `c:"1 = state, 2 = tables, 0xFF = error"`
	Phase     uint8  `c:"PHASE_*"`
	Round     uint8  `c:"1..12"`
	MySeat    int8   `c:"-1 = spectator"`
	Err       uint8  `c:"ERR_* of the last action"`
	Size      uint8  `c:"reserved"`
	T         uint16 `c:"server clock, jiffies (1/60 s), low 16 bits"`
	Elapsed   uint16 `c:"jiffies since the phase started"`
	Countdown int16  `c:"lobby: jiffies until start, -1 = waiting"`
	Result    int16  `c:"action result (assay level, winnings, ...)"`
}

type BinPlayer struct {
	Name    [9]byte  `c:"NUL-terminated"`
	Species uint8    `c:"SPECIES_*"`
	Color   uint8    `c:"0..3"`
	Flags   uint8    `c:"PF_*"`
	Money   uint16   `c:""`
	Goods   [4]uint8 `c:"food, energy, smithore, crystite"`
	Score   uint16   `c:""`
	Rank    uint8    `c:"1..4"`
	Prod    [4]uint8 `c:"last production"`
	Spoil   [4]uint8 `c:"last spoilage"`
}

type BinMap struct {
	Terrain  [45]uint8 `c:"bits 0-1 TERRAIN_*, bits 2-3 mountains"`
	Owner    [45]int8  `c:"seat, -1 = none"`
	Mule     [45]int8  `c:"GOOD_*, -1 = none"`
	Crystite [45]int8  `c:"assay result known to you, -1 = unknown"`
	Prod     [45]uint8 `c:"bits 0-3 production, bit 6 for sale, bit 7 lost"`
}

type BinStore struct {
	Stock     [4]uint8  `c:""`
	Price     [4]uint16 `c:"reference price"`
	Buy       [4]uint16 `c:"store pays"`
	Sell      [4]uint16 `c:"store charges"`
	Mules     uint8     `c:""`
	MulePrice uint16    `c:""`
}

type BinGrant struct {
	Cursor  int8    `c:"plot under cursor, -1 before start, 45 = done"`
	Claimed [4]int8 `c:"plot claimed per seat, -1 none"`
	Step    uint8   `c:"jiffies per cursor step"`
	Next    uint16  `c:"jiffies until the next cursor step"`
}

type BinAuction struct {
	Kind      uint8     `c:"0 none, 1 land, 2 goods"`
	Good      int8      `c:"GOOD_*"`
	Plot      int8      `c:"land plot"`
	Seller    int8      `c:"land seller seat, -1 colony"`
	Flags     uint8     `c:"AF_*"`
	Winner    int8      `c:""`
	Colluders uint8     `c:"bitmask of seats"`
	Stage     uint8     `c:"STAGE_*"`
	Step      uint8     `c:"dollars per line step"`
	Traded    uint8     `c:"units traded"`
	Start     uint16    `c:"land starting price"`
	Ask       uint16    `c:"land asking price"`
	Lo        uint16    `c:"bottom of the price scale"`
	Hi        uint16    `c:"top of the price scale"`
	StoreBuy  uint16    `c:""`
	StoreSell uint16    `c:""`
	Price     uint16    `c:"land sale price"`
	LastTrade uint16    `c:""`
	StageLeft uint16    `c:"jiffies left in a timed stage"`
	Ticks     uint8     `c:"time bar ticks left"`
	TickLeft  uint8     `c:"jiffies left in this tick (25 pushing, 6 idle)"`
	Role      [4]uint8  `c:"ROLE_*"`
	Pos       [4]uint16 `c:"line price"`
	Target    [4]uint16 `c:""`
	Units     [4]int8   `c:"+bought / -sold"`
	Push      [4]int8   `c:"+1 up, -1 down, 0 released"`
}

type BinWampus struct {
	Plot  uint8  `c:""`
	Mount uint8  `c:"mountain index on plot"`
	From  uint16 `c:"turn BTU"`
	To    uint16 `c:""`
}

const MaxWampus = 12

type BinTurn struct {
	Seat     int8                 `c:""`
	Flags    uint8                `c:"TF_*"`
	Carrying int8                 `c:"GOOD_* outfit, -1 none"`
	Event    uint8                `c:"player event id"`
	Length   uint16               `c:"turn length, jiffies"`
	Elapsed  uint16               `c:"jiffies since the turn began (or was offered)"`
	Left     uint16               `c:"jiffies of turn time left"`
	EndIn    uint16               `c:"after the turn: jiffies until the next turn"`
	PTU      uint8                `c:"jiffies per PTU (1 pixel of the time bar)"`
	NWampus  uint8                `c:""`
	Wampus   [MaxWampus]BinWampus `c:"only sent to the active player"`
}

type BinColony struct {
	Event  uint8  `c:"CE_*"`
	Plot   int8   `c:""`
	Rating uint8  `c:"final colony rating 0..6"`
	Winner int8   `c:""`
	Failed uint8  `c:""`
	Pad    uint8  `c:""`
	Total  uint32 `c:"colony total"`
}

// BinState is the complete ?bin=1 state message.
type BinState struct {
	H       BinHeader
	Players [4]BinPlayer
	Map     BinMap
	Store   BinStore
	Grant   BinGrant
	Auction BinAuction
	Turn    BinTurn
	Colony  BinColony
}

// Flag bits.
const (
	PFAI       = 1
	PFReady    = 2
	PFDeclared = 4
	PFOnline   = 8

	AFActive    = 1
	AFDone      = 2
	AFColluding = 4
	AFIdle      = 8
	AFIn        = 16 // reserved

	TFStarted = 1
	TFEnded   = 2
	TFHasMule = 4
	TFCaught  = 8
)

func u16(v int) uint16 { return uint16(max(0, min(v, 0xFFFF))) }
func u8(v int) uint8   { return uint8(max(0, min(v, 0xFF))) }
func b2u(b bool, bit uint8) uint8 {
	if b {
		return bit
	}
	return 0
}

// ToBin converts a view to its binary layout.
func ToBin(v *View) *BinState {
	b := &BinState{}
	b.H = BinHeader{Magic: 'M', Version: Version, MsgType: MsgState, Phase: uint8(v.PhaseID),
		Round: uint8(v.Round), MySeat: int8(v.Me), Err: uint8(v.Err), T: uint16(v.T),
		Elapsed: u16(v.Elapsed), Countdown: int16(v.Count), Result: int16(v.Result)}
	for i, p := range v.Players {
		bp := &b.Players[i]
		copy(bp.Name[:8], strings.ToLower(p.Name))
		bp.Species, bp.Color = uint8(p.Species), uint8(p.Color)
		bp.Flags = b2u(p.AI, PFAI) | b2u(p.Ready, PFReady) | b2u(p.Declared, PFDeclared) | b2u(p.Online, PFOnline)
		bp.Money, bp.Score, bp.Rank = u16(p.Money), u16(p.Score), uint8(p.Rank)
		for k := 0; k < 4; k++ {
			bp.Goods[k], bp.Prod[k], bp.Spoil[k] = u8(p.Goods[k]), u8(p.Prod[k]), u8(p.Spoil[k])
		}
	}
	for i := range b.Map.Owner {
		b.Map.Owner[i], b.Map.Mule[i], b.Map.Crystite[i] = -1, -1, -1
	}
	if m := v.Map; m != nil {
		for i := range m.Terrain {
			b.Map.Terrain[i] = uint8(m.Terrain[i] | m.Mountains[i]<<2)
			b.Map.Owner[i], b.Map.Mule[i], b.Map.Crystite[i] = int8(m.Owner[i]), int8(m.Mule[i]), int8(m.Crystite[i])
			b.Map.Prod[i] = uint8(m.Prod[i]&15) | b2u(m.ForSale[i], 0x40) | b2u(m.Lost[i], 0x80)
		}
	}
	if s := v.Store; s != nil {
		for k := 0; k < 4; k++ {
			b.Store.Stock[k], b.Store.Price[k] = u8(s.Stock[k]), u16(s.Price[k])
			b.Store.Buy[k], b.Store.Sell[k] = u16(s.Buy[k]), u16(s.Sell[k])
		}
		b.Store.Mules, b.Store.MulePrice = u8(s.Mules), u16(s.MulePrice)
	}
	b.Grant.Cursor = -1
	b.Grant.Claimed = [4]int8{-1, -1, -1, -1}
	if gr := v.Grant; gr != nil {
		b.Grant.Cursor, b.Grant.Step, b.Grant.Next = int8(gr.Cursor), uint8(gr.Step), u16(gr.Next)
		for i, c := range gr.Claimed {
			b.Grant.Claimed[i] = int8(c)
		}
	}
	b.Auction.Good, b.Auction.Plot, b.Auction.Seller, b.Auction.Winner = -1, -1, -1, -1
	if a := v.Auction; a != nil {
		ba := &b.Auction
		ba.Kind = 2
		if a.Land {
			ba.Kind = 1
		}
		ba.Good, ba.Plot, ba.Seller, ba.Winner = int8(a.Good), int8(a.Plot), int8(a.Seller), int8(a.Winner)
		ba.Flags = b2u(a.Stage == 3, AFActive) | b2u(a.Done, AFDone) | b2u(a.Colluders != 0, AFColluding) | b2u(a.Idle, AFIdle)
		ba.Colluders, ba.Stage, ba.Step, ba.Traded = uint8(a.Colluders), uint8(a.Stage), u8(a.Step), u8(a.Traded)
		ba.Start, ba.Ask, ba.Lo, ba.Hi = u16(a.Start), u16(a.Ask), u16(a.Lo), u16(a.Hi)
		ba.StoreBuy, ba.StoreSell, ba.Price, ba.LastTrade = u16(a.StoreBuy), u16(a.StoreSell), u16(a.Price), u16(a.LastTrade)
		ba.StageLeft, ba.Ticks, ba.TickLeft = u16(a.StageLeft), u8(a.Ticks), u8(a.TickLeft)
		for i, l := range a.Lines {
			ba.Role[i], ba.Pos[i], ba.Target[i], ba.Units[i] = uint8(l.Role), u16(l.Pos), u16(l.Target), int8(l.Units)
			ba.Push[i] = int8(l.Push)
		}
	}
	b.Turn.Seat, b.Turn.Carrying = -1, -1
	if t := v.Turn; t != nil {
		bt := &b.Turn
		bt.Seat, bt.Carrying, bt.Event = int8(t.Seat), int8(t.Carrying), uint8(t.Event)
		bt.Flags = b2u(t.Started, TFStarted) | b2u(t.Ended, TFEnded) | b2u(t.HasMule, TFHasMule) | b2u(t.Caught, TFCaught)
		bt.Length, bt.Elapsed, bt.Left, bt.EndIn, bt.PTU = u16(t.Length), u16(t.Elapsed), u16(t.Left), u16(t.EndIn), u8(t.PTU)
		for i, w := range t.Wampus {
			if i >= MaxWampus {
				break
			}
			bt.Wampus[i] = BinWampus{u8(w.Plot), u8(w.Mount), u16(w.From), u16(w.To)}
			bt.NWampus++
		}
	}
	b.Colony.Plot, b.Colony.Winner = -1, -1
	if c := v.Colony; c != nil {
		b.Colony = BinColony{Event: uint8(c.Event), Plot: int8(c.Plot), Rating: uint8(c.Rating),
			Winner: int8(c.Winner), Failed: b2u(c.Failed, 1), Total: uint32(max(0, c.Total))}
	}
	return b
}

// BinTable is one entry of the ?bin=1 table list, same layout as 5cardstud.
type BinTable struct {
	ID      [8]byte  `c:"table id, NUL padded"`
	Name    [20]byte `c:""`
	Players [5]byte  `c:"\"n / 4\""`
}

// Encode serializes any fixed-size value in the requested byte order.
func Encode(v any, bigEndian bool) []byte {
	var buf bytes.Buffer
	var order binary.ByteOrder = binary.LittleEndian
	if bigEndian {
		order = binary.BigEndian
	}
	if err := binary.Write(&buf, order, v); err != nil {
		panic(err)
	}
	return buf.Bytes()
}

// FixedString copies s into a NUL-padded array of length n.
func FixedString(s string, n int) []byte {
	b := make([]byte, n)
	copy(b, s)
	return b
}

// ---------------------------------------------------------------- C header

// CHeader generates clients/mule_proto.h from the Go types.
func CHeader() string {
	var sb strings.Builder
	sb.WriteString("/* mule_proto.h - GENERATED by `go run ./cmd/muleserver -cheader`. Do not edit.\n")
	sb.WriteString(" * M.U.L.E. Tournament server binary protocol (?bin=1). Multi-byte values are\n")
	sb.WriteString(" * little-endian unless ?be=1 is given. Structs are packed. GPLv3.\n */\n")
	sb.WriteString("#ifndef MULE_PROTO_H\n#define MULE_PROTO_H\n\n#include <stdint.h>\n\n")
	sb.WriteString("#if defined(__GNUC__) || defined(_MSC_VER)\n#pragma pack(push, 1)\n#endif\n\n")
	fmt.Fprintf(&sb, "#define MULE_PROTO_VERSION %d\n#define MSG_STATE %d\n#define MSG_TABLES %d\n#define MSG_ERROR 0x%X\n\n", Version, MsgState, MsgTables, MsgError)
	defs := func(prefix string, names []string) {
		for i, n := range names {
			fmt.Fprintf(&sb, "#define %s%s %d\n", prefix, strings.ToUpper(n), i)
		}
		sb.WriteString("\n")
	}
	defs("PHASE_", rules.PhaseNames)
	defs("GOOD_", rules.GoodNames[:])
	defs("TERRAIN_", rules.TerrainNames)
	defs("SPECIES_", rules.SpeciesNames[:])
	defs("ROLE_", []string{"none", "buyer", "seller"})
	defs("STAGE_", []string{"show", "declare", "lead", "active", "done"})
	defs("CE_", []string{"none", "pest", "pirates", "acidrain", "quake", "sunspot", "meteorite", "radiation", "fire", "shipreturns"})
	defs("ERR_", []string{"none", "phase", "notturn", "timeup", "plot", "nomule", "havemule", "nostock", "money", "outfit", "done", "terrain", "seat", "badrequest", "tablefull"})
	sb.WriteString("#define PF_AI 1\n#define PF_READY 2\n#define PF_DECLARED 4\n#define PF_ONLINE 8\n")
	sb.WriteString("#define AF_ACTIVE 1\n#define AF_DONE 2\n#define AF_COLLUDING 4\n#define AF_IDLE 8\n")
	sb.WriteString("#define JIFFIES_PER_SECOND 60\n")
	sb.WriteString("#define TF_STARTED 1\n#define TF_ENDED 2\n#define TF_HASMULE 4\n#define TF_CAUGHT 8\n\n")
	fmt.Fprintf(&sb, "#define NUM_PLOTS %d\n#define STORE_PLOT %d\n#define MAX_WAMPUS %d\n\n", rules.NumPlots, rules.StorePlot, MaxWampus)
	sb.WriteString("/* stream mode (docs/PROTOCOL.md): frame types, limits */\n")
	for _, d := range []struct {
		n string
		v byte
	}{{"SC_HELLO", SCHello}, {"SC_ACT", SCAct}, {"SC_TABLES", SCTables}, {"SC_RESYNC", SCResync},
		{"SC_BYE", SCBye}, {"SC_PING", SCPing}, {"SC_CREDIT", SCCredit},
		{"CS_PATCH", CSPatch}, {"CS_COMMIT", CSCommit}, {"CS_TABLE", CSTable}, {"CS_ERROR", CSError}} {
		fmt.Fprintf(&sb, "#define %s '%c'\n", d.n, d.v)
	}
	fmt.Fprintf(&sb, "#define STREAM_VERSION %d\n#define STREAM_MAX_FRAME %d\n#define STREAM_WINDOW %d\n#define STREAM_PORT %d\n\n",
		StreamVersion, StreamMaxFrame, StreamWindow, StreamPort)
	for _, t := range []any{BinHeader{}, BinPlayer{}, BinMap{}, BinStore{}, BinGrant{}, BinAuction{},
		BinWampus{}, BinTurn{}, BinColony{}, BinState{}, BinTable{}} {
		writeCStruct(&sb, reflect.TypeOf(t))
	}
	sb.WriteString("#if defined(__GNUC__) || defined(_MSC_VER)\n#pragma pack(pop)\n#endif\n\n#endif\n")
	return sb.String()
}

func cType(t reflect.Type) string {
	switch t.Kind() {
	case reflect.Uint8:
		return "uint8_t"
	case reflect.Int8:
		return "int8_t"
	case reflect.Uint16:
		return "uint16_t"
	case reflect.Int16:
		return "int16_t"
	case reflect.Uint32:
		return "uint32_t"
	case reflect.Struct:
		return cName(t)
	}
	panic("unsupported type " + t.String())
}

func writeCStruct(sb *strings.Builder, t reflect.Type) {
	fmt.Fprintf(sb, "typedef struct { /* %d bytes */\n", binary.Size(reflect.New(t).Elem().Interface()))
	for i := 0; i < t.NumField(); i++ {
		f := t.Field(i)
		name := strings.ToLower(f.Name[:1]) + f.Name[1:]
		ft, arr := f.Type, ""
		if ft.Kind() == reflect.Array {
			arr = fmt.Sprintf("[%d]", ft.Len())
			ft = ft.Elem()
			if ft.Kind() == reflect.Uint8 && strings.Contains(f.Tag.Get("c"), "NUL") || f.Name == "ID" || f.Name == "Players" && t.Name() == "BinTable" || f.Name == "Name" {
				fmt.Fprintf(sb, "  char %s%s;", name, arr)
				comment(sb, f)
				continue
			}
		}
		fmt.Fprintf(sb, "  %s %s%s;", cType(ft), name, arr)
		comment(sb, f)
	}
	fmt.Fprintf(sb, "} %s;\n\n", cName(t))
}

func cName(t reflect.Type) string { return "Mule" + strings.TrimPrefix(t.Name(), "Bin") }

func comment(sb *strings.Builder, f reflect.StructField) {
	if c := f.Tag.Get("c"); c != "" {
		fmt.Fprintf(sb, " /* %s */", c)
	}
	sb.WriteString("\n")
}
