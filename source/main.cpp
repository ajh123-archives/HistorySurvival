#include <glad/glad.h>  // Initialize with gladLoadGL()

// Include glfw3.h after our OpenGL definitions
#include <GLFW/glfw3.h>
#include <spdlog/common.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>

#include "vs_drawable.h"
#include "vs_shader.h"
#include "vs_ui.h"
#include "vs_ui_state.h"
#include "vs_cube.h"
#include "vs_camera.h"
#include "vs_log.h"
#include "vs_model.h"
#include "vs_skybox.h"
#include "vs_textureloader.h"
#include "vs_heightmap.h"
#include "vs_chunk.h"

void framebufferSizeCallback(GLFWwindow* window, int width, int height);
void mouseCallback(GLFWwindow* window, double xpos, double ypos);
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

static void glfw_error_callback(int error, const char* description)
{
    std::cerr << "Glfw Error " << error << ": " << description << "\n";
}

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// camera
Camera camera(glm::vec3(0.0F, 0.0F, 3.0F));
float lastX = SCR_WIDTH / 2.0F;
float lastY = SCR_HEIGHT / 2.0F;
bool firstMouse = true;

// timing
float deltaTime = 0.0F;  // time between current frame and last frame
float lastFrame = 0.0F;

int main(int, char**)
{
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (glfwInit() == 0)
    {
        return 1;
    }

    // Test Heightmap generation
    // VSHeightmap *hm = new VSHeightmap();
    // for (int i = 0; i < 10; i++)
    // {
    //     for (int j = 0; j < 10; j++)
    //     {
    //         std::cout << hm->getVoxelHeight(i, j) << " ";
    //     }
    //     std::cout << "\n";
    // }

    // Decide GL+GLSL versions
#if __APPLE__
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);  // 3.0+ only
#endif

    // Create window with graphics context
    const auto width = 1280;
    const auto height = 720;

    VSUI UI;
    // initialize logger
    VSLog::init(UI.getMutableState()->logStream);
    VSLog::Log(VSLog::Category::Core, VSLog::Level::info, "Succesfully initialized logger");

    GLFWwindow* window = glfwCreateWindow(width, height, "Voxelscape", nullptr, nullptr);
    if (window == nullptr)
    {
        VSLog::Log(VSLog::Category::Core, VSLog::Level::critical, "Failed to create GLFW window");
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSwapInterval(1);  // Enable vsync

    VSLog::Log(
        VSLog::Category::Core,
        VSLog::Level::info,
        "Succesfully setup GLFW window and opengl context");

    bool err = gladLoadGL() == 0;
    if (err)
    {
        VSLog::Log(
            VSLog::Category::Core, VSLog::Level::critical, "Failed to initialize OpenGL loader");
        return 1;
    }
    VSLog::Log(VSLog::Category::Core, VSLog::Level::info, "Succesfully initialized OpenGL loader");

    // Setup Dear ImGui context
    UI.setup(glsl_version, window);

    const auto* uiState = UI.getState();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);

    auto monkeyModel = std::make_shared<VSModel>("monkey.obj");
    auto monkeyShader = std::make_shared<VSShader>("Monkey");

    auto skybox = std::make_shared<VSSkybox>();
    auto skyboxShader = std::make_shared<VSShader>("Skybox");

    auto chunk = std::make_shared<VSChunk>(glm::vec3(50, 50, 50), 0);
    auto chunkShader = std::make_shared<VSShader>("Chunk");

    std::map<std::shared_ptr<IVSDrawable>, std::shared_ptr<VSShader>> drawables = {
        {monkeyModel, monkeyShader}, {skybox, skyboxShader}, {chunk, chunkShader}};

    VSLog::Log(VSLog::Category::Core, VSLog::Level::info, "Starting main loop");

    // Main loop
    while (glfwWindowShouldClose(window) == 0)
    {
        // per-frame time logic
        // --------------------
        double currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        glfwPollEvents();

        if (uiState->isWireframeModeEnabled)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        else
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        // Start the Dear ImGui frame
        UI.render();

        auto display_w = 0;
        auto display_h = 0;

        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);

        const auto clearColor = uiState->clearColor;
        glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);

        // Clear the screen and depth buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 Projection =
            glm::perspective(glm::radians(camera.zoom), (float)width / (float)height, 0.1F, 100.0F);
        glm::mat4 View = camera.getViewMatrix();
        glm::mat4 Model = glm::mat4(1.F);
        glm::mat4 MVP = Projection * View * Model;

        // Setup shader uniforms
        // TODO refactor into seprate function (maybe a decorator)
        // Idealy we want to draw after setting shader params
        // to avoid multiple calls to gluseprogram
        monkeyShader->uniforms()
            .setVec3("lightPos", uiState->lightPos)
            .setVec3("lightColor", uiState->lightColor)
            .setVec3("viewPos", camera.position)
            .setMat4("model", Model)
            .setMat4("MVP", MVP);

        skyboxShader->uniforms()
            .setMat4("view", glm::mat4(glm::mat3(View)))
            .setMat4("projection", Projection)
            .setVec3("viewPos", camera.position)
            .setMat4("model", Model)
            .setMat4("MVP", MVP);

        chunkShader->uniforms()
            .setVec3("lightPos", uiState->lightPos)
            .setVec3("lightColor", uiState->lightColor)
            .setVec3("viewPos", camera.position)
            .setVec3("chunkSize", chunk->getSize())
            .setMat4("model", glm::scale(Model, glm::vec3(0.0625f, 0.0625f, 0.0625f)))
            .setMat4(
                "MVP", Projection * View * glm::scale(Model, glm::vec3(0.0625f, 0.0625f, 0.0625f)));

        // draw drawables
        for (const auto& [drawable, shader] : drawables)
        {
            drawable->draw(shader);
        }

        // draw ui
        UI.draw();
        glfwSwapBuffers(window);
    }

    // Cleanup
    UI.cleanup();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react
// accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        camera.processKeyboard(FORWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        camera.processKeyboard(BACKWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        camera.processKeyboard(LEFT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        camera.processKeyboard(RIGHT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    {
        camera.processKeyboard(UP, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    {
        camera.processKeyboard(DOWN, deltaTime);
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    // Only move camera if left mouse is pressed
    int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    if (state != GLFW_PRESS)
    {
        return;
    }

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;  // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.processMouseMovement(xoffset, yoffset);
}

// glfw: whenever a mouse button is pressed or released, this callback is called
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double xpos;
        double ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        lastX = xpos;
        lastY = ypos;
    }
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.processMouseScroll(yoffset);
}