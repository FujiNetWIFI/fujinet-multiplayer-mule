.PHONY: build test run assets header
build:
	go build -o muleserver ./cmd/muleserver
test:
	go test ./...
run:
	go run ./cmd/muleserver
header:
	go run ./cmd/muleserver -cheader > clients/mule_proto.h
assets:
	@test -n "$(ATR)" || (echo 'usage: make assets ATR="path/to/M.U.L.E. (1983)(Electronic Arts)(US).atr"'; exit 1)
	python3 tools/extract/extract.py "$(ATR)" web/assets
