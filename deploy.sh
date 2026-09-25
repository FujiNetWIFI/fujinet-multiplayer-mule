#!/bin/sh
# Deploy to Google Cloud Run (same approach as the fujinet-game-system servers).
# State is in memory, so exactly one instance must run. Cloud Run carries
# HTTP only: stream mode (the Atari client) needs a host with a raw TCP
# port (muleserver.service).
set -e
: "${MULE_APPKEY:=0}"  # set once a lobby appkey is registered
gcloud run deploy fuji-mule \
  --source . \
  --set-env-vars "GO_PROD=1,MULE_APPKEY=${MULE_APPKEY},MULE_SERVER_URL=${MULE_SERVER_URL:-https://mule.fujinet.online/},GOOGLE_BUILDABLE=./cmd/muleserver" \
  --region=us-central1 --min-instances=0 --max-instances=1 --cpu-boost \
  --execution-environment=gen1 --memory=128Mi
