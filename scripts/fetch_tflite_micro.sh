#!/usr/bin/env bash
# Copyright 2021 The TensorFlow Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================
#
# Creates the project file distributions for the TensorFlow Lite Micro test and
# example targets aimed at embedded platforms.

set -e -x

component_hash=$1

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}/.."
TFLITE_LIB_DIR="${ROOT_DIR}/3rdparty/tflite-micro/"
mkdir -p "${TFLITE_LIB_DIR}"
cd "${TFLITE_LIB_DIR}"

if [ -f ".component_hash" ]; then
    current_hash=$(cat ".component_hash")
    if [ "${current_hash}" == "${component_hash}" ]; then
        echo "Skipping fetching tflite-micro"
        exit 0
    fi
fi

TEMP_DIR=$(mktemp -d)
cd "${TEMP_DIR}"

echo Cloning tflite-micro repo to "${TEMP_DIR}"
git clone --depth 1 "https://github.com/tensorflow/tflite-micro.git"
cd tflite-micro
git fetch --depth=1 origin "${component_hash}"
git checkout "${component_hash}"


# Create the TFLM base tree
python3 tensorflow/lite/micro/tools/project_generation/create_tflm_tree.py "${TEMP_DIR}/tflm-out"

cd "${TFLITE_LIB_DIR}"
rm -rf tensorflow
mv "${TEMP_DIR}/tflm-out/tensorflow" tensorflow

# For this repo we are forking both the models and the examples.
rm -rf tensorflow/lite/micro/models
mkdir -p third_party/
cp -r "${TEMP_DIR}"/tflm-out/third_party/* third_party/
mkdir -p signal/
cp -r "${TEMP_DIR}"/tflm-out/signal/* signal/

echo "${component_hash}" > ".component_hash"

rm -rf "${TEMP_DIR}"
