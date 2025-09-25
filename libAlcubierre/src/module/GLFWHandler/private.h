#ifndef ALCENGINE_MODULE_GLFWHANDLER_PRIVATE_H
#define ALCENGINE_MODULE_GLFWHANDLER_PRIVATE_H

#include <GLFW/glfw3.h>

// struct IWindowManager::WindowInfo;

class GLFWImpl {
    public:
        GLFWImpl();
        ~GLFWImpl();

        int CreateWindow();
        IWindowManager::WindowInfo* GetWindowInfoIMPL();

        GLFWwindow* Window;
        IWindowManager::WindowInfo* WindowInfo;
};

#endif