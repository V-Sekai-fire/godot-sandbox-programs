cmake_minimum_required(VERSION 3.10)
project(csharp-hello)

# Find RISC-V .NET SDK
find_program(DOTNET_CMD 
	NAMES dotnet
	PATHS 
		/opt/dotnet-riscv
		$ENV{DOTNET_ROOT}
		$ENV{HOME}/.dotnet
	PATH_SUFFIXES 
		.
		dotnet
	REQUIRED
)

if(NOT DOTNET_CMD)
	message(FATAL_ERROR "RISC-V .NET SDK not found. Please install it to /opt/dotnet-riscv or set DOTNET_ROOT")
endif()

message(STATUS "Found .NET SDK: ${DOTNET_CMD}")

# Build the C# program using dotnet publish
# We need to publish as a self-contained AOT executable
set(CSHARP_PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CSHARP_OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin)
set(CSHARP_EXE_NAME "csharp-hello")

# Custom target to build C# program
# Set environment variables to work around qemu limitations
add_custom_target(csharp_hello_build
	COMMAND ${CMAKE_COMMAND} -E env
		DOTNET_SYSTEM_GLOBALIZATION_INVARIANT=true
		DOTNET_SKIP_FIRST_TIME_EXPERIENCE=true
		DOTNET_CLI_TELEMETRY_OPTOUT=true
		QEMU_LD_PREFIX=/usr/riscv64-linux-gnu
		${DOTNET_CMD} publish
		${CSHARP_PROJECT_DIR}/csharp-hello.csproj
		-r linux-riscv64
		-c Release
		--self-contained true
		/p:PublishAot=true
		/p:InvariantGlobalization=true
		/p:TrimMode=link
		/p:NativeLib=Static
		-o ${CSHARP_OUTPUT_DIR}
	WORKING_DIRECTORY ${CSHARP_PROJECT_DIR}
	COMMENT "Building C# AOT executable for RISC-V"
)

# The dotnet publish should produce a native executable
# Rename or copy to expected name if needed
add_custom_command(TARGET csharp_hello_build POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E echo "C# program built. Output: ${CSHARP_OUTPUT_DIR}/${CSHARP_EXE_NAME}"
	COMMAND ${CMAKE_COMMAND} -E chmod +x ${CSHARP_OUTPUT_DIR}/${CSHARP_EXE_NAME}
)

# Main target that produces the final .elf file
# The dotnet publish produces a native executable that is already an ELF file
add_custom_target(csharp_hello
	DEPENDS csharp_hello_build
	COMMENT "C# RISC-V Sandbox Program (Pure C#)"
)

# Set output properties similar to add_ci_program
set_target_properties(csharp_hello PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

