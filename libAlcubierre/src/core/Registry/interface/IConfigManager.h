#ifndef ALCENGINE_CORE_REGISTRY_INTERFACE_ICONFIGMANAGER_H
#define ALCENGINE_CORE_REGISTRY_INTERFACE_ICONFIGMANAGER_H

#include "core/Registry/base/InterfaceBaseClass.h"

#include <type_traits>
#include <cstdint>
#include <vector>
#include <variant>
#include <stdexcept>

class IConfigManager : public InterfaceBaseClass {
    public:
        std::string token() override { return "IConfigManager"; }

        template <typename T>
        struct is_vector : std::false_type {};

        template <typename U, typename Alloc>
        struct is_vector<std::vector<U, Alloc>> : std::true_type {};

        template <typename T>
        const T Get(std::string key) {
            const std::vector<std::string> key_ = {key};
            return Get<T>(key_, T(), true);
        }

        template <typename T>
        const T Get(const std::vector<std::string> key) {
            return Get<T>(key, T(), true);
        }

        template <typename T>
        const T Get(std::string key, T defaultValue) {
            const std::vector<std::string> key_ = {key};
            return Get<T>(key_, defaultValue);
        }

        template <typename T>
        const T Get(const std::vector<std::string> key, T defaultValue, bool missingDefault = false) {
            T hold;
            TypeDescriptor* descriptor = new TypeDescriptor(std::type_identity<T>{});

            Container item = Container(key, nullptr, descriptor);

            if(GetInternal(item) == 0) {
                if(key[key.size() - 1] == "SIZE_T") {
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
                    std::string fullkey;
                    for(int i = 0; i < key.size(); i++) {
                        fullkey += key[i];
                        if(i + 1 < key.size()) fullkey += "/";
                    }
                    throw std::runtime_error("Failed to get value at key " + fullkey + " because it did not exist. Because no default value was provided this is a FATAL error");
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
        int Set(std::string key, std::string value) {
            const std::vector<std::string> key_ = {key};
            return Set<T>(key_, value);
        }

        template <typename T>
        int Set(const std::vector<std::string> key, std::string value) {
            TypeDescriptor* descriptor = new TypeDescriptor(std::type_identity<T>{});

            Container item = Container(key, &value, descriptor);
            int hold = SetInternal(item);

            delete descriptor;

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
                if(type == "void" || other.type == "void") return true; // T = void
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
            else if constexpr (std::is_same_v<T, long long>) return "long long";
            else if constexpr (std::is_same_v<T, unsigned>) return "unsigned int";
            else if constexpr (std::is_same_v<T, float>) return "float";
            else if constexpr (std::is_same_v<T, double>) return "double";
            else if constexpr (std::is_same_v<T, std::string>) return "string";
            else if constexpr (std::is_same_v<T, std::nullptr_t>) return "nullptr";
            else if constexpr (std::is_same_v<T, void*>) return "void*";
            else if constexpr (std::is_same_v<T, std::monostate>) return "void";
            else return "FAILED_GET_TYPE";
            //Congratulations, you have assigned a config value to a primitive type that *is* specified by templates, but that I forgot to account for. Amazing job. Dipshit.
            //"Why is this manually accounting" because typeid().name() is buns and im not pulling in a regex library to demangle it.
            //Just add a constexpr line to account for your type I guess. Remember that templates are made at compile so you can't do custom types because they don't exist in this translation unit.
        }

        struct Container {
            const std::vector<std::string>& key;
            void* ptr;
            TypeDescriptor* t_info;
            
            Container(const std::vector<std::string>& _key, void* value, TypeDescriptor* _t_info) : key(_key), ptr(value), t_info(_t_info) {}
        };

        virtual int GetInternal(Container& v_out) = 0; //Implementation reports status via int and fills out the ptr. Because the container is owned by the interface we can handle deletion smoothly. 
        virtual int SetInternal(Container& v_in) = 0;
        virtual int SetFromFile(const std::string& value) = 0;
};

#endif