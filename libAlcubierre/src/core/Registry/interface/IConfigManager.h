#ifndef ALCENGINE_CORE_REGISTRY_INTERFACE_ICONFIGMANAGER_H
#define ALCENGINE_CORE_REGISTRY_INTERFACE_ICONFIGMANAGER_H

#include "core/Registry/base/InterfaceBaseClass.h"

#include <type_traits>
#include <cstdint>
#include <vector>
#include <variant>
#include <stdexcept>
#include <functional>

#define CFGARRAY_SIZE_T "SIZE_T"

class IConfigManager : public InterfaceBaseClass {
    public:
        std::string token() override { return "IConfigManager"; }

        struct key_t {
            std::vector<std::string> steps;

            key_t(const char* _key) : steps({std::string(_key)}) {}
            key_t(std::string _key) : steps({_key}) {}
            key_t(std::initializer_list<std::string> _steps) : steps(std::vector<std::string>(_steps)) {}
            key_t(std::vector<std::string> _steps) : steps(_steps) {}

            const std::string toString() const { return toString(steps.size()); }
            const std::string toString(int depth) const {
                assert(depth <= steps.size());

                std::string fullkey;

                for(int i = 0; i < depth; i++) {
                    fullkey += steps[i];
                    if(i + 1 < depth) fullkey += "/";
                }

                return fullkey;
            }

            bool operator==(const key_t& other) const {
                return steps == other.steps;
            }
        };

        template <typename T>
        struct is_vector : std::false_type {};

        template <typename U, typename Alloc>
        struct is_vector<std::vector<U, Alloc>> : std::true_type {};

        template <typename T>
        const T Get(key_t key, int* resultPtr = nullptr) {
            return Get<T>(key, T(), resultPtr, true);
        }

        template <typename T>
        const T Get(key_t key, T defaultValue, int* resultPtr = nullptr, bool missingDefault = false) {
            T hold;
            TypeDescriptor* descriptor = new TypeDescriptor(std::type_identity<T>{});

            Container item = Container(key, nullptr, descriptor);
            int result = getInternal(item);
            if(resultPtr) *resultPtr = result;

            if(result == 0) {
                if(key.steps[key.steps.size() - 1] == CFGARRAY_SIZE_T) {
                    if constexpr (std::is_same_v<T, int>) {
                        hold = Extract<int>(item.ptr);
                    } else {
                        hold = defaultValue;
                    }
                } else {
                    hold = Extract<T>(item.ptr);
                }

            } else {
                //Implementation promises that pointer is nullptr in this case.

                if(!missingDefault) {
                    hold = defaultValue;
                } else {
                    throw std::runtime_error("Failed to get value at key " + key.toString() + " because it did not exist. Because no default value was provided this is a FATAL error");
                }
            }
            
            delete descriptor;
            return hold;
            
        }

        template <typename T>
        T Extract(void* ptr) {
            T hold;
            
            if constexpr (is_vector<T>::value) {
                std::vector<void*>* vector = static_cast<std::vector<void*>*>(ptr);

                for(void* element : *vector) {
                    hold.emplace_back(Extract<typename T::value_type>(element));
                }
            
                delete vector;

            } else {
                hold = *static_cast<T*>(ptr);
                delete static_cast<T*>(ptr);
            }

            return hold;
        }

        template <typename T>
        int Set(key_t key, std::string value) {
            TypeDescriptor* descriptor = new TypeDescriptor(std::type_identity<T>{});

            Container item = Container(key, &value, descriptor);
            int hold = setInternal(item);

            delete descriptor;

            return hold;
        }

        int SetRaw(std::string value) {
            key_t dummy = key_t(std::string());
            Container item = Container(dummy, &value, nullptr);
            int hold = setParseInternal(item);

            return hold;
        }

        struct TypeDescriptor{
            TypeDescriptor() = default;

            template <typename T>
            explicit TypeDescriptor(std::type_identity<T> = {}) {
                if constexpr (is_vector<T>::value) {
                    nested = new TypeDescriptor(std::type_identity<typename T::value_type>{});
                    type = "!!VECTOR!! You should never see this.";

                } else {
                    nested = nullptr;
                    type = human_readable_type<T>();
                }
            }

            ~TypeDescriptor() {
                if(nested) delete nested;
            }

            std::string Type() {
                if(nested) {
                    return "vector<" + nested->Type() + ">";

                } else {
                    return type;
                }
            }

            std::string ChildType() {
                if(nested) {
                    return nested->ChildType();

                } else {
                    return type;
                }
            }

            bool operator==(const TypeDescriptor& other) const {
                if(type == "lossy" || other.type == "lossy") return true; // T = std::monostate (don't care)
                else if(nested && other.nested) return *nested == *other.nested; //vector<T> == Vector<U>
                else if(nested || other.nested) return false; //vector<T> != U
                else if(type == other.type) return true; //T = T
                else return false;
            }

            bool operator!=(const TypeDescriptor& other) const {
                return !(*this == other);
            }

            TypeDescriptor* nested;
            std::string type;
        };

        template <typename T>
        static std::string human_readable_type() {
            if constexpr(std::is_same_v<T, bool>) return "bool";
            else if constexpr (std::is_same_v<T, int>) return "int";
            else if constexpr (std::is_same_v<T, int64_t>) return "int";
            else if constexpr (std::is_same_v<T, uint64_t>) return "int";
            else if constexpr (std::is_same_v<T, long>) return "long";
            else if constexpr (std::is_same_v<T, long long>) return "long_long";
            else if constexpr (std::is_same_v<T, unsigned>) return "unsigned_int";
            else if constexpr (std::is_same_v<T, float>) return "float";
            else if constexpr (std::is_same_v<T, double>) return "double";
            else if constexpr (std::is_same_v<T, std::string>) return "string";
            else if constexpr (std::is_same_v<T, std::nullptr_t>) return "nullptr";
            else if constexpr (std::is_same_v<T, void*>) return "lossy";
            else if constexpr (std::is_same_v<T, std::monostate>) return "lossy"; //This doesn't really work, because we can't return metadata about the object like array size through this.
            else return "FAILED_GET_TYPE";
            //Congratulations, you have assigned a config value to a primitive type that *is* specified by templates, but that I forgot to account for. Amazing job. Dipshit.
            //"Why is this manually accounting" because typeid().name() is buns and im not pulling in a regex library to demangle it.
            //Just add a constexpr line to account for your type I guess. Remember that templates are made at compile so you can't do custom types because they don't exist in this translation unit.
        }

        struct Container {
            key_t& key;
            void* ptr;
            TypeDescriptor* t_info;
            
            Container(key_t& _key, void* value, TypeDescriptor* _t_info) : key(_key), ptr(value), t_info(_t_info) {}
        };

        virtual int getInternal(Container& v_out) = 0; //Implementation reports status via int and fills out the ptr. Because the container is owned by the interface we can handle deletion smoothly. 
        virtual int setInternal(Container& v_in) = 0;
        virtual int setParseInternal(Container& v_in) = 0;
        virtual int dump() = 0; //Print current config contents to DebugManager.
        virtual int registerCallback(key_t key, std::function<void()> fn) = 0; //CM->registerCallback(key, [this](){functionName();});
};

#endif