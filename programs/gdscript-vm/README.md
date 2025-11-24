# GDScript VM - Godot Bytecode VM Port

This program ports Godot's GDScript bytecode VM to godot-sandbox, allowing execution of GDScript code using Godot's proven VM implementation.

## Overview

This is a full port of Godot's GDScript bytecode system:
- **Bytecode Generator**: Converts AST to bytecode instructions
- **VM Execution**: Executes bytecode using Godot's VM
- **gdextension API**: Uses godot-sandbox's API for Variant, String, etc.

## Architecture

```
GDScript Source Code
    ↓
[Parser] → AST (our parser)
    ↓
[AST to Bytecode Adapter] → Godot Bytecode
    ↓
[GDScript VM] → Execution (via gdextension API)
```

## Status

**In Progress**: Files copied from Godot, adaptation to godot-sandbox API in progress.

## Files

- `gdscript_byte_codegen.h/cpp` - Bytecode generator from Godot
- `gdscript_vm.cpp` - VM execution engine from Godot
- `gdscript_function.h/cpp` - Function representation from Godot
- `gdscript_utility_functions.h/cpp` - Utility functions from Godot
- `gdscript.h/cpp` - GDScript class from Godot
- `godot_core_stubs.h` - Stubs to replace Godot core dependencies
- `ast_to_bytecode_adapter.h` - Adapter to convert our AST to bytecode

## Dependencies

Uses godot-sandbox's gdextension API (`<api.hpp>`) for:
- `Variant` - Value type
- `String` - String type
- `StringName` - String name type (aliased to String)
- Other Godot types through gdextension

## Next Steps

1. Complete adaptation of Godot files to use gdextension API
2. Create AST to bytecode adapter
3. Integrate with our parser
4. Test execution

