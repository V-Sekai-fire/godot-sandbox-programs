#pragma once
// Minimal API header for standalone compilation

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

typedef int64_t godot_int;
typedef uint64_t godot_uint;
typedef int32_t godot_bool;

// Minimal Godot types needed for mock handlers
namespace godot {
    class String {
    public:
        String(const char* s = "") : data(s ? s : "") {}
        const char* utf8() const { return data.c_str(); }
    private:
        std::string data;
    };

    class Array {
    public:
        Array() = default;
        godot_int size() const { return static_cast<godot_int>(data.size()); }
        void push_back(const godot::String& s) { data.push_back(s); }
        godot::String operator[](godot_int idx) const { return data[idx]; }
        void resize(godot_int s) { data.resize(s); }
        void set(godot_int idx, const godot::String& val) { if (idx >= 0 && idx < data.size()) data[idx] = val; }
    private:
        std::vector<godot::String> data;
    };

    class Dictionary {
    public:
        Dictionary() = default;
        godot_int size() const { return static_cast<godot_int>(data.size()); }
        void set(const godot::String& key, const godot::String& val) { data[key.utf8()] = val.utf8(); }
        godot::String get(const godot::String& key, const godot::String& def = "") const {
            auto it = data.find(key.utf8());
            if (it != data.end()) return godot::String(it->second.c_str());
            return def;
        }
        bool has(const godot::String& key) const { return data.find(key.utf8()) != data.end(); }
        Array keys() const {
            Array result;
            for (const auto& kv : data) {
                result.push_back(godot::String(kv.first.c_str()));
            }
            return result;
        }
    private:
        std::unordered_map<std::string, std::string> data;
    };

    class Variant {
    public:
        enum Type { NIL, STRING };
        Variant() : type(NIL) {}
        Variant(const char* s) : type(STRING), string(s ? s : "") {}
        Type get_type() const { return type; }
        godot::String as_string() const { return string; }
    private:
        Type type;
        godot::String string;
    };
};

// Type aliases for convenience
using Dictionary = godot::Dictionary;
using Array = godot::Array;
using String = godot::String;
using Variant = godot::Variant;

// Mock Object class
typedef struct _Object {
    const char* _name;
    void* _internal;
} Object;

inline Object* Object_new(const char* name) {
    Object* obj = new Object;
    obj->_name = name;
    obj->_internal = nullptr;
    return obj;
}

inline void Object_call(Object* obj, const char* method, ...) {
}

inline void Object_destroy(Object* obj) {
    //    delete obj;
}

// Global functions
inline godot::String String_from_char(const char* s) { return godot::String(s); }
inline godot::Array Array_new() { return godot::Array(); }
inline void Array_push_back(godot::Array& arr, const godot::String& val) { arr.push_back(val); }
inline godot::Dictionary Dictionary_new() { return godot::Dictionary(); }
inline void Dictionary_set(godot::Dictionary& dict, const godot::String& key, const godot::String& val) { dict.set(key, val); }
inline godot::String Dictionary_get(godot::Dictionary& dict, const godot::String& key, const godot::String& def) { return dict.get(key, def); }

// For JSON-RPC
inline const char* String_utf8(const godot::String& s) { return s.utf8(); }
inline int64_t Array_size(const godot::Array& arr) { return arr.size(); }
inline int64_t Dictionary_size(const godot::Dictionary& dict) { return dict.size(); }
