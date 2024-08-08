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
    EIGEN_LIB_DIR="${ROOT_DIR}/3rdparty/eigen/"
fi

mkdir -p "${EIGEN_LIB_DIR}"
cd "${EIGEN_LIB_DIR}"

if [ -f ".component_hash" ]; then
    current_hash=$(cat ".component_hash")
    if [ "${current_hash}" == "${component_hash}" ]; then
        echo "Skipping fetching eigen"
        exit 0
    fi
fi

TEMP_DIR=$(mktemp -d)
cd "${TEMP_DIR}"

echo Cloning eigen repo to "${TEMP_DIR}"
git clone --depth 1 "https://gitlab.com/libeigen/eigen.git"
cd eigen
git fetch --depth=1 origin "${component_hash}"
git checkout "${component_hash}"

cd "${EIGEN_LIB_DIR}"
rm -rf eigen
mv "${TEMP_DIR}/eigen" eigen

echo "${component_hash}" > ".component_hash"

rm -rf "${TEMP_DIR}"
