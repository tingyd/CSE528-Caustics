#include "renderer/renderer_gpu.h"
#include <fstream>
#include <sstream>
#include <iostream>

// Define the variables INSIDE the namespace
namespace GPUCameraState {
    glm::vec3 cameraPos = glm::vec3(278, 273, -800);
    glm::vec3 cameraFront = glm::vec3(0, 0, 1);
    glm::vec3 cameraUp = glm::vec3(0, 1, 0);
    float yaw = 90.0f;
    float pitch = 0.0f;
    float lastX = 400;
    float lastY = 400;
    bool firstMouse = true;
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
    bool cameraMovedFlag = false;
}

// Now bring them into scope for convenience
using namespace GPUCameraState;

void gpu_mouse_callback(GLFWwindow *window, double xposIn, double yposIn)
{
    float xpos = float(xposIn);
    float ypos = float(yposIn);
    if (firstMouse) {
        firstMouse = false;
        lastX = xpos;
        lastY = ypos;
        return;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;
    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;
    yaw += xoffset;
    pitch += yoffset;
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
    cameraMovedFlag = true;
}

void gpu_processInput(GLFWwindow *window)
{
    float speed = 200.0f * deltaTime;
    bool moved = false;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) { cameraPos += cameraFront * speed; moved = true; }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) { cameraPos -= cameraFront * speed; moved = true; }
    glm::vec3 right = glm::normalize(glm::cross(cameraFront, cameraUp));
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) { cameraPos -= right * speed; moved = true; }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) { cameraPos += right * speed; moved = true; }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) { cameraPos += cameraUp * speed; moved = true; }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) { cameraPos -= cameraUp * speed; moved = true; }
    if (moved) cameraMovedFlag = true;
}

std::string loadShaderSource(const char *filepath)
{
    std::ifstream file(filepath);
    if (!file) {
        std::cerr << "Could not load shader: " << filepath << "\n";
        return "";
    }
    std::stringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

GLuint compileShader(GLenum type, const char *src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetShaderInfoLog(s, 2048, NULL, log);
        std::cerr << "Shader error:\n" << log << "\n";
    }
    return s;
}

GLuint linkProgram(const std::vector<GLuint> &shaders)
{
    GLuint p = glCreateProgram();
    for (GLuint s : shaders) glAttachShader(p, s);
    glLinkProgram(p);
    GLint ok;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetProgramInfoLog(p, 2048, NULL, log);
        std::cerr << "Link error:\n" << log << "\n";
    }
    for (GLuint s : shaders) glDeleteShader(s);
    return p;
}