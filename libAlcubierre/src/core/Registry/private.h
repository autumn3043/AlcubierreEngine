#ifndef ALCENGINE_CORE_REGISTRY_PRIVATE_H
#define ALCENGINE_CORE_REGISTRY_PRIVATE_H

#include "core/Registry/public.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <utility>

class RegistryImpl {
   private:
      Registry* registry_ptr;

      class ModuleData {
         public:
            ModuleData(ModuleRegistryBundle* _bundle, Registry* registry);
            ~ModuleData();
            ModuleData(const ModuleData&) = default;
            ModuleData& operator=(const ModuleData&) = default;
            ModuleData(ModuleData&&) = default;
            ModuleData& operator=(ModuleData&&) = default;

            bool exists();
            int instantiate();
            int destruct();
            const std::vector<ServiceID>& serviceArr();
            const std::vector<ServiceID>& dependsArr();
            InterfaceBaseClass*& GetService(ServiceID id);

         private:
            Registry* registry_ptr;

            std::string Label;
            std::unique_ptr<WrapperBaseClass> Instance;
            std::function<WrapperBaseClass*(Registry*)> Constructor;
            std::vector<ServiceID> Provides;
            std::vector<ServiceID> Depends;
      };

      std::vector<ModuleData> Modules;
      std::unordered_map<ServiceID, std::pair<std::vector<int>, std::vector<int>>> Services;

      int SelectBestProvider(ServiceID id);

   public:
      RegistryImpl(Registry* _parent);
      ~RegistryImpl();

      int GracefulDeconstruct(ModuleData& module);

      InterfaceBaseClass*& FetchService(ServiceID id);

};

#endif