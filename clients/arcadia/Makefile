# Makefile -- M.U.L.E. for the Emerson Arcadia 2001: build/mule.bin (see build.sh).
#   make                             the image
#   make ENDPOINT=http://host:port   the server, when no lobby appkey says
#   make AUTOPLAY=1                  plays by itself (soak tests)
ENDPOINT ?= http://fujinet.online:9900
AUTOPLAY ?= 0

all:
	ENDPOINT=$(patsubst %/,%,$(ENDPOINT)) AUTOPLAY=$(AUTOPLAY) ./build.sh

clean:
	rm -rf build

.PHONY: all clean
