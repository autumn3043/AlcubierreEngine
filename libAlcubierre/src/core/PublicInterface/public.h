#ifndef ALCENGINE_PUBLICINTERFACE_PUBLIC_H
#define ALCENGINE_PUBLICINTERFACE_PUBLIC_H

class AlcubierreEngineImpl;

class AlcubierreEngine {
    public:
        AlcubierreEngine();
        ~AlcubierreEngine();
    
    private:
        AlcubierreEngineImpl* PrivatePtr = nullptr;
};

#endif