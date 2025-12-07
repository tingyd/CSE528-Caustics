#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "renderer/camera.h"
#include "renderer/renderer_cpu.h"
#include "renderer/renderer_gpu.h"
#include "renderer/scene.h"
#include "renderer/texture.h"
#include "renderer/utils.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include "renderer/shader_utils.h"
static float *g_deltaTime = nullptr;
static bool *g_cameraMovedFlag = nullptr;

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    GPUCamera::mouse_callback(window, xpos, ypos);
    if (g_cameraMovedFlag)
        *g_cameraMovedFlag = true;
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (g_deltaTime && g_cameraMovedFlag)
    {
        GPUCamera::processInput(window, *g_deltaTime, *g_cameraMovedFlag);
    }
}

static bool *g_cameraMoving_cpu = nullptr;
void cpu_cursor_callback(GLFWwindow *window, double x, double y)
{
    if (g_cameraMoving_cpu)
    {
        CPUCameraControl::cursorPosCallback(window, x, y, *g_cameraMoving_cpu);
    }
}
int main()
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }
    int frameCount = 0;
    bool cameraMovedFlag = true;
    float deltaTime = 0.0f;
    float lastTime = glfwGetTime();

    // Local state variables for CPU mode
    bool needsRenderCPU = true;
    int renderCountCPU = 0;
    bool cameraMoving = false;
    extern bool texturesEnabled;
    bool savePPMRequested = false;
    g_deltaTime = &deltaTime;
    g_cameraMovedFlag = &cameraMovedFlag;
    g_cameraMoving_cpu = &cameraMoving;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(
        WIDTH,
        HEIGHT,
        "Cornell Box - GPU Monte Carlo [1] / CPU Jensen [2]",
        nullptr,
        nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to init GLAD\n";
        return -1;
    }

    glViewport(0, 0, WIDTH, HEIGHT);

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
    std::cout << "Controls:\n";
    std::cout << "  1 : GPU Monte Carlo (compute shader)\n";
    std::cout << "  2 : CPU Jensen Photon Mapping\n";
    std::cout << "  GPU mode: WASD + Space/Shift + mouse look\n";
    std::cout << "  CPU mode: WASD + Q/E + drag LMB to rotate\n";
    std::cout << "  T (CPU mode): toggle textures\n";
    std::cout << "  ESC: quit\n\n";

    // GPU MONTE CARLO RESOURCES
    GLuint fsqVAO_GPU;
    glGenVertexArrays(1, &fsqVAO_GPU);

    GLuint accumTex;
    glGenTextures(1, &accumTex);
    glBindTexture(GL_TEXTURE_2D, accumTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLuint rayTex;
    glGenTextures(1, &rayTex);
    glBindTexture(GL_TEXTURE_2D, rayTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    std::string vsSrc = loadShaderSource("shaders/fullscreen_quad.vert");
    GLuint vsGPU = compileShader(GL_VERTEX_SHADER, vsSrc.c_str());
    std::string fsSrc = loadShaderSource("shaders/display.frag");
    GLuint fsGPU = compileShader(GL_FRAGMENT_SHADER, fsSrc.c_str());
    std::string compSrc = loadShaderSource("shaders/raytrace.comp");
    GLuint compShader = compileShader(GL_COMPUTE_SHADER, compSrc.c_str());
    GLuint compProg = linkProgram({compShader});
    GLuint fsqProg = linkProgram({vsGPU, fsGPU});

    glDisable(GL_DEPTH_TEST);

    // CPU JENSEN PHOTON MAPPING RESOURCES
    std::cout << "=== Loading Textures (CPU path) ===\n";
    floorTexture.load("textures/checkerboard.ppm");
    backWallTexture.load("textures/brick_wall.ppm");
    ceilingTexture.load("textures/ceiling.ppm");
    std::cout << "===================================\n";

    std::cout << "=== Pre-computing Photon Maps (CPU path) ===\n";
    PhotonMap causticMap, globalMap;
    std::mt19937 rng(42);
    tracePhotons(causticMap, globalMap, rng);
    std::cout << "Balancing caustic photon map...\n";
    causticMap.balance();
    std::cout << "Balancing global photon map...\n";
    globalMap.balance();
    std::cout << "=== Photon maps ready! ===\n";

    float quadVertices[] = {
        -1.0f, 1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
        1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f,
        1.0f, -1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f};

    GLuint quadVAO_CPU, quadVBO_CPU;
    glGenVertexArrays(1, &quadVAO_CPU);
    glGenBuffers(1, &quadVBO_CPU);

    glBindVertexArray(quadVAO_CPU);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO_CPU);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    GLuint vertexShaderCPU = glCreateShader(GL_VERTEX_SHADER);
    std::string cpuVsSrc = loadShaderSource("shaders/cpu_fullscreen.vert");
    const char *vsPtr = cpuVsSrc.c_str();
    glShaderSource(vertexShaderCPU, 1, &vsPtr, nullptr);
    glCompileShader(vertexShaderCPU);

    GLuint fragmentShaderCPU = glCreateShader(GL_FRAGMENT_SHADER);
    std::string cpuFsSrc = loadShaderSource("shaders/cpu_fullscreen.frag");
    const char *fsPtr = cpuFsSrc.c_str();
    glShaderSource(fragmentShaderCPU, 1, &fsPtr, nullptr);
    glCompileShader(fragmentShaderCPU);

    GLuint shaderProgramCPU = glCreateProgram();
    glAttachShader(shaderProgramCPU, vertexShaderCPU);
    glAttachShader(shaderProgramCPU, fragmentShaderCPU);
    glLinkProgram(shaderProgramCPU);

    glDeleteShader(vertexShaderCPU);
    glDeleteShader(fragmentShaderCPU);

    std::vector<unsigned char> frameData(WIDTH * HEIGHT * 3);

    GLuint cpuTexture;
    glGenTextures(1, &cpuTexture);
    glBindTexture(GL_TEXTURE_2D, cpuTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

    std::cout << "\n=== Controls (CPU path) ===\n";
    std::cout << "Mouse drag: Rotate camera\n";
    std::cout << "WASD: Move camera\n";
    std::cout << "Q/E: Move up/down\n";
    std::cout << "T: Toggle textures ON/OFF\n";
    std::cout << "1: GPU Monte Carlo\n";
    std::cout << "2: CPU Photon Mapping\n";
    std::cout << "ESC: Exit\n";
    std::cout << "===========================\n\n";

    enum RenderMode
    {
        MODE_GPU_MONTE_CARLO = 1,
        MODE_CPU_JENSEN = 2
    };

    RenderMode currentMode = MODE_GPU_MONTE_CARLO;

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, CPUCameraControl::mouseButtonCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    lastTime = glfwGetTime();
    needsRenderCPU = true;
    renderCountCPU = 0;

    // MAIN LOOP
    while (!glfwWindowShouldClose(window))
    {
        double currentTime = glfwGetTime();
        deltaTime = static_cast<float>(currentTime - lastTime);
        lastTime = currentTime;

        glfwPollEvents();

        // Mode switching
        static bool prev1 = false, prev2 = false;
        bool key1 = glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS;
        bool key2 = glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS;

        if (key1 && !prev1 && currentMode != MODE_GPU_MONTE_CARLO)
        {
            glfwSetWindowSize(window, 600, 600);

            currentMode = MODE_GPU_MONTE_CARLO;
            frameCount = 0;
            cameraMovedFlag = true;

            glfwSetCursorPosCallback(window, mouse_callback);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

            std::cout << "Switched to GPU Monte Carlo mode\n";
        }
        if (key2 && !prev2 && currentMode != MODE_CPU_JENSEN)
        {
            glfwSetWindowSize(window, 400, 400);

            currentMode = MODE_CPU_JENSEN;
            needsRenderCPU = true;
            cameraMoving = true;

            glfwSetCursorPosCallback(window, cpu_cursor_callback);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

            std::cout << "Switched to CPU Jensen Photon Mapping mode\n";
        }
        prev1 = key1;
        prev2 = key2;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }

        // GPU MONTE CARLO MODE
        if (currentMode == MODE_GPU_MONTE_CARLO)
        {
            processInput(window);

            if (cameraMovedFlag)
            {
                frameCount = 0;
                cameraMovedFlag = false;

                glBindTexture(GL_TEXTURE_2D, accumTex);
                std::vector<float> zeros(WIDTH * HEIGHT * 4, 0.0f);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WIDTH, HEIGHT, GL_RGBA, GL_FLOAT, zeros.data());
            }

            glBindImageTexture(0, accumTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
            glBindImageTexture(1, rayTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

            glUseProgram(compProg);
            glUniform2f(glGetUniformLocation(compProg, "uResolution"),
                        static_cast<float>(WIDTH),
                        static_cast<float>(HEIGHT));

            glUniform3fv(glGetUniformLocation(compProg, "uCamPos"), 1, glm::value_ptr(GPUCamera::cameraPos));
            glUniform3fv(glGetUniformLocation(compProg, "uCamFront"), 1, glm::value_ptr(GPUCamera::cameraFront));
            glUniform3fv(glGetUniformLocation(compProg, "uCamUp"), 1, glm::value_ptr(GPUCamera::cameraUp));
            glUniform1i(glGetUniformLocation(compProg, "uFrame"), frameCount);

            glDispatchCompute((WIDTH + 7) / 8, (HEIGHT + 7) / 8, 1);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

            frameCount++;

            glClear(GL_COLOR_BUFFER_BIT);
            glUseProgram(fsqProg);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, rayTex);
            glUniform1i(glGetUniformLocation(fsqProg, "uRayImage"), 0);

            glBindVertexArray(fsqVAO_GPU);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            if (frameCount % 100 == 0)
            {
                std::cout << "GPU Monte Carlo samples: " << frameCount << "\r" << std::flush;
            }
        }
        // CPU JENSEN PHOTON MAPPING MODE
        else if (currentMode == MODE_CPU_JENSEN)
        {
            processInputCPU(window, deltaTime, cameraMoving, savePPMRequested);

            if (cameraMoving)
            {
                needsRenderCPU = true;
            }

            if (needsRenderCPU && !cameraMoving)
            {
                constexpr float CAMERA_FOV = 40.0f;
                float aspectRatio = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
                float scale = std::tan(CAMERA_FOV * 0.5f * PI / 180.0f);

#pragma omp parallel for schedule(dynamic)
                for (int y = 0; y < HEIGHT; y++)
                {
                    for (int x = 0; x < WIDTH; x++)
                    {
                        std::mt19937 localRng(y * WIDTH + x + renderCountCPU * WIDTH * HEIGHT);

                        float px = ((static_cast<float>(x) + 0.5f) / WIDTH * 2.0f - 1.0f) * aspectRatio * scale;
                        float py = ((static_cast<float>(y) + 0.5f) / HEIGHT * 2.0f - 1.0f) * scale;

                        Vec3 color = renderPixel(px, py, CPUCameraControl::camera, causticMap, globalMap, localRng);

                        color.x = color.x / (1.0f + color.x);
                        color.y = color.y / (1.0f + color.y);
                        color.z = color.z / (1.0f + color.z);

                        color.x = powf(color.x, 1.0f / 2.2f);
                        color.y = powf(color.y, 1.0f / 2.2f);
                        color.z = powf(color.z, 1.0f / 2.2f);

                        auto clamp01 = [](float v)
                        {
                            return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
                        };

                        int idx = (y * WIDTH + x) * 3;
                        frameData[idx + 0] = static_cast<unsigned char>(clamp01(color.x) * 255.0f);
                        frameData[idx + 1] = static_cast<unsigned char>(clamp01(color.y) * 255.0f);
                        frameData[idx + 2] = static_cast<unsigned char>(clamp01(color.z) * 255.0f);
                    }
                }

                glBindTexture(GL_TEXTURE_2D, cpuTexture);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WIDTH, HEIGHT,
                                GL_RGB, GL_UNSIGNED_BYTE, frameData.data());

                needsRenderCPU = false;
                renderCountCPU++;

                std::cout << "CPU Jensen frame: " << renderCountCPU << "\n";
                if (renderCountCPU == 1 || savePPMRequested)
                {
                    std::string filename = (renderCountCPU == 1) ? "cornell_box_demo.ppm" : "cornell_box_frame.ppm";
                    savePPM(frameData, WIDTH, HEIGHT, filename.c_str());
                    savePPMRequested = false;
                }
            }

            glClear(GL_COLOR_BUFFER_BIT);
            glUseProgram(shaderProgramCPU);
            glBindVertexArray(quadVAO_CPU);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, cpuTexture);
            glUniform1i(glGetUniformLocation(shaderProgramCPU, "screenTexture"), 0);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        glfwSwapBuffers(window);
    }

    // CLEANUP
    glDeleteVertexArrays(1, &fsqVAO_GPU);
    glDeleteTextures(1, &accumTex);
    glDeleteTextures(1, &rayTex);
    glDeleteProgram(compProg);
    glDeleteProgram(fsqProg);

    glDeleteVertexArrays(1, &quadVAO_CPU);
    glDeleteBuffers(1, &quadVBO_CPU);
    glDeleteTextures(1, &cpuTexture);
    glDeleteProgram(shaderProgramCPU);

    glfwTerminate();
    return 0;
}