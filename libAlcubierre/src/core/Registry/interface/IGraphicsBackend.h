#ifndef ALCENGINE_CORE_REGISTRY_INTERFACE_IGRAPHICSBACKEND_H
#define ALCENGINE_CORE_REGISTRY_INTERFACE_IGRAPHICSBACKEND_H

#include "core/Registry/base/InterfaceBaseClass.h"

class IGraphicsBackend : public InterfaceBaseClass {
    public:
        std::string token() override { return "IGraphicsBackend"; }

        virtual int createObjectBuffer(uint32_t& modelHash, std::vector<Vector>& vertices, std::vector<uint32_t>& indices) = 0;

        virtual int clearFrame() = 0;
        virtual int addObjectToFrame(uint32_t& modelHash, Vector& position) = 0;
        virtual int drawFrame() = 0;
};

#endif