set -e -x

component_hash=$1
EIGEN_LIB_DIR=$2

if [ -z "${component_hash}" ]; then
    echo "Usage: $0 <component_hash>"
    exit 1
fi

if [ -z "${EIGEN_LIB_DIR}" ]; then     
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    ROOT_DIR="${SCRIPT_DIR}/.."
    EIGEN_LIB_DIR="${ROOT_DIR}/3rdparty/littlefs/"
fi

mkdir -p "${EIGEN_LIB_DIR}"
cd "${EIGEN_LIB_DIR}"

if [ -f ".component_hash" ]; then
    current_hash=$(cat ".component_hash")
    if [ "${current_hash}" == "${component_hash}" ]; then
        echo "Skipping fetching littlefs"
        exit 0
    fi
fi

TEMP_DIR=$(mktemp -d)
cd "${TEMP_DIR}"

echo Cloning littlefs repo to "${TEMP_DIR}"
git clone --depth 1 "https://github.com/littlefs-project/littlefs.git"
cd littlefs
git fetch --depth=1 origin "${component_hash}"
git checkout "${component_hash}"

cd "${EIGEN_LIB_DIR}"
rm -rf littlefs
mv "${TEMP_DIR}/littlefs" littlefs

echo "${component_hash}" > ".component_hash"

rm -rf "${TEMP_DIR}"
