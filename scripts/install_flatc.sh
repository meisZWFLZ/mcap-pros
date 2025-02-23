#!/bin/env bash

# Installs flatc if not already installed

set -e

FLATC_CMD="flatc"

# Where to put the flatbuffers repo
FB_REPO_DIR="$(mktemp -d)"
# URL of the flatbuffers repo
FB_REPO_URL="https://github.com/google/flatbuffers.git"
# Tag of the flatbuffers repo to checkout
FB_TAG="v25.2.10"

if type $FLATC_CMD &> /dev/null; then
  echo "flatc is installed!"
  exit 0
fi

echo "flatc is not installed. Installing..."

echo "Downloading flatbuffers repo..."
git clone "$FB_REPO_URL" "$FB_REPO_DIR" --depth=1 --branch="$FB_TAG"

# Ensures that the tmp dir is deleted
cleanup() {
  echo "Cleaning up.."
  rm -Rf $FB_REPO_DIR
}

# If prompted to exit early, make sure to cleanup
trap "cleanup; exit 3" SIGINT

# Change to the flatbuffers repo directory
cd "$FB_REPO_DIR"


echo "Building flatc..."

# Configure environment
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
# Build flatc
make -j$(($(nproc) - 1)) flatc

target_dir="$HOME/.local/bin"

# Move flatc to the bin directory
mkdir -p "$target_dir"
mv -t "$target_dir" flatc 

echo "flatc installed!"

# Cleanup
cleanup