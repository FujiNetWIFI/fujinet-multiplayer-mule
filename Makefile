# Nothing here needs root, and sudo breaks it: HOME becomes /root (the intv
# build can't find IntyBASIC), outputs end up root-owned, scp uses root's key.
ifeq ($(shell id -u),0)
$(error Don't run make as root: HOME becomes /root (intv can't find IntyBASIC), outputs end up root-owned, and scp uses root's SSH key)
endif

.PHONY: build test run assets header clients dist deploy-clients
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

# ---- clients: build every platform and publish to apps.irata.online --------
#   make clients           build them all
#   make dist              ... and gather the images in dist/<target>/
#   make deploy-clients    ... and scp dist/<target>/ to $(DEPLOY_HOST):$(DEPLOY_DIR)/<target>/
# ENDPOINT is passed to every build so a local test server can't leak in.
ENDPOINT    ?= http://fujinet.online:9900/
DEPLOY_HOST ?= thomc@irata.online
DEPLOY_DIR  ?= apps.irata.online/mule
DIST        = dist
MEKKOGX_PLATFORMS = apple2 coco msdos adam msxrom atari lynx nes sms atari7800 atari5200 h89-cpm

clients:
	$(MAKE) -C clients/mekkogx ENDPOINT=$(ENDPOINT) $(MEKKOGX_PLATFORMS)
	$(MAKE) -C clients/intv ENDPOINT=$(ENDPOINT)
	$(MAKE) -C clients/astrocade -B ENDPOINT=$(ENDPOINT)
	$(MAKE) -C clients/atari-2600 -B ENDPOINT=$(ENDPOINT)
	$(MAKE) -C clients/channelf -B ENDPOINT=$(ENDPOINT)
	$(MAKE) -C clients/o2 -B ENDPOINT=$(ENDPOINT)
	$(MAKE) -C clients/arcadia -B ENDPOINT=$(ENDPOINT)
	$(MAKE) -C clients/studio2 -B ENDPOINT=$(ENDPOINT)

dist: clients
	rm -rf $(DIST)
	mkdir -p $(addprefix $(DIST)/,$(MEKKOGX_PLATFORMS) intv astrocade atari-2600 channelf o2 arcadia studio2)
	cp clients/mekkogx/r2r/apple2/mule.po $(DIST)/apple2/
	cp clients/mekkogx/r2r/coco/mule.dsk $(DIST)/coco/
	cp clients/mekkogx/r2r/msdos/mule.img $(DIST)/msdos/
	cp clients/mekkogx/r2r/adam/mule.ddp clients/mekkogx/r2r/adam/mule.dsk $(DIST)/adam/
	cp clients/mekkogx/r2r/msxrom/mule.rom 'clients/mekkogx/r2r/msxrom/mule[ASCII8].rom' $(DIST)/msxrom/
	cp clients/mekkogx/r2r/atari/mule.atr $(DIST)/atari/
	cp clients/mekkogx/r2r/lynx/mule.lnx $(DIST)/lynx/
	cp clients/mekkogx/r2r/nes/mule.nes $(DIST)/nes/
	cp clients/mekkogx/r2r/sms/mule.sms $(DIST)/sms/
	cp clients/mekkogx/r2r/atari7800/mule.a78 $(DIST)/atari7800/
	cp clients/mekkogx/r2r/atari5200/mule.a52 $(DIST)/atari5200/
	cp clients/mekkogx/r2r/h89-cpm/mule.h8d clients/mekkogx/r2r/h89-cpm/mule.com $(DIST)/h89-cpm/
	$(MAKE) -C clients/mekkogx h89-boot
	cp clients/mekkogx/r2r/h89-cpm/mule-boot.h8d $(DIST)/h89-cpm/
	cp clients/intv/mule.rom clients/intv/mule.bin clients/intv/mule.cfg $(DIST)/intv/
	cp clients/astrocade/build/mule.bin $(DIST)/astrocade/
	cp clients/atari-2600/build/mule2600.bin $(DIST)/atari-2600/
	cp clients/channelf/build/mule.bin $(DIST)/channelf/
	cp clients/o2/build/mule.bin $(DIST)/o2/
	cp clients/arcadia/build/mule.bin $(DIST)/arcadia/
	cp clients/studio2/build/mule.st2 $(DIST)/studio2/

deploy-clients: dist
	ssh $(DEPLOY_HOST) 'mkdir -p $(DEPLOY_DIR)'
	scp -r $(DIST)/* $(DEPLOY_HOST):$(DEPLOY_DIR)/
