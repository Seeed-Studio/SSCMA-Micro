#!/usr/bin/env bash

set -e -x

component_hash=$1

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}/.."
CJSON_LIB_DIR="${ROOT_DIR}/3rdparty/json/"
mkdir -p "${CJSON_LIB_DIR}"
cd "${CJSON_LIB_DIR}"

if [ -f ".component_hash" ]; then
    current_hash=$(cat ".component_hash")
    if [ "${current_hash}" == "${component_hash}" ]; then
        echo "Skipping fetching cJSON"
        exit 0
    fi
fi

TEMP_DIR=$(mktemp -d)
cd "${TEMP_DIR}"

echo Cloning cJSON repo to "${TEMP_DIR}"
git clone --depth 1 "https://github.com/DaveGamble/cJSON"
cd cJSON
git fetch --depth=1 origin "${component_hash}"
git checkout "${component_hash}"

cd "${CJSON_LIB_DIR}"
rm -rf cJSON
mv "${TEMP_DIR}/cJSON" cJSON

echo "${component_hash}" > ".component_hash"

rm -rf "${TEMP_DIR}"
