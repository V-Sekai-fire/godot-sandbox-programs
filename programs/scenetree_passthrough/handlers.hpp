#pragma once

// Include the mock handlers header which provides the JSONRPCHandler class
// and mock implementations for testing without Godot API access.
// This is used for embedded RISC-V mode where we don't have full Godot C++ API.

#include "../../scenetree_common/handlers/mock/handlers.hpp"
