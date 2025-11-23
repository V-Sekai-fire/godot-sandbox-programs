using System;
using System.Runtime.InteropServices;
using System.Text;

namespace CSharpHello
{
	// Variant type representation (matches Godot's Variant structure)
	[StructLayout(LayoutKind.Sequential)]
	public struct Variant
	{
		public long type;
		public long value;
		
		public static Variant Nil => new Variant { type = 0, value = 0 };
	}

	// Variant type constants (matching Godot's Variant::Type enum)
	public enum VariantType
	{
		NIL = 0,
		BOOL = 1,
		INT = 2,
		FLOAT = 3,
		STRING = 4,
		// Add more as needed
	}

	// Sandbox API interop - these functions are provided by the sandbox runtime
	public static unsafe class SandboxAPI
	{
		// Print function - variadic, but we'll use a simple string version
		// The actual print() in C++ is variadic, but for P/Invoke we use a simpler signature
		[DllImport("__Internal", EntryPoint = "print", CallingConvention = CallingConvention.Cdecl)]
		private static extern void print_internal(byte* str);

		// Halt function
		[DllImport("__Internal", EntryPoint = "halt", CallingConvention = CallingConvention.Cdecl)]
		public static extern void Halt();

		// Get Sandbox node - returns a pointer to the Sandbox object
		// Note: This is a template function in C++, so we need the mangled name or a C wrapper
		// For now, we'll try the likely mangled name
		[DllImport("__Internal", EntryPoint = "_Z8get_nodeI7SandboxET_v", CallingConvention = CallingConvention.Cdecl)]
		private static extern IntPtr get_node_sandbox();

		// Alternative: If there's a C-compatible version
		[DllImport("__Internal", EntryPoint = "get_sandbox_node", CallingConvention = CallingConvention.Cdecl)]
		private static extern IntPtr get_sandbox_node_c();

		// Register API function
		// The ADD_API_FUNCTION macro likely calls an underlying registration function
		// Function signature: void add_api_function(void* func_ptr, const char* return_type, const char* params, const char* description)
		[DllImport("__Internal", EntryPoint = "add_api_function", CallingConvention = CallingConvention.Cdecl)]
		private static extern void add_api_function_internal(
			IntPtr funcPtr,
			byte* returnType,
			byte* parameters,
			byte* description
		);

		// Add property
		// void add_property(const char* name, int type, long default_value, void* getter, void* setter)
		[DllImport("__Internal", EntryPoint = "add_property", CallingConvention = CallingConvention.Cdecl)]
		private static extern void add_property_internal(
			byte* name,
			int type,
			long defaultValue,
			IntPtr getter,
			IntPtr setter
		);

		// Helper to convert C# string to UTF-8 null-terminated byte array
		private static byte* StringToUtf8(string str)
		{
			if (string.IsNullOrEmpty(str))
				return null;

			byte[] bytes = Encoding.UTF8.GetBytes(str);
			byte* ptr = (byte*)Marshal.AllocHGlobal(bytes.Length + 1);
			Marshal.Copy(bytes, 0, (IntPtr)ptr, bytes.Length);
			ptr[bytes.Length] = 0; // null terminator
			return ptr;
		}

		// Helper to free allocated memory
		private static void FreeUtf8(byte* ptr)
		{
			if (ptr != null)
				Marshal.FreeHGlobal((IntPtr)ptr);
		}

		// Public API methods with proper string handling
		public static void Print(string message)
		{
			if (string.IsNullOrEmpty(message))
				return;

			byte* utf8 = StringToUtf8(message);
			try
			{
				print_internal(utf8);
			}
			finally
			{
				FreeUtf8(utf8);
			}
		}

		public static IntPtr GetSandboxNode()
		{
			// Try C-compatible version first, then mangled name
			try
			{
				return get_sandbox_node_c();
			}
			catch
			{
				try
				{
					return get_node_sandbox();
				}
				catch
				{
					return IntPtr.Zero;
				}
			}
		}

		public static void AddAPIFunction(IntPtr funcPtr, string returnType, string parameters, string description)
		{
			if (funcPtr == IntPtr.Zero || string.IsNullOrEmpty(returnType))
				return;

			byte* returnTypeUtf8 = StringToUtf8(returnType);
			byte* paramsUtf8 = StringToUtf8(parameters ?? "");
			byte* descUtf8 = StringToUtf8(description ?? "");

			try
			{
				add_api_function_internal(funcPtr, returnTypeUtf8, paramsUtf8, descUtf8);
			}
			finally
			{
				FreeUtf8(returnTypeUtf8);
				FreeUtf8(paramsUtf8);
				FreeUtf8(descUtf8);
			}
		}

		public static void AddProperty(string name, VariantType type, long defaultValue, IntPtr getter, IntPtr setter)
		{
			if (string.IsNullOrEmpty(name))
				return;

			byte* nameUtf8 = StringToUtf8(name);
			try
			{
				add_property_internal(nameUtf8, (int)type, defaultValue, getter, setter);
			}
			finally
			{
				FreeUtf8(nameUtf8);
			}
		}

		// Check if binary translation is enabled
		// This requires access to the Sandbox object's methods
		// We'll need to call through the Sandbox pointer
		public static bool IsBinaryTranslated(IntPtr sandboxPtr)
		{
			if (sandboxPtr == IntPtr.Zero)
				return false;

			// The is_binary_translated() method on Sandbox
			// We need to call it through the pointer
			// This is a C++ method call, so we'd need the mangled name or a C wrapper
			// For now, return false as a placeholder
			return false;
		}
	}
}

