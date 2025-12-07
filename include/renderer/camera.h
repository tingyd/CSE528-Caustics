#pragma once
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <cmath>
#include <algorithm>
#include <iostream>

struct Vec3
{
    float x, y, z;
    Vec3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
    Vec3 operator+(const Vec3 &v) const { return Vec3(x + v.x, y + v.y, z + v.z); }
    Vec3 operator-(const Vec3 &v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
    Vec3 operator-() const { return Vec3(-x, -y, -z); }
    Vec3 operator*(float t) const { return Vec3(x * t, y * t, z * t); }
    Vec3 operator/(float t) const { return Vec3(x / t, y / t, z / t); }
    Vec3 operator*(const Vec3 &v) const { return Vec3(x * v.x, y * v.y, z * v.z); }
    Vec3 &operator+=(const Vec3 &v)
    {
        x += v.x; y += v.y; z += v.z;
        return *this;
    }
    Vec3 &operator-=(const Vec3 &v)
    {
        x -= v.x; y -= v.y; z -= v.z;
        return *this;
    }
    float dot(const Vec3 &v) const { return x * v.x + y * v.y + z * v.z; }
    Vec3 cross(const Vec3 &v) const { return Vec3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x); }
    float lengthSq() const { return x * x + y * y + z * z; }
    float length() const { return std::sqrt(lengthSq()); }
    Vec3 normalize() const
    {
        float l = length();
        return l > 0 ? Vec3(x / l, y / l, z / l) : Vec3();
    }
    float operator[](int i) const { return i == 0 ? x : (i == 1 ? y : z); }
};
inline Vec3 operator*(float t, const Vec3 &v) { return v * t; }


namespace GPUCamera {
    inline glm::vec3 cameraPos = glm::vec3(278.0f, 273.0f, -800.0f);
    inline glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, 1.0f);
    inline glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    inline float yaw = 90.0f;
    inline float pitch = 0.0f;
    inline float lastX = 400.0f; 
    inline float lastY = 400.0f;
    inline bool firstMouse = true;

    inline void mouse_callback(GLFWwindow *window, double xposIn, double yposIn)
    {
        float xpos = static_cast<float>(xposIn);
        float ypos = static_cast<float>(yposIn);

        if (firstMouse)
        {
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

        yaw   += xoffset;
        pitch += yoffset;

        if (pitch > 89.0f)  pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFront = glm::normalize(front);
    }

    inline void processInput(GLFWwindow *window, float deltaTime, bool& cameraMovedFlag)
    {
        float cameraSpeed = 200.0f * deltaTime;
        bool moved = false;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            cameraPos += cameraSpeed * cameraFront;
            moved = true;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            cameraPos -= cameraSpeed * cameraFront;
            moved = true;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
            moved = true;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
            moved = true;
        }
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            cameraPos += cameraUp * cameraSpeed;
            moved = true;
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            cameraPos -= cameraUp * cameraSpeed;
            moved = true;
        }

        if (moved) cameraMovedFlag = true;
    }
}


class CPUCamera {
public:
    Vec3 position;
    float yaw;
    float pitch;

    CPUCamera()
        : position(278.0f, 273.0f, -800.0f),
          yaw(0.0f),
          pitch(0.0f)
    {
    }

    Vec3 getForward() const
    {
        return Vec3(
                   cosf(pitch) * sinf(yaw),
                   sinf(pitch),
                   cosf(pitch) * cosf(yaw))
            .normalize();
    }

    Vec3 getRight() const
    {
        Vec3 forward = getForward();
        return Vec3(0, 1, 0).cross(forward).normalize();
    }

    Vec3 getUp() const
    {
        Vec3 forward = getForward();
        Vec3 right = getRight();
        return forward.cross(right).normalize();
    }
};

namespace CPUCameraControl {
    inline CPUCamera camera;
    inline bool mousePressed = false;
    inline double lastMouseX = 0, lastMouseY = 0;
    
    inline void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT)
        {
            if (action == GLFW_PRESS)
            {
                mousePressed = true;
                glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
            }
            else
            {
                mousePressed = false;
            }
        }
    }

    inline void cursorPosCallback(GLFWwindow *window, double xpos, double ypos, bool& cameraMoving)
    {
        if (mousePressed)
        {
            cameraMoving = true;
            float sensitivity = 0.003f;
            float dx = float(xpos - lastMouseX) * sensitivity;
            float dy = float(ypos - lastMouseY) * sensitivity;

            camera.yaw += dx;
            camera.pitch -= dy;
            camera.pitch = std::max(-1.55f, std::min(1.55f, camera.pitch));

            lastMouseX = xpos;
            lastMouseY = ypos;
        }
    }

    inline void processInput(GLFWwindow *window, float deltaTime, bool& cameraMoving, 
                            bool& texturesEnabled, bool& savePPMRequested)
    {
        cameraMoving = false;
        float moveSpeed = 400.0f * deltaTime;

        Vec3 forward = camera.getForward();
        Vec3 right = camera.getRight();
        Vec3 up(0, 1, 0);

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            camera.position += forward * moveSpeed;
            cameraMoving = true;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            camera.position -= forward * moveSpeed;
            cameraMoving = true;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            camera.position -= right * moveSpeed;
            cameraMoving = true;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            camera.position += right * moveSpeed;
            cameraMoving = true;
        }
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            camera.position += up * moveSpeed;
            cameraMoving = true;
        }
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
            camera.position -= up * moveSpeed;
            cameraMoving = true;
        }

        // Save PPM
        static bool pPressed = false;
        if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !pPressed) {
            std::cout << "Saving current frame..." << std::endl;
            savePPMRequested = true;
            pPressed = true;
        }
        if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE) {
            pPressed = false;
        }

        // Toggle textures
        static bool tPressed = false;
        if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && !tPressed) {
            texturesEnabled = !texturesEnabled;
            cameraMoving = true;
            std::cout << "Textures: " << (texturesEnabled ? "ON" : "OFF") << std::endl;
            tPressed = true;
        }
        if (glfwGetKey(window, GLFW_KEY_T) == GLFW_RELEASE) {
            tPressed = false;
        }
    }
}