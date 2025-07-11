#ifndef GLFWHANDLER_ALE_H
#define GLFWHANDLER_ALE_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class GLFWHandler {
    public:
        GLFWwindow* Window;

        GLFWHandler();
        ~GLFWHandler();

    private:
        void CreateWindow(GLFWwindow*&);
};

#endif