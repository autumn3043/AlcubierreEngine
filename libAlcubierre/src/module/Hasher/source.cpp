#include "module/Hasher/private.h"

static void logIdentity(std::string message, int level = 0, bool Write = true) { return DM().Log(DebugReport(message, level, "Hasher"), Write); }

ModuleRegistryBundle Hasher::bundle(
    [](void* registry) -> WrapperBaseClass* { return new Hasher(registry); },
    {HASH_GENERATOR},
    {},
    "Hasher"
);

Hasher::Hasher(void* registry) 
    :   IHashGenerator_Hasher(this),
        registry_ptr(static_cast<Registry*>(registry))
    {
        Services = {{HASH_GENERATOR, &IHashGenerator_Hasher}};
        init();
    }

void Hasher::init() {
    int dummy;
    hashSeed = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&dummy));
}

#include "xxhash.h"
uint32_t Hasher::hash32(const void* data, size_t dataSize) {
    return XXH32(data, dataSize, hashSeed);
}
uint64_t Hasher::hash64(const void* data, size_t dataSize) {
    return XXH64(data, dataSize, hashSeed);
}
