# Deprecated Code

This directory contains deprecated code that is no longer used in the main compilation path.

## Files

- `ast_to_ir.h/cpp` - MLIR-based AST to IR conversion (deprecated, replaced by direct AST to RISC-V)
- `ast_to_riscv.h/cpp` - Old RISC-V assembly text emitter (deprecated, replaced by `ast_to_riscv_biscuit.h/cpp` which generates machine code)
- `mlir/` - MLIR integration code (deprecated, no longer used)

## Current Implementation

The current implementation uses:
- `ast_to_riscv_biscuit.h/cpp` - Direct AST to RISC-V machine code using biscuit (following BEAM JIT pattern)

These deprecated files are kept for reference and may be used by some old tests, but are not part of the active codebase.

