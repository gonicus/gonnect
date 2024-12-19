#!/usr/bin/env bash

export COMMIT=$(flat-manager-client create https://flathub.intranet.gonicus.de stable)
flat-manager-client push --commit $COMMIT repo
flat-manager-client publish $COMMIT
