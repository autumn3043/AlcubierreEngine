#ifndef ALCENGINE_CORE_REGISTRY_PRIVATE_H
#define ALCENGINE_CORE_REGISTRY_PRIVATE_H

#include <functional>
#include <vector>
#include <unordered_map>
#include <memory>

#include "core/Registry/base/WrapperBaseClass.h"
#include "core/Registry/base/InterfaceBaseClass.h"

class RegistryImpl {
   public:
      std::vector<std::function<WrapperBaseClass*()>> Constructors;
      std::vector<std::unique_ptr<WrapperBaseClass>> Modules;
      std::unordered_map<std::string, InterfaceBaseClass*> Services;
};

#endif