#ifndef ALCENGINE_PUBLICINTERFACE_PUBLIC_H
#define ALCENGINE_PUBLICINTERFACE_PUBLIC_H

#include <string>
#include <vector>

class AlcubierreEngineImpl;

class AlcubierreEngine {    
    private:
        AlcubierreEngineImpl* PrivatePtr = nullptr;

        class Debug {
            private:
                AlcubierreEngineImpl*& PrivatePtr;

            public:
                void log(std::string message, int priority = 0);

                Debug(AlcubierreEngineImpl*& _PrivatePtr) : PrivatePtr(_PrivatePtr) {};
        };

        class Config {
            private:
                AlcubierreEngineImpl*& PrivatePtr;

            public:
                int get(std::vector<std::string> key, bool* returnPtr);
                int get(std::vector<std::string> key, int* returnPtr);
                int get(std::vector<std::string> key, float* returnPtr);
                int get(std::vector<std::string> key, std::string* returnPtr);
                
                int parse(std::string value);
                int dump();

                Config(AlcubierreEngineImpl*& _PrivatePtr) : PrivatePtr(_PrivatePtr) {};
        };

        class Window {
            private:
                AlcubierreEngineImpl*& PrivatePtr;

            public:
                bool shouldClose();

                Window(AlcubierreEngineImpl*& _PrivatePtr) : PrivatePtr(_PrivatePtr) {};

        };

        class Graphics {
            private:
                AlcubierreEngineImpl*& PrivatePtr;

            public:
                int frame();

                Graphics(AlcubierreEngineImpl*& _PrivatePtr) : PrivatePtr(_PrivatePtr) {};
        };

        class Input {
            private:
                AlcubierreEngineImpl*& PrivatePtr;

            public:
                Input(AlcubierreEngineImpl*& _PrivatePtr) : PrivatePtr(_PrivatePtr) {};

        };
    
    public:
        AlcubierreEngine();
        ~AlcubierreEngine();
        void initEngine();

        Debug debug = Debug(PrivatePtr);
        Config config = Config(PrivatePtr);
        Window window = Window(PrivatePtr);
        Graphics graphics = Graphics(PrivatePtr);
        Input input = Input(PrivatePtr);
};

#endif