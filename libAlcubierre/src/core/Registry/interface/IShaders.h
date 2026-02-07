#ifndef ALCENGINE_CORE_REGISTRY_INTERFACE__EXAMPLE_H
#define ALCENGINE_CORE_REGISTRY_INTERFACE__EXAMPLE_H

#include "core/Registry/base/InterfaceBaseClass.h"

class IShaders : public InterfaceBaseClass {
    public:
        std::string token() override { return "IShaders"; }

        struct AlcShader {
            virtual uint32_t* const& data() const = 0;
            virtual const uint32_t size() const = 0;
        };

        virtual AlcShader* fetchShader(int index) = 0;
};

#endif