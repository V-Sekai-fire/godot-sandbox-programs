#pragma once

// Stub headers to replace Godot core dependencies with godot-sandbox API
// This allows the Godot GDScript VM code to compile in godot-sandbox

#include <api.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <set>

// Replace core/string/string_name.h
// StringName can be aliased to String in godot-sandbox
using StringName = String;

// Replace core/object/ref_counted.h
// Minimal RefCounted stub for godot-sandbox
class RefCounted {
public:
    virtual ~RefCounted() = default;
};

// Replace core/object/script_language.h
// Minimal Script stub
class Script : public RefCounted {
public:
    virtual ~Script() = default;
};

// Replace core/templates/rb_map.h
// Use std::map or std::unordered_map instead
template<typename K, typename V>
using RBMap = std::unordered_map<K, V>;

// Replace core/templates/rb_set.h
template<typename T>
using RBSet = std::unordered_set<T>;

// Replace core/templates/vector.h
template<typename T>
using Vector = std::vector<T>;

// Replace core/templates/pair.h
template<typename A, typename B>
using Pair = std::pair<A, B>;

// Replace core/templates/self_list.h
// Minimal stub - may need full implementation later
template<typename T>
class SelfList {
    // Stub implementation
};

// Replace core/templates/a_hash_map.h
template<typename K, typename V>
using HashMap = std::unordered_map<K, V>;

// Replace core/templates/hash_set.h
template<typename T>
using HashSet = std::unordered_set<T>;

// Replace core/templates/list.h
template<typename T>
using List = std::vector<T>;

// Replace core/object/class_db.h
class ClassDB {
public:
    static bool class_exists(const StringName& p_class) {
        // Stub implementation - return false for now
        return false;
    }
    static bool has_method(const StringName& p_class, const StringName& p_method, bool p_no_inheritance = false) {
        // Stub implementation - return false for now
        return false;
    }
    static bool has_property(const StringName& p_class, const StringName& p_property) {
        // Stub implementation - return false for now
        return false;
    }
    static bool is_parent_class(const StringName& p_class, const StringName& p_inherits) {
        // Stub implementation - return false for now
        return false;
    }
    static int64_t get_integer_constant(const StringName& p_class, const StringName& p_name, bool* ok) {
        // Stub implementation
        if (ok) *ok = false;
        return 0;
    }
    static MethodBind* get_method(const StringName& p_class, const StringName& p_method) {
        // Stub implementation
        return nullptr;
    }
    static void bind_method(const char* p_method, void* p_func) {
        // Stub implementation - no-op
    }
};

// Replace core/object/object.h
class Object {
public:
    virtual ~Object() = default;
    StringName get_class_name() const {
        // Stub implementation - return empty StringName
        return StringName();
    }
    template<typename T>
    static T* cast_to(Object* p_object) {
        // Stub implementation - dynamic cast
        return dynamic_cast<T*>(p_object);
    }
};

// Replace core/io/resource_loader.h and resource_saver.h
// Stubs - may not be needed for VM execution
class ResourceLoader {
    // Stub
};
class ResourceSaver {
    // Stub
};

// Replace core/debugger/engine_debugger.h
class EngineDebugger {
    // Stub
};

// Replace core/debugger/script_debugger.h
class ScriptDebugger {
    // Stub
};

// Replace core/doc_data.h
class DocData {
    // Stub
};

// Replace core/os/os.h and core/os/thread.h
class OS {
    // Stub
};
class Thread {
    // Stub
};

// Replace core/variant/typed_array.h
// TypedArray is part of Variant in godot-sandbox

// Replace core/config/engine.h and project_settings.h
class Engine {
    // Stub
};
class ProjectSettings {
    // Stub
};

// Replace scene/scene_string_names.h
class SceneStringNames {
    // Stub
};

// MethodBind stub (from core/object/method_bind.h)
class MethodBind {
    // Stub
};

// Callable::CallError (from core/object/callable.h)
namespace Callable {
    enum CallError {
        CALL_OK,
        CALL_ERROR_INVALID_METHOD,
        CALL_ERROR_INVALID_ARGUMENT,
        CALL_ERROR_TOO_MANY_ARGUMENTS,
        CALL_ERROR_TOO_FEW_ARGUMENTS,
        CALL_ERROR_INSTANCE_IS_NULL,
        CALL_ERROR_METHOD_NOT_CONST
    };
}

// PropertyInfo stub
struct PropertyInfo {
    Variant::Type type;
    String name;
    // ... other fields
};

// MethodInfo stub
struct MethodInfo {
    String name;
    // ... other fields
};

// GDCLASS macro stub (empty macro for godot-sandbox)
#define GDCLASS(ClassName, BaseClassName)

// GDSOFTCLASS macro stub (for soft classes)
#define GDSOFTCLASS(ClassName, BaseClassName)

// Additional missing types and constants
class Mutex {
    // Stub - may need implementation later
};

class ScriptInstance {
public:
    virtual ~ScriptInstance() = default;
    virtual Ref<Script> get_script() const = 0;
    virtual ScriptLanguage *get_language() = 0;
};

class ScriptLanguage {
public:
    virtual ~ScriptLanguage() = default;
};

class PlaceHolderScriptInstance : public ScriptInstance {
    // Stub
};

class ResourceFormatLoader {
public:
    enum CacheMode {
        CACHE_MODE_REUSE
    };
    virtual ~ResourceFormatLoader() = default;
};

class ResourceFormatSaver {
public:
    virtual ~ResourceFormatSaver() = default;
};

// Error enum (from core/error/error_macros.h)
enum Error {
    OK,
    FAILED,
    ERR_UNAVAILABLE,
    ERR_UNCONFIGURED,
    ERR_UNAUTHORIZED,
    ERR_PARAMETER_RANGE_ERROR,
    ERR_OUT_OF_MEMORY,
    ERR_FILE_NOT_FOUND,
    ERR_FILE_BAD_DRIVE,
    ERR_FILE_BAD_PATH,
    ERR_FILE_NO_PERMISSION,
    ERR_FILE_ALREADY_IN_USE,
    ERR_FILE_CANT_OPEN,
    ERR_FILE_CANT_WRITE,
    ERR_FILE_CANT_READ,
    ERR_FILE_UNRECOGNIZED,
    ERR_FILE_CORRUPT,
    ERR_FILE_MISSING_DEPENDENCIES,
    ERR_FILE_EOF,
    ERR_CANT_OPEN,
    ERR_CANT_CREATE,
    ERR_QUERY_FAILED,
    ERR_ALREADY_IN_USE,
    ERR_LOCKED,
    ERR_TIMEOUT,
    ERR_CANT_CONNECT,
    ERR_CANT_RESOLVE,
    ERR_CONNECTION_ERROR,
    ERR_CANT_ACQUIRE_RESOURCE,
    ERR_CANT_FORK,
    ERR_INVALID_DATA,
    ERR_INVALID_PARAMETER,
    ERR_ALREADY_EXISTS,
    ERR_DOES_NOT_EXIST,
    ERR_DATABASE_CANT_READ,
    ERR_DATABASE_CANT_WRITE,
    ERR_COMPILATION_FAILED,
    ERR_METHOD_NOT_FOUND,
    ERR_LINK_FAILED,
    ERR_SCRIPT_FAILED,
    ERR_CYCLIC_LINK,
    ERR_INVALID_DECLARATION,
    ERR_DUPLICATE_SYMBOL,
    ERR_PARSE_ERROR,
    ERR_BUSY,
    ERR_SKIP,
    ERR_HELP,
    ERR_BUG,
    ERR_PRINTER_ON_FIRE
};

// ObjectID type (from core/object/object_id.h)
typedef uint64_t ObjectID;

// Forward declarations that may be needed
class GDScriptDataType;
class GDScript;
class GDScriptFunction;
class GDScriptInstance;
class GDScriptLanguage;
class GDScriptCompiler;
class GDScriptAnalyzer;
class GDScriptDocGen;
class GDScriptLambdaCallable;
class GDScriptLambdaSelfCallable;
class GDScriptCache;
class GDScriptParser;
class GDScriptTokenizerBuffer;
class GDScriptWarning;
class GDExtension;
class RefCounted;
class Script;
class Resource;
class PackedScene;
class FileAccess;

// Additional helper macros
#define ERR_PRINT(msg) // Stub - no-op for now
#define ERR_FAIL_INDEX_V(idx, size, default_val) // Stub
#define ERR_FAIL_INDEX(idx, size) // Stub
#define ERR_FAIL_COND(cond) // Stub
#define ERR_FAIL_NULL_V_MSG(ptr, default_val, msg) // Stub
#define ERR_FAIL_NULL_V(ptr, default_val) // Stub
#define ERR_FAIL_MSG(msg) // Stub
#define ERR_FAIL() // Stub
#define CRASH_COND(cond) // Stub
#define _FORCE_INLINE_ inline
#define unlikely(x) (x)
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define itos(i) std::to_string(i)
#define vformat(fmt, ...) String::format(fmt, __VA_ARGS__)
#define D_METHOD(method) #method

// Signal class (from core/object/signal.h)
class Signal {
    // Stub - minimal implementation
public:
    Signal() {}
    ~Signal() {}
};

// KeyValue template (from core/templates/pair.h)
template<typename K, typename V>
using KeyValue = std::pair<const K, V>;

// StackInfo (from core/object/script_language.h)
struct StackInfo {
    int line;
    String func;
    String file;
};

// SafeNumeric (from core/templates/safe_ref_count.h)
template<typename T>
class SafeNumeric {
    T value;
public:
    SafeNumeric() : value(0) {}
    SafeNumeric(T v) : value(v) {}
    T get() const { return value; }
    void set(T v) { value = v; }
    T operator++() { return ++value; }
    T operator++(int) { return value++; }
    T operator--() { return --value; }
    T operator--(int) { return value--; }
    T operator+=(T v) { return value += v; }
    T operator-=(T v) { return value -= v; }
};

// CharString (from core/string/ustring.h)
class CharString {
    std::string str;
public:
    CharString() {}
    CharString(const std::string& s) : str(s) {}
    const char* get_data() const { return str.c_str(); }
    char* ptrw() { return const_cast<char*>(str.data()); }
    size_t length() const { return str.length(); }
};

// TypedArray forward declaration
template<typename T>
class TypedArray {
    // Stub
};

