#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>

namespace GPUCameraState {
    extern glm::vec3 cameraPos;
    extern glm::vec3 cameraFront;
    extern glm::vec3 cameraUp;

    extern float yaw;
    extern float pitch;

    extern float lastX;
    extern float lastY;
    extern bool firstMouse;

    extern float deltaTime;
    extern float lastFrame;

    extern bool cameraMovedFlag;
}

// Callbacks
void gpu_mouse_callback(GLFWwindow *window, double xpos, double ypos);
void gpu_processInput(GLFWwindow *window);

// Shader utilities
std::string loadShaderSource(const char *filepath);
GLuint compileShader(GLenum type, const char *src);
GLuint linkProgram(const std::vector<GLuint> &shaders);
