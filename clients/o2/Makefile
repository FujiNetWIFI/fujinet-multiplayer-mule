# Makefile -- M.U.L.E. for the Odyssey 2: build/mule.bin (see build.sh).
#   make                             the image
#   make ENDPOINT=http://host:port   the server, when no lobby appkey says
ENDPOINT ?= http://fujinet.online:9900

all:
	ENDPOINT=$(patsubst %/,%,$(ENDPOINT)) ./build.sh

clean:
	rm -rf build

.PHONY: all clean
