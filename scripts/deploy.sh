#!/usr/bin/env bash

COMMIT=$(flat-manager-client create https://flathub.intranet.gonicus.de stable)
export COMMIT
flat-manager-client push --commit "$COMMIT" repo
flat-manager-client publish "$COMMIT"
