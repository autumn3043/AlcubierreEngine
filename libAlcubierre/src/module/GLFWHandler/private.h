#ifndef ALCENGINE_MODULE_GLFWHANDLER_PRIVATE_H
#define ALCENGINE_MODULE_GLFWHANDLER_PRIVATE_H

#include <GLFW/glfw3.h>

class GLFWImpl {
    public:
        GLFWImpl();
        ~GLFWImpl();

        int CreateWindow();

        GLFWwindow* Window;
};

#endif