# Testing SceneTree Passthrough JSON-RPC Server

This document describes how to test the `scenetree_passthrough` JSON-RPC server.

## Quick Start

### 1. Build the Standalone Server

```bash
cd ./godot-sandbox-programs

# Build the standalone version (x86_64 for fast iteration)
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make scenetree_passthrough_standalone
```

### 2. Start the Server

```bash
# Run the server (will auto-start JSON-RPC on port 7777)
./programs/scenetree_passthrough/scenetree_passthrough_standalone
```

You should see output like:
```
JSON-RPC SceneTree Server initialized
JSON-RPC TCP server auto-started on 127.0.0.1:7777
Registered virtual API bindings from programs/scenetree_passthrough/api/virtual_methods_v4.6.1_stable.jsonl
```

### 3. Run Tests

In a separate terminal:

```bash
cd ./godot-sandbox-programs/programs/scenetree_passthrough

# Run the test suite
./run_tests.sh
```

## Test Components

### 1. RapidCheck Property Tests (`test_jsonrpc_compliance.cpp`)

Tests JSON-RPC 2.0 specification compliance:

- **JSON-RPC version property**: All responses must have `"jsonrpc": "2.0"`
- **Error code range property**: Error codes must be in `[-32768, -32000]`
- **Response ID match property**: Response must contain the same ID as request
- **Valid method names property**: Method names must be valid identifiers

**Run standalone:**
```bash
./test_jsonrpc_compliance
```

### 2. Integration Tests (`test_scenetree_rpc.cpp`)

Tests actual server connectivity and method calls:

- **get_root**: Get the root node
- **get_frame**: Get current frame number
- **get_current_scene**: Get current scene
- **Invalid method**: Should return proper JSON-RPC error

**Run standalone:**
```bash
./test_scenetree_rpc
```

## Manual Testing with curl

You can also test manually using curl:

```bash
# Test get_root
curl -X POST http://127.0.0.1:7777 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"get_root","params":[],"id":1}'

# Test get_frame
curl -X POST http://127.0.0.1:7777 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"get_frame","params":[],"id":2}'

# Test get_current_scene
curl -X POST http://127.0.0.1:7777 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"get_current_scene","params":[],"id":3}'

# Test invalid method (should return error)
curl -X POST http://127.0.0.1:7777 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"nonexistent","params":[],"id":4}'
```

## Expected Responses

### Success Response
```json
{
  "jsonrpc": "2.0",
  "result": <result_data>,
  "id": 1
}
```

### Error Response
```json
{
  "jsonrpc": "2.0",
  "error": {
    "code": -32601,
    "message": "Method not found"
  },
  "id": 4
}
```

## Available Methods

The server exposes all `SceneTree` methods from Godot 4.6.1:

- `get_root()` - Get the root node
- `get_frame()` - Get current frame number
- `get_current_scene()` - Get current scene
- `get_node_count()` - Get node count
- `call()` - Call a method on a node
- `get()` - Get a property
- `set()` - Set a property
- `is_valid()` - Check if node is valid
- `free()` - Free a node
- `queue_delete()` - Queue node for deletion
- ... and many more (see `virtual_methods_v4.6.1_stable.jsonl`)

## Troubleshooting

### Server won't start
- Check if port 7777 is already in use: `lsof -i :7777`
- Kill existing process: `kill $(lsof -t -i :7777)`

### Connection refused
- Ensure server is running
- Check firewall settings
- Verify server is listening: `netstat -tuln | grep 7777`

### Invalid JSON-RPC response
- Check server logs for errors
- Verify request format matches JSON-RPC 2.0 spec
- Ensure method name is valid

## Next Steps

After basic tests pass:

1. **Load testing**: Test with concurrent connections
2. **Stress testing**: Test with rapid method calls
3. **Integration testing**: Test with actual Godot scenes
4. **MCP protocol layer**: Add MCP Streamable HTTP on top of JSON-RPC
