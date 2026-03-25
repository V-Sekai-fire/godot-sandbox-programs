# SceneTreeMCP Implementation Plan for Godot Engine (C++)

## 1\. Overview

This module will implement `SceneTreeMCP`, a native Model Context Protocol (MCP) server for Godot. It allows AI assistants to connect to a running Godot instance via Streamable HTTP, enabling tool calls, engine-state reads (Resources), and prompt/template access.

Following Godot C++ conventions, the server will be a `SceneTree`-derived runtime service (`SceneTreeMCP`) that uses native `MainLoop` lifecycle callbacks for protocol startup, per-frame processing, and teardown.

### 1.1 SceneTreeMCP Context

- `SceneTreeMCP` is a custom `SceneTree` implementation (not a scene-bound node).
- MCP protocol lifecycle is tied directly to the `SceneTreeMCP` lifecycle.
- It is the single authority for MCP session state (initialization state, registered tools/resources, and active endpoint peers).
- GDScript and engine systems interact with MCP through `SceneTreeMCP`, avoiding direct networking logic in gameplay scripts.

### 1.2 Non-Goals

- No scene-bound `Node` implementation for MCP transport.
- No per-scene MCP instances or per-scene protocol state.
- No legacy split endpoint transport (`/sse` + `/message`).
- No direct socket-handling code in gameplay scripts.

## 2\. Specification Context

To ensure full compatibility with standard MCP clients, this implementation adheres to the MCP Streamable HTTP transport:

### 2.1 Model Context Protocol (MCP) Specification

- **Transport Mechanism:** This implementation uses the MCP **Streamable HTTP** transport with a single MCP endpoint.
- **Wire Format:** All payloads must be valid JSON-RPC 2.0 objects.
- **Request/Response Model:** The single endpoint accepts JSON-RPC requests and returns JSON-RPC responses over HTTP; when needed, responses may be streamed over the same transport.
- **Initialization Lifecycle:** The server MUST NOT process standard tool/resource requests until the client has sent an `initialize` request and followed up with an `initialized` notification.

## 3\. Core Architecture

The server handles a single HTTP endpoint (e.g., `/mcp`) for all MCP traffic.

1.  **Single MCP Endpoint (`/mcp`)**: Receives JSON-RPC requests from the client and returns JSON-RPC responses using Streamable HTTP.

### 3.1 Reused Design from `programs/scenetree_passthrough`

`SceneTreeMCP` should follow the same structural split already proven in the passthrough system:

1.  **MainLoop Layer (SceneTreeMCP-specific)**
    - Equivalent role to `programs/scenetree_passthrough/main.cpp`.
    - Owns transport/session concerns (HTTP parsing, `/mcp` routing, MCP initialization state).
    - Converts MCP JSON-RPC payloads into internal dispatch calls and serializes replies during native lifecycle callbacks.

2.  **Shared Common Layer (reuse from `programs/scenetree_common`)**
    - Reuse request validation, JSON-RPC error helpers, and dispatch scaffolding patterns.
    - Keep common utilities transport-agnostic, with transport-specific logic only in the MainLoop layer.
    - Preserve the same “single dispatch function + helper functions” style used by passthrough.

3.  **Handler Implementation Layer (real only)**
    - Use only real handlers that execute Godot SceneTree/Object operations.
    - Do not include handler backend switching or alternate handler modes in this implementation.

4.  **Adapter Difference (only what changes)**
    - Passthrough entrypoint currently serves raw JSON-RPC over TCP.
    - `SceneTreeMCP` MainLoop layer replaces that transport adapter with MCP Streamable HTTP while reusing dispatch/handler architecture.

### Required Godot Classes

- `TCPServer`: To listen for incoming connections on a specified port.
- `StreamPeerTCP`: To read/write raw bytes to connected clients.
- `JSON`: To parse incoming request bodies and format outgoing JSON-RPC payloads.
- `SceneTree`: Base class for `SceneTreeMCP` and host for native `MainLoop` lifecycle callbacks.

