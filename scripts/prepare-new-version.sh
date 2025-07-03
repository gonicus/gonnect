#!/usr/bin/env bash

# This script changes all necessary places for a new version in this repository.

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# show_error prints an error message and exits the script.
# Arguments:
# 1 - the exit code
# 2 - the error message
function show_error {
    echo >&2 "$2"
    exit "$1"
}

# Check for needed tools
command -v sed >/dev/null 2>&1 || show_error 2 "sed command is missing!"
command -v date >/dev/null 2>&1 || show_error 2 "date command is missing!"
command -v xmlstarlet >/dev/null 2>&1 || show_error 2 "xmlstarlet command is missing!"
command -v npx >/dev/null 2>&1 || show_error 2 "npx command is missing!"

# help shows the usage information.
function help {
    cat <<HELP
Usage: prepare_new_version.sh <new version> [<commits as Base64 encoded JSON>] [options]

<new version> is a string that conforms to the rules of semantic versioning. E.g.: 1.2.3
<commits as Base64 encoded JSON> contains the commits as Base64 encoded JSON object.

Options:
    -h  --help  Show this help
HELP
}

# Parsing command line
VERSION=
COMMITS=

while [[ "$#" -gt 0 ]]; do
    case $1 in
        -h|--help)
            help
            return 0
            ;;
        *)
            if [ -z "$VERSION" ]; then
                VERSION="$1"
            elif [ -z "$COMMITS" ]; then
                COMMITS="$1"
            else
               echo "Unknown parameter \"$1\""
               help
               exit 1
            fi 
            shift
            ;;
    esac
done

if [ -z "$VERSION" ]; then
    echo "A version must be set"
    help
    exit 1
fi

###########
# Functions

# update_cmakelists sets the given version in the CMakeLists.txt
# Arguments:
# 1 - the version number to set
function update_cmakelists {
    local CMAKELISTS_FILE
    CMAKELISTS_FILE="$SCRIPT_DIR/../CMakeLists.txt"

    if [ ! -f "$CMAKELISTS_FILE" ]; then
        show_error 1 "CMakeLists.txt not found!"
    fi

    # inline update the version number using sed
    sed -i -r -e "s/^(project.*VERSION\s)\S*/\1$1/g" "$CMAKELISTS_FILE"
}

# update_antora_module sets the given version in the Antora module description docs/antora.yml
# Arguments:
# 1 - the version number to set
function update_antora_module {
    local ANTORA_FILE
    ANTORA_FILE="$SCRIPT_DIR/../docs/antora.yml"

    if [ ! -f "$ANTORA_FILE" ]; then
        show_error 1 "docs/antora.yml not found!"
    fi

    # inline update the version number using sed
    sed -i -r -e "s/^(version: )\S*/\1$1/g" "$ANTORA_FILE"
}

function create_release_notes {
    # Extract messages and remove duplicate entries
    MSG=$(base64 -d <<< "$1" | jq -rc 'map(.subject) | unique | .[]')

    # Parse messages
    local COMMITS
    COMMITS=
    local COMMIT
    while read -r line; do
        if [ -n "$COMMITS" ]; then
            COMMITS+=$'\n'
        fi

        COMMIT=$(echo "$line" | npx --packages=conventional-changelog-parser conventional-commits-parser | jq -rc '.[]')

        COMMITS+=$COMMIT
    done < <(echo "$MSG")

    NOTES=$(echo "$COMMITS" | npx --packages=conventional-changelog-writer conventional-changelog-writer -o "$SCRIPT_DIR/release-notes-options.js")
}

# update_flatpak sets the new version and notes in the Flatpak metainfo
# Arguments:
# 1 - the version number to set
# 2 - the release notes to set
function update_flatpak {
    local MANIFEST
    MANIFEST="$SCRIPT_DIR/../resources/flatpak/de.gonicus.gonnect.releases.xml"
    local CURRENT_DATE
    CURRENT_DATE=$(date +%Y-%m-%d)

    # check, if at least one release already exists
    local LAST_VERSION
    LAST_VERSION=$(xmlstarlet sel -t -v /releases/release[1]/@version "$MANIFEST")
    if [ -z "$LAST_VERSION" ]; then
        # create the first release as subnode
        xmlstarlet edit --inplace \
            --subnode "/releases" --type elem -n "NEWrelease" \
            "$MANIFEST"
    else
        # insert a new release before the existing
        xmlstarlet edit --inplace \
            --insert "/releases/release[1]" --type elem -n "NEWrelease" \
            "$MANIFEST"
    fi

    # set release informations
    xmlstarlet edit --inplace \
        --insert "//NEWrelease" --type attr -n "version" -v "$1" \
        --insert "//NEWrelease" --type attr -n "date" -v "$CURRENT_DATE" \
        "$MANIFEST"

    # add the release notes, if provided
    if [ -n "$2" ]; then
        # As xmlstarlet can't insert raw XML, do create a unique node name, which will
        # be replaced with the raw XML of the release notes by sed.
        local temp
        temp=node$(date | sha256sum | cut -d' ' -f1)
        # add description node to new release node
        xmlstarlet edit --inplace \
            --subnode "//NEWrelease" --type elem -n "description" \
            --subnode "//NEWrelease/description" --type elem -n "$temp" \
            "$MANIFEST"
        sed -i "s|<$temp/>|$2|" "$MANIFEST"
    fi

    # rename new release node to eventual name
    xmlstarlet edit --inplace \
        --rename "//NEWrelease" -v "release" \
        "$MANIFEST"

}

##############
# Main program

echo "Preparing new version \"$VERSION\""

echo "Updating CMakeLists"
update_cmakelists "$VERSION"

NOTES=
if [ -n "$COMMITS" ]; then
    echo "Creating release notes for Flatpak metainfo"
    create_release_notes "$COMMITS"
fi
echo "Updating Flatpak metainfo"
update_flatpak "$VERSION" "$NOTES"

echo "Updating Antora module"
update_antora_module "$VERSION"
