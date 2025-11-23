# Architecture Decision Records (ADRs)

This directory contains RFC-style Architecture Decision Records (ADRs) documenting key architectural decisions for the GDScript to RISC-V compiler.

## RFCs

### Architecture Decisions

- [RFC 0001: Migration Strategy](./0001-migration-strategy.md) - Strategy for migrating from AST interpreter to native code templates
- [RFC 0002: Testing Environments](./0002-testing-environments.md) - Testing environments: Host vs RISC-V
- [RFC 0003: BeamAsm Template Approach](./0003-beamasm-template-approach.md) - BeamAsm-style template approach for RISC-V code generation
- [RFC 0004: Dual-Mode Architecture](./0004-dual-mode-architecture.md) - Dual-mode architecture: AST Interpreter + Native Code Templates

### Implementation Decisions (Post-Facto)

- [RFC 0005: Recursive Descent Parser](./0005-recursive-descent-parser.md) - Hand-written recursive descent parser instead of PEG
- [RFC 0006: INDENT/DEDENT Tokens](./0006-indent-dedent-tokens.md) - INDENT/DEDENT token approach for indentation handling
- [RFC 0007: Biscuit RISC-V Codegen](./0007-biscuit-riscv-codegen.md) - Biscuit library for RISC-V code generation
- [RFC 0008: ELF Generation for libriscv](./0008-elf-generation-libriscv.md) - ELF generation with libriscv compatibility
- [RFC 0009: Structured Error Handling](./0009-structured-error-handling.md) - Error collection and source location tracking
- [RFC 0010: Memory Management RAII](./0010-memory-management-raii.md) - RAII-based memory management for executable code
- [RFC 0011: Constants Centralization](./0011-constants-centralization.md) - Centralized constants for magic number elimination
- [RFC 0012: Godot C++ Naming Conventions](./0012-godot-cpp-naming-conventions.md) - Godot C++ naming conventions throughout codebase
- [RFC 0013: Function Registry](./0013-function-registry-calling-convention.md) - Function registry and calling convention
- [RFC 0014: Dynamic Buffer Growth](./0014-dynamic-buffer-growth.md) - Dynamic buffer growth strategy for code generation
- [RFC 0015: AST to RISC-V Direct Compilation](./0015-ast-to-riscv-direct-compilation.md) - Direct AST to RISC-V compilation (not Godot bytecode)

## Format

Each RFC follows this structure:

- **Status**: Accepted, Proposed, Rejected, Superseded
- **Created**: Date
- **Authors**: Development Team
- **Summary**: Brief overview
- **Motivation**: Why this decision was made
- **Detailed Design**: Implementation details
- **Alternatives Considered**: Other options evaluated
- **Unresolved Questions**: Open questions
- **References**: Links to related RFCs

## Adding New RFCs

1. Create a new file: `decisions/XXXX-topic.md` (where XXXX is the next number)
2. Follow the RFC template structure
3. Update this README with the new RFC
4. Link related RFCs in the References section

