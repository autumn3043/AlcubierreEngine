//THIS HASH GENERATOR IS NOT INTENDED FOR ANY CRYPTOGRAPHICAL USE
//IT IS FOR LIGHTWEIGHT UNIQUE INSTANCE DETECTION ONLY
//DO NOT USE THIS GENERATOR OR ITS IMPLEMENTATION FOR ANY PROGRAM WHERE SECURITY IS EVEN REMOTELY RELEVANT

#ifndef ALCENGINE_CORE_REGISTRY_INTERFACE_HASHGENERATOR_H
#define ALCENGINE_CORE_REGISTRY_INTERFACE_HASHGENERATOR_H

#include "core/Registry/base/InterfaceBaseClass.h"

class IHashGenerator : public InterfaceBaseClass {
    public:
        std::string token() override { return "IHashGenerator"; }

        virtual uint32_t hash32(const void* data, size_t dataSize) = 0;
        virtual uint64_t hash64(const void* data, size_t dataSize) = 0;
};

#endif