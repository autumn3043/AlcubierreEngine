#ifndef ALCENGINE_MODULE_Hasher_H
#define ALCENGINE_MODULE_Hasher_H

#include "core/Registry/public.h"

//Services
//Depends
//Provides
#include "core/Registry/interface/IHashGenerator.h"

#include "core/DebugManager/public.h"

class HasherException : public AlcEngineException {
    public:
        HasherException(std::string message) : AlcEngineException(DebugReport(message)) {}
};

class Hasher : public WrapperBaseClass {
    public:
        Hasher(void* registry);

        void init();

        class IHashGeneratorImpl : public IHashGenerator {
            private:
                Hasher* parent;

            public:
                IHashGeneratorImpl(Hasher* _parent) : parent(_parent) {}

                uint32_t hash32(const void* data, size_t dataSize) override { return parent->hash32(data, dataSize); }
                uint64_t hash64(const void* data, size_t dataSize) override { return parent->hash64(data, dataSize); }
        };
        IHashGeneratorImpl IHashGenerator_Hasher;

        uint32_t hash32(const void* data, size_t dataSize);
        uint64_t hash64(const void* data, size_t dataSize);
    
    private:
        static ModuleRegistryBundle bundle;
        Registry* registry_ptr = nullptr;

        uint32_t hashSeed;
};

#endif