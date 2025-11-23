#pragma once

namespace gdscript {

// Compilation/execution mode
enum class CompilerMode {
    INTERPRET,      // AST interpreter mode (slower, simpler, good for debugging)
    NATIVE_CODE     // Native code templates mode (faster, like BeamAsm)
};

} // namespace gdscript