## 4\. Class Design (`SceneTreeMCP`)

    class SceneTreeMCP : public SceneTree {
        GDCLASS(SceneTreeMCP, SceneTree);
        // ...

### 4.1 Connection Management

- **TCPServer Processing**: The server will maintain a `Ref<TCPServer>`. During native `SceneTree` processing callbacks, it will check `tcp_server->is_connection_available()`.
- **Peer Tracking**: Mirror passthrough's client-lifecycle pattern (`accept -> poll -> read -> write -> prune`) for peers connected to `/mcp`.

### 4.1.1 Ownership Rules (Refs vs Nodes)

- Use `Ref<T>` only for `RefCounted` engine types (e.g., `TCPServer`, `StreamPeerTCP`).
- `TCPServer` inheritance: `SocketServer < RefCounted < Object` (use `Ref<TCPServer>`).
- `StreamPeerTCP` inheritance: `StreamPeerSocket < StreamPeer < RefCounted < Object` (use `Ref<StreamPeerTCP>`).
- Keep `SceneTreeMCP` as a `SceneTree` object type (MainLoop object), not a `Ref`.
- Keep scene graph entities as `Node`-typed objects; do not model nodes as `Ref` wrappers.

### 4.2 Native `MainLoop` Callback Flow

Driven by native `SceneTree`/`MainLoop` callbacks:

1.  `_initialize()`: Start transport state and begin listening on the configured TCP endpoint.
2.  `_process(double delta)`: Execute per-frame network work (`accept -> poll -> read -> route -> write -> prune`).
3.  `_finalize()`: Stop listening, close peers, and release MCP runtime state.

Use internal helpers to keep lifecycle logic explicit:

- `_start_transport(port)`: Called from `_initialize()`.
- `_stop_transport()`: Called from `_finalize()`.

Within `_process(double delta)`, the flow will:

1.  Accept new TCP connections.
2.  Read HTTP headers and ensure the request targets `/mcp`.
3.  Read the request body (if present), parse JSON-RPC payload(s), and validate protocol state.
4.  Write JSON-RPC responses on the same endpoint, using streamed HTTP responses when appropriate.

## 5\. Feature Implementation

### 5.1 Streamable HTTP Responses

When Godot needs to send data (response or notification), it formats a `Dictionary` into a JSON `String` and writes it through the `/mcp` HTTP response channel using standard Streamable HTTP semantics.

### 5.2 Handling JSON-RPC (Client -> Server)

When a request arrives at `/mcp`:

1.  Parse the body using `godot::JSON`.
2.  Extract the `method` string (e.g., `initialize`, `tools/call`, `resources/read`).
3.  Reuse passthrough-style JSON-RPC validation behavior (single/batch request handling, notification detection, spec-compliant error envelopes).
4.  Verify the MCP initialization state (rejecting non-initialize calls until initialized).
5.  Route the request through the same dispatch abstraction style used in passthrough.

### 5.3 Exposing Godot Logic (The API)

To make the server useful, GDScript must be able to register tools and resources. We will expose methods via `_bind_methods()`:

- **Registering Tools**: `SceneTreeMCP.register_tool(name: String, description: String, schema: Dictionary, callable: Callable)` _When the AI calls a tool, the C++ server invokes the stored `Callable`, gets the result (Variant), and formats it into a JSON-RPC response sent back via Streamable HTTP._
- **Registering Resources**: `SceneTreeMCP.register_resource(uri: String, name: String, callable: Callable)`

## 6\. Step-by-Step Execution Plan

1.  **Phase 1: MainLoop Skeleton (SceneTreeMCP)**
    - Create `scenetree_mcp.h` and `scenetree_mcp.cpp` as the MainLoop transport/session layer.
    - Implement `_start_transport(port)` and `_stop_transport()`.
    - Implement native callbacks `_initialize()`, `_process(double delta)`, and `_finalize()`.
    - Implement basic HTTP request parsing (extracting Method, Path, and Body).

2.  **Phase 2: Reuse Common Dispatch Patterns**
    - Port/reuse passthrough dispatch helpers (request validation, JSON-RPC error construction, notification semantics).
    - Keep dispatch helpers transport-neutral where possible.

3.  **Phase 3: Streamable HTTP Endpoint & Initialization**
    - Handle requests on `/mcp` only.
    - Implement endpoint validation and streamable response behavior.
    - Handle the mandatory `initialize` JSON-RPC method to negotiate capabilities.

4.  **Phase 4: Message Routing**
    - Handle JSON-RPC messages on `/mcp`.
    - Implement Godot `JSON` parsing for the request body.
    - Map standard MCP protocol methods (`ping`, `tools/list`, `resources/list`).

5.  **Phase 5: GDScript Interop**
    - Implement tool/resource registries using `HashMap<String, Callable>`.
    - Bind `register_tool` and `register_resource` to ClassDB.
    - Implement Callable execution (`callable.call(args)`) and route the return Variant back through Streamable HTTP responses.

## 7\. Godot C++ Constraints Checklist

- \[x\] Use `godot::String` for all text manipulation.
- \[x\] Use `Ref<StreamPeerTCP>` and `Ref<TCPServer>` for memory safety.
- \[x\] Keep ownership model strict: refs are refs (`RefCounted` types), nodes/main loop types remain object/node types.
- \[x\] No `try/catch` blocks; use Godot's `Error` enums for return values.
- \[x\] Use `Callable` for executing arbitrary game logic triggered by the AI.
- \[x\] Register `SceneTreeMCP` as a `SceneTree`-derived engine type and wire startup to instantiate it as the active main loop where applicable.
