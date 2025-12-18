#ifndef ALCENGINE_PUBLICINTERFACE_PUBLIC_H
#define ALCENGINE_PUBLICINTERFACE_PUBLIC_H

#include <string>

class AlcubierreEngineImpl;

class AlcubierreEngine {
    public:
        AlcubierreEngine();
        ~AlcubierreEngine();
        void InitEngine();

        bool ShouldClose();
        void Frame();

        int SetConfigFromJsonString(std::string jsonString);
    
    private:
        AlcubierreEngineImpl* PrivatePtr = nullptr;
};

#endif