#ifndef ALCENGINE_MODULE_GLFWHANDLER_PUBLIC_H
#define ALCENGINE_MODULE_GLFWHANDLER_PUBLIC_H

class GLFWImpl;

#include "core/Registry/interface/IWindowManager.h"

class GLFWHandler {
    public:
        GLFWHandler();
        ~GLFWHandler();

    private:
        GLFWImpl* PrivatePtr;

        IWindowManager& IWindowManager_IMPL;
};

#endif