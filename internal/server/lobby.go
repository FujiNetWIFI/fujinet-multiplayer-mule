package server

import (
	"bytes"
	"io"
	"log"
	"net/http"
	"time"

	"github.com/goccy/go-json"
)

// Lobby endpoints. [fujinet-lobby]
const (
	LobbyEndpointProd = "https://lobby.fujinet.online/server"
	LobbyEndpointQA   = "https://qalobby.fujinet.online/server"
)

const (
	PlayerTypeBot   = "bot"
	PlayerTypeHuman = "human"
)

// GameServer is the lobby registration payload.
type GameServer struct {
	Game       string       `json:"game"`
	Appkey     int          `json:"appkey"`
	Server     string       `json:"server"`
	Region     string       `json:"region"`
	Serverurl  string       `json:"serverurl"`
	Status     string       `json:"status"`
	Maxplayers int          `json:"maxplayers"`
	Curplayers int          `json:"curplayers"`
	Clients    []GameClient `json:"clients"`
	GameResult *GameResult  `json:"gameResult,omitempty"`
}

type GameClient struct {
	Platform string `json:"platform"`
	Url      string `json:"url"`
}

type GameResult struct {
	Players []GamePlayer `json:"players"`
}

type GamePlayer struct {
	Name   string `json:"name"`
	Winner bool   `json:"winner"`
	Type   string `json:"type"`
}

// LobbyClient posts table status to the FujiNet lobby.
type LobbyClient struct {
	Endpoint string
	Details  GameServer // defaults copied for every post
	client   http.Client
}

// NewLobbyClient creates a lobby client. appkey must be registered on the
// FujiNet firmware wiki ("SIO Command $DC Open App Key", lobby client app key ids).
func NewLobbyClient(endpoint string, appkey int, serverURL, region string, clients []GameClient) *LobbyClient {
	return &LobbyClient{
		Endpoint: endpoint,
		Details:  GameServer{Game: "M.U.L.E.", Appkey: appkey, Region: region, Serverurl: serverURL, Clients: clients},
		client:   http.Client{Timeout: 10 * time.Second},
	}
}

// Send posts one table's state to the lobby.
func (l *LobbyClient) Send(maxPlayers, curPlayers int, online bool, server, urlSuffix string, result *GameResult) {
	d := l.Details
	d.Maxplayers, d.Curplayers = maxPlayers, curPlayers
	d.Status = "offline"
	if online {
		d.Status = "online"
	}
	d.Server = server
	d.Serverurl += urlSuffix
	d.GameResult = result
	body, err := json.Marshal(d)
	if err != nil {
		log.Println("lobby:", err)
		return
	}
	resp, err := l.client.Post(l.Endpoint, "application/json; charset=UTF-8", bytes.NewReader(body))
	if err != nil {
		log.Println("lobby:", err)
		return
	}
	defer resp.Body.Close()
	if resp.StatusCode > 300 {
		b, _ := io.ReadAll(resp.Body)
		log.Printf("lobby %s: %s", resp.Status, b)
	}
}
