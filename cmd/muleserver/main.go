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
	flag.Parse()

	if *cheader {
		fmt.Print(proto.CHeader())
		return
	}

	prod := os.Getenv("GO_PROD") == "1"
	if prod {
		gin.SetMode(gin.ReleaseMode)
	}

	// Lobby registration only in production, and only once an appkey is assigned.
	var lobby *server.LobbyClient
	appkey, _ := strconv.Atoi(os.Getenv("MULE_APPKEY"))
	if prod && appkey > 0 {
		url := os.Getenv("MULE_SERVER_URL")
		if url == "" {
			url = "https://mule.fujinet.online/"
		}
		lobby = server.NewLobbyClient(server.LobbyEndpointQA, appkey, url, "us", []server.GameClient{
			{Platform: "atari", Url: "tnfs://tnfs.fujinet.online/atari/mule.xex"},
		})
		log.Println("updating lobby at", lobby.Endpoint)
	}

	s := server.New(lobby)
	go s.Run(50*time.Millisecond, nil)

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
