#ifndef ALCENGINE_CORE_REGISTRY_INTERFACE_ICONFIGMANAGER_H
#define ALCENGINE_CORE_REGISTRY_INTERFACE_ICONFIGMANAGER_H

#include "core/Registry/base/InterfaceBaseClass.h"

#include <typeinfo> //This prevents undefined behaviour when a value is malformed.

class IConfigManager : public InterfaceBaseClass {
    public:
        std::string token = "IConfigManager";

        //This interface is a bit weighty because we handle creating a return object in the same context as type detection, the trade off being a brief invokation. TODO.
        struct Container {
            void* ptr = nullptr;
            std::type_info T_info;
            std::string& key;

            Container(std::string& _k, type_info _ti) : key(_k), T_info(_ti) {}
        };

        template <typename T>
        T GetKey(const std::string key, const T defaultValue) {
            T Rvalue = defaultValue;
            Container Cvalue = Container(key, typeid(T));

            if(Get(Cvalue) == 0) {
                T* ptrCopy = static_cast<T*>(Cvalue.ptr);
                Rvalue = *ptrCopy;
            }

            if(Cvalue.ptr) {
                delete static_cast<T*>(Cvalue.ptr); //Implementation promises to assign the ptr *if and only if* it matches the requested type
            } else {
                delete Cvalue.ptr; //If Cvalue isn't set, then it is just nullptr and safe to delete like this. This doesn't actually do anything but it helps to visualise the process.
            }

            return Rvalue;

        }

    private:
        virtual const int Get(Container& v_out) = 0;

    //akjsdn

        template <typename T>
        int SetKey(const std::string& key, const T& value) {
            return Set(key, static_cast<const void*>(&value)); 
        }

        virtual int Set(const std::string& key, const void* value) = 0;
        //Again, we use a template to allow for arbitrary types to be handed over without the interface ever having to see the backend parser. 
        //void* must cast to the type the backend thinks it is or the entire universe will sponteneously combust. TODO

        virtual int Import(const std::string& dataset) = 0;
        //Dataset in backend-parseable format. Maybe having this be a string is the flimsiest part of this interface. Maybe you should shut up and cast the damn string.
};

#endif