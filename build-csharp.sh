#!/bin/bash
# Build C# program to RISC-V without dev container
# Uses Docker to run the RISC-V .NET SDK

set -e

echo "Building C# program to RISC-V..."
echo ""

# Check if SDK is downloaded
if [ ! -d ".dotnet-riscv" ] || [ ! -f ".dotnet-riscv/dotnet" ]; then
	echo "⚠️  RISC-V .NET SDK not found. Downloading..."
	./download-dotnet-riscv.sh
fi

# Create output directory
mkdir -p build/bin/csharp-hello

# Build using Docker
echo "Building with Docker..."
./run-dotnet-amd64.sh publish \
	programs/csharp-hello/csharp-hello.csproj \
	-r linux-riscv64 \
	-c Release \
	--self-contained true \
	/p:PublishAot=true \
	/p:InvariantGlobalization=true \
	/p:TrimMode=link \
	/p:NativeLib=Static \
	-o build/bin/csharp-hello \
	2>&1 | tee build/build.log

if [ ${PIPESTATUS[0]} -eq 0 ]; then
	echo ""
	echo "✅ Build successful!"
	echo "Output: build/bin/csharp-hello/csharp-hello"
	
	# Check if file exists and show info
	if [ -f "build/bin/csharp-hello/csharp-hello" ]; then
		echo ""
		echo "File info:"
		file build/bin/csharp-hello/csharp-hello || echo "  (file command not available)"
		echo ""
		echo "Size:"
		ls -lh build/bin/csharp-hello/csharp-hello
	else
		echo "⚠️  Warning: Expected output file not found"
		echo "Check build/build.log for details"
	fi
else
	echo ""
	echo "❌ Build failed. Check build/build.log for details."
	echo ""
	echo "Common issues:"
	echo "  - QEMU mutex errors: Known limitation, see BUILD-NOTES.md"
	echo "  - SDK not found: Run ./download-dotnet-riscv.sh"
	echo "  - Docker not running: Start Docker Desktop"
	exit 1
fi

