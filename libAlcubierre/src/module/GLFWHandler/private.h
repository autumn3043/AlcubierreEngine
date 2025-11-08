#ifndef ALCENGINE_MODULE_GLFWHANDLER_PRIVATE_H
#define ALCENGINE_MODULE_GLFWHANDLER_PRIVATE_H

#include <GLFW/glfw3.h>

class GLFWImpl {
    public:
        Registry* registry_ptr = nullptr;
        GLFWImpl(Registry* registry);
        ~GLFWImpl();

        int CreateWindow();
        IWindowManager::WindowInfo* GetWindowInfoIMPL();
        bool TouchSurfaceApi();

        bool apiStatus = false;

        GLFWwindow* Window;
        IWindowManager::WindowInfo* WindowInfo;
};

#endif