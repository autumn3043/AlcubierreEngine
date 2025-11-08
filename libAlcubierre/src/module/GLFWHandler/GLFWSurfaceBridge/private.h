#ifndef ALCENGINE_MODULE_GLFWHANDLER_GLFWSURFACEBRIDGE_PRIVATE_H
#define ALCENGINE_MODULE_GLFWHANDLER_GLFWSURFACEBRIDGE_PRIVATE_H

class GLFWSurfaceBridgeImpl {
    public:
        Registry* registry_ptr = nullptr;
        GLFWSurfaceBridgeImpl(Registry* registry);
        ~GLFWSurfaceBridgeImpl();

        int createwindowsurface(void* TargetInstance, void* TargetSurfaceObject);
};

#endif