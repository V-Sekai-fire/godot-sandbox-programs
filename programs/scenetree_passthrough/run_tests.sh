#!/bin/bash
# Test script for scenetree_passthrough JSON-RPC server

set -e

echo "=== SceneTree Passthrough Test Suite ==="
echo ""

# Check if we should build the test client
if [ ! -f "./test_scenetree_rpc" ]; then
    echo "Building test client..."
    mkdir -p build_tests
    cd build_tests
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make test_scenetree_rpc
    cd ..
fi

# Check if server is running
echo "Checking if JSON-RPC server is running on port 7777..."
if ! netstat -tuln 2>/dev/null | grep -q ":7777 " && ! ss -tuln 2>/dev/null | grep -q ":7777 "; then
    echo "Server not running. Please start the scenetree_passthrough program first."
    echo "Example: ./scenetree_passthrough_standalone"
    exit 1
fi

echo "Server detected on port 7777"
echo ""

# Run RapidCheck tests if available
if [ -f "./test_jsonrpc_compliance" ]; then
    echo "=== Running RapidCheck Property Tests ==="
    ./test_jsonrpc_compliance
    echo ""
fi

# Run integration tests
echo "=== Running Integration Tests ==="
echo "Press Enter to start testing the server..."
read -r

./test_scenetree_rpc

echo ""
echo "=== All Tests Complete ==="
