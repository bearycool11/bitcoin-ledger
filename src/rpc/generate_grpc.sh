#!/bin/bash

# Exit on error
set -e

# Paths
PROTO_FILE="bitcoin.proto"
OUT_DIR="."
PROTOC_GEN_GRPC_PLUGIN=$(which grpc_cpp_plugin)

# Check dependencies
if ! command -v protoc &> /dev/null; then
    echo "Error: protoc (Protocol Buffers compiler) not found in PATH."
    exit 1
fi

if [ -z "$PROTOC_GEN_GRPC_PLUGIN" ]; then
    echo "Error: grpc_cpp_plugin not found. Install gRPC C++ tools."
    exit 1
fi

echo "Generating C++ gRPC and Protobuf code from ${PROTO_FILE}..."

protoc -I . \
  --cpp_out="$OUT_DIR" \
  --grpc_out="$OUT_DIR" \
  --plugin=protoc-gen-grpc="$PROTOC_GEN_GRPC_PLUGIN" \
  "$PROTO_FILE"

echo "✅ gRPC bindings generated: bitcoin.pb.cc, bitcoin.grpc.pb.cc"
