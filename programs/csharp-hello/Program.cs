using System;
using System.Runtime.InteropServices;

namespace CSharpHello
{
	unsafe class Program
	{
		// Property storage (matching C++ example)
		private static int meaning_of_life = 42;

		// API Functions that will be registered with the sandbox
		// These use UnmanagedCallersOnly for AOT compatibility and to match C++ function signatures

		[UnmanagedCallersOnly]
		public static Variant HelloWorld()
		{
			// Return a string - in C++ this returns "Hello, world!" as a Variant
			// For now, we return a simple variant representation
			// In a full implementation, we'd need proper Variant string marshalling
			SandboxAPI.Print("Hello, world!");
			return Variant.Nil;
		}

		[UnmanagedCallersOnly]
		public static Variant PrintString(IntPtr strPtr)
		{
			// C++ version: static Variant print_string(String str)
			// String is a Godot type, passed as pointer in interop
			if (strPtr != IntPtr.Zero)
			{
				string str = Marshal.PtrToStringAnsi(strPtr);
				SandboxAPI.Print($"String: {str}\n");
			}
			return Variant.Nil;
		}

		[UnmanagedCallersOnly]
		public static Variant Fibonacci(int n)
		{
			// C++ version: static Variant fibonacci(int n)
			// Returns the nth Fibonacci number
			long result = Fib(n, 0, 1);
			// Return as Variant - in full implementation would marshal properly
			return new Variant { type = (long)VariantType.INT, value = result };
		}

		// Helper function matching C++ implementation
		private static long Fib(long n, long acc, long prev)
		{
			if (n == 0)
				return acc;
			else
				return Fib(n - 1, prev + acc, acc);
		}

		[UnmanagedCallersOnly]
		public static Variant TestMemalign()
		{
			// C++ version demonstrates memory alignment
			// In C# we can use Marshal.AllocHGlobal with alignment
			// For simplicity, just demonstrate allocation
			IntPtr ptr1 = Marshal.AllocHGlobal(32);
			IntPtr ptr2 = Marshal.AllocHGlobal(32);
			
			SandboxAPI.Print($"Test: {ptr1}, {ptr2}\n");
			
			Marshal.FreeHGlobal(ptr1);
			Marshal.FreeHGlobal(ptr2);
			
			return Variant.Nil;
		}

		// Property getter (matching C++ lambda)
		[UnmanagedCallersOnly]
		public static Variant GetMeaningOfLife()
		{
			return new Variant { type = (long)VariantType.INT, value = meaning_of_life };
		}

		// Property setter (matching C++ lambda)
		[UnmanagedCallersOnly]
		public static Variant SetMeaningOfLife(IntPtr valuePtr)
		{
			// In C++: [](Variant value) -> Variant { meaning_of_life = value; print("Set to: ", meaning_of_life); return Nil; }
			if (valuePtr != IntPtr.Zero)
			{
				Variant value = Marshal.PtrToStructure<Variant>(valuePtr);
				meaning_of_life = (int)value.value;
				SandboxAPI.Print($"Set to: {meaning_of_life}\n");
			}
			return Variant.Nil;
		}

		static void Main()
		{
			// Match C++ main() function structure
			SandboxAPI.Print("Hello, world!");

			// Get Sandbox node and check binary translation
			IntPtr sandboxPtr = SandboxAPI.GetSandboxNode();
			if (sandboxPtr != IntPtr.Zero)
			{
				bool isTranslated = SandboxAPI.IsBinaryTranslated(sandboxPtr);
				SandboxAPI.Print(isTranslated
					? "The current program is accelerated by binary translation.\n"
					: "The current program is running in interpreter mode.\n");
			}

			// Register API functions (matching ADD_API_FUNCTION calls)
			// In Native AOT, we get function pointers using LDSTR_TOKEN or direct method addresses
			// Using unsafe function pointers for UnmanagedCallersOnly methods
			unsafe
			{
				delegate* unmanaged<Variant> helloWorldFn = &HelloWorld;
				SandboxAPI.AddAPIFunction((IntPtr)helloWorldFn, "String", "", "Returns the string 'Hello, world!'");

				delegate* unmanaged<IntPtr, Variant> printStringFn = &PrintString;
				SandboxAPI.AddAPIFunction((IntPtr)printStringFn, "void", "String str", "Prints a string to the console");

				delegate* unmanaged<int, Variant> fibonacciFn = &Fibonacci;
				SandboxAPI.AddAPIFunction((IntPtr)fibonacciFn, "long", "int n", "Calculates the nth Fibonacci number");

				delegate* unmanaged<Variant> testMemalignFn = &TestMemalign;
				SandboxAPI.AddAPIFunction((IntPtr)testMemalignFn, "void", "", "Tests memory alignment");

				// Add property (matching add_property call)
				delegate* unmanaged<Variant> getterFn = &GetMeaningOfLife;
				delegate* unmanaged<IntPtr, Variant> setterFn = &SetMeaningOfLife;
				SandboxAPI.AddProperty("meaning_of_life", VariantType.INT, 42, (IntPtr)getterFn, (IntPtr)setterFn);
			}

			// Halt execution (matching C++ halt() call)
			SandboxAPI.Halt();
		}
	}
}

