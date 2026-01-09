#ifndef ALCENGINE_CORE_REGISTRY_INTERFACE_IGRAPHICSBACKEND_H
#define ALCENGINE_CORE_REGISTRY_INTERFACE_IGRAPHICSBACKEND_H

#include "core/Registry/base/InterfaceBaseClass.h"

class IGraphicsBackend : public InterfaceBaseClass {
    public:
        std::string token() override { return "IGraphicsBackend"; }

        struct modelData {
            std::vector<Vector> vertices;
            std::vector<uint32_t> indices;
        };

        virtual int createObjectBuffer(int*& modelBufferIndex, modelData& data) = 0;

        struct placementData {
            Vector position;
        };

        virtual int addObjectToFrame(int& modelBufferIndex, placementData& data) = 0;

        virtual int discardObjectBuffer(int& modelBufferIndex) = 0;

        virtual int clearFrame() = 0;
        virtual int drawFrame() = 0;
};

#endif