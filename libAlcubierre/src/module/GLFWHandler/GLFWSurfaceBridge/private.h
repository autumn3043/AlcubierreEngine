#ifndef ALCENGINE_MODULE_GLFWHANDLER_GLFWSURFACEBRIDGE_PRIVATE_H
#define ALCENGINE_MODULE_GLFWHANDLER_GLFWSURFACEBRIDGE_PRIVATE_H

class GLFWSurfaceBridgeImpl {
    public:
        GLFWSurfaceBridgeImpl();
        ~GLFWSurfaceBridgeImpl();

        int createwindowsurface(void* TargetInstance, void* TargetSurfaceObject);
};

#endif