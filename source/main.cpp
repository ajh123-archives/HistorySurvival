#include <glad/glad.h>  // Initialize with gladLoadGL()

// Include glfw3.h after our OpenGL definitions
#include <GLFW/glfw3.h>
#include <spdlog/common.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "vs_shader.h"
#include "vs_ui.h"
#include "vs_ui_state.h"
#include "vs_cube.h"
#include "vs_camera.h"

#include "vs_log.h"

#include "vs_model.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

static void glfw_error_callback(int error, const char* description)
{
    std::cerr << "Glfw Error " << error << ": " << description << "\n";
}

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;  // time between current frame and last frame
float lastFrame = 0.0f;

int main(int, char**)
{
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (glfwInit() == 0)
    {
        return 1;
    }

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
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
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

    const auto testModel = VSModel("monkey.obj");

    auto meshShader = VSShader("Mesh");

    VSLog::Log(VSLog::Category::Core, VSLog::Level::info, "Starting main loop");

    // Main loop
    while (glfwWindowShouldClose(window) == 0)
    {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
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

        // recalc camera mvp
        // TODO move ot extra function only recalc if needed
        // shader stuff should be abstracted and controlled by cube?
        // glm::mat4 Projection =
        // glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);
        glm::mat4 Projection =
            glm::perspective(glm::radians(camera.zoom), (float)width / (float)height, 0.1f, 100.0f);

        // glm::mat4 View = cam->updateCameraFromInputs(window);
        glm::mat4 View = camera.getViewMatrix();
        // glm::mat4 View = glm::lookAt(
        //     uiState->cameraPos,  // Camera is at (4,3,3), in World Space
        //     glm::vec3(0, 0, 0),  // and looks at the origin
        //     glm::vec3(0, 1, 0)   // Head is up (set to 0,-1,0 to look upside-down)
        // );

        glm::mat4 Model = glm::mat4(1.f);
        glm::mat4 MVP = Projection * View * Model;

        meshShader.setVec3("lightPos", uiState->lightPos);
        meshShader.setVec3("lightColor", uiState->lightColor);
        meshShader.setVec3("viewPos", camera.position);
        meshShader.setMat4("model", Model);
        meshShader.setMat4("MVP", MVP);
        testModel.draw(&meshShader);

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
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.processKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.processKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.processKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.processKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
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

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.processMouseScroll(yoffset);
}