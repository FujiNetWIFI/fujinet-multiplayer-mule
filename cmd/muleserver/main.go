// Command muleserver runs the FujiNet M.U.L.E. Tournament server.
//
// Copyright (C) 2026 The fujinet-multiplayer-mule authors. GPLv3 - see LICENSE.
package main

import (
	"flag"
	"fmt"
	"log"
	"net/http"
	"os"
	"strconv"
	"time"

	"github.com/gin-gonic/gin"

	"fujinet-multiplayer-mule/internal/proto"
	"fujinet-multiplayer-mule/internal/server"
)

func main() {
	cheader := flag.Bool("cheader", false, "print the generated C header (clients/mule_proto.h) and exit")
	webDir := flag.String("web", "web", "directory of the web client, served at /play/")
	stream := flag.String("stream", envOr("MULE_STREAM_ADDR", ":"+strconv.Itoa(proto.StreamPort)),
		"address for stream mode (TCP, FujiNet netstream clients); empty: off")
	flag.Parse()

	if *cheader {
		fmt.Print(proto.CHeader())
		return
	}

	prod := os.Getenv("GO_PROD") == "1"
	if prod {
		gin.SetMode(gin.ReleaseMode)
	}

	// Lobby registration only in production. The lobby client stores the
	// chosen table's URL under this appkey id, and the clients read key 10
	// (clients/mekkogx/src/net.c AK_SERVER), so 10 is the default;
	// MULE_APPKEY=0 turns registration off.
	var lobby *server.LobbyClient
	appkey := 10
	if s, ok := os.LookupEnv("MULE_APPKEY"); ok {
		appkey, _ = strconv.Atoi(s)
	}
	if prod && appkey > 0 {
		url := os.Getenv("MULE_SERVER_URL")
		if url == "" {
			url = "http://fujinet.online:9900/" // the clients' built-in ENDPOINT
		}
		lobby = server.NewLobbyClient(server.LobbyEndpointQA, appkey, url, "us", []server.GameClient{
			// Images published by `make deploy-clients` (mule/<target>/).
			{Platform: "atari", Url: "tnfs://irata.online/mule/atari/mule.atr"},
			{Platform: "apple2", Url: "tnfs://irata.online/mule/apple2/mule.po"},
			{Platform: "coco", Url: "tnfs://irata.online/mule/coco/mule.dsk"},
			{Platform: "msdos", Url: "tnfs://irata.online/mule/msdos/mule.img"},
			{Platform: "lynx", Url: "tnfs://irata.online/mule/lynx/mule.lnx"},
			{Platform: "adam", Url: "tnfs://irata.online/mule/adam/mule.ddp"},
			{Platform: "msx", Url: "tnfs://irata.online/mule/msxrom/mule[ASCII8].rom"},
			{Platform: "nes", Url: "tnfs://irata.online/mule/nes/mule.nes"},
			{Platform: "sms", Url: "tnfs://irata.online/mule/sms/mule.sms"},
			{Platform: "h89", Url: "tnfs://irata.online/mule/h89-cpm/mule-boot.h8d"},
			{Platform: "intv", Url: "tnfs://irata.online/mule/intv/mule.rom"},
			{Platform: "astrocade", Url: "tnfs://irata.online/mule/astrocade/mule.bin"},
			{Platform: "a2600", Url: "tnfs://irata.online/mule/atari-2600/mule2600.bin"},
			{Platform: "channelf", Url: "tnfs://irata.online/mule/channelf/mule.bin"},
			{Platform: "o2", Url: "tnfs://irata.online/mule/o2/mule.bin"},
			{Platform: "arcadia", Url: "tnfs://irata.online/mule/arcadia/mule.bin"},
		})
		log.Println("updating lobby at", lobby.Endpoint)
	}

	s := server.New(lobby)
	go s.Run(50*time.Millisecond, nil)

	if *stream != "" {
		if err := s.ListenStream(*stream); err != nil {
			log.Fatal(err)
		}
	}

	r := gin.Default()
	s.Routes(r)
	r.Static("/play", *webDir)
	r.GET("/", func(c *gin.Context) { c.Redirect(http.StatusFound, "/play/") })
	r.GET("/mule_proto.h", func(c *gin.Context) { c.String(http.StatusOK, proto.CHeader()) })

	port := os.Getenv("PORT")
	if port == "" {
		port = "8080"
	}
	log.Printf("listening on port %s", port)
	if err := r.Run(":" + port); err != nil {
		log.Fatal(err)
	}
}

func envOr(k, def string) string {
	if v, ok := os.LookupEnv(k); ok {
		return v
	}
	return def
}
