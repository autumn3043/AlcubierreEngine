#ifndef ALCENGINE_PUBLICINTERFACE_PUBLIC_H
#define ALCENGINE_PUBLICINTERFACE_PUBLIC_H

class AlcubierreEngineImpl;

class AlcubierreEngine {
    public:
        AlcubierreEngine();
        ~AlcubierreEngine();

        bool ShouldClose();

        void Frame();
    
    private:
        AlcubierreEngineImpl* PrivatePtr = nullptr;
};

#endif