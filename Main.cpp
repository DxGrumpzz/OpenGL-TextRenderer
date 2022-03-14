#define WIN32_LEAN_AND_MEN

#include <glad/glad.h>
#include <Windows.h>
#include <locale.h>
#include <objidl.h>
#include <gdiplus.h>
#include <GLFW/glfw3.h>
#include <string_view>
#include <iostream>
#include <fstream>
#include <vector>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <memory>
#include <map>
#include <unordered_map>
#include <numeric>

#pragma comment(lib, "gdiplus.lib")

#include "ShaderProgram.hpp"
#include "ShaderStorageBuffer.hpp"
#include "FontSprite.hpp"


int WindowWidth = 0;
int WindowHeight = 0;


void APIENTRY GLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    // Ignore unused parameter warnings
    (void*)&type;
    (void*)&source;
    (void*)&id;
    (void*)&length;
    (void*)&length;
    (void*)&userParam;

    switch(severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:
        case GL_DEBUG_SEVERITY_MEDIUM:
        case GL_DEBUG_SEVERITY_LOW:
        {
            std::cerr << message << "\n";

            __debugbreak();
            break;
        };

        default:
            break;
    };
};


void GLFWErrorCallback(int, const char* err_str) noexcept
{
    std::cerr << "GLFW Error: " << err_str << "\n";
    __debugbreak();
};


/// <summary>
/// Initialize GLFW and show window
/// </summary>
/// <param name="windowWidth"> The width of the window </param>
/// <param name="windowHeight"> The height of the window </param>
/// <param name="windowTitle"> The window's tile </param>
/// <returns></returns>
GLFWwindow* InitializeGLFWWindow(int windowWidth, int windowHeight, std::string_view windowTitle)
{
    glfwInit();

    glfwSetErrorCallback(GLFWErrorCallback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* glfwWindow = glfwCreateWindow(windowWidth, windowHeight, windowTitle.data(), nullptr, nullptr);


    glfwMakeContextCurrent(glfwWindow);

    // Draw as fast as computerly possible
    glfwSwapInterval(0);

    glfwSetFramebufferSizeCallback(glfwWindow, [](GLFWwindow* glfwWindow, int width, int height) noexcept
    {
        WindowWidth = width;
        WindowHeight = height;


        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(glfwWindow);

        glViewport(0, 0, width, height);
    });

    WindowWidth = windowWidth;
    WindowHeight = windowHeight;

    gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress));

    glfwShowWindow(glfwWindow);

    return glfwWindow;
};


void SetupOpenGL()
{
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(GLDebugCallback, nullptr);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
};



int main()
{
    constexpr std::uint32_t initialWindowWidth = 800;
    constexpr std::uint32_t initialWindowHeight = 600;

    GLFWwindow* glfwWindow = InitializeGLFWWindow(initialWindowWidth, initialWindowHeight, "OpenGL-TextRenderer");


    SetupOpenGL();

    const ShaderProgram shaderProgram = ShaderProgram("Shaders\\FontSpriteVertexShader.glsl", "Shaders\\FontSpriteFragmentShader.glsl");

    const FontSprite fontSprite = FontSprite(13, 24, shaderProgram, L"Resources\\Consolas13x24.bmp");


    static std::string textToDraw = "Type anything!Type anything1234567";


    // Keyboard input handler
    glfwSetKeyCallback(glfwWindow, [](GLFWwindow* glfwWindow, int key, int scanCode, int actions, int modBits) noexcept
    {
        if(actions == GLFW_PRESS)
            return;

        if(key == GLFW_KEY_BACKSPACE)
        {
            if(textToDraw.empty() == true)
                return;

            textToDraw.erase(textToDraw.end() - 1);
        };


        const char* pressedKeyName = glfwGetKeyName(key, scanCode);

        // Paste
        if((modBits & GLFW_MOD_CONTROL) &&
           (key == GLFW_KEY_V))
        {
            const char* clipboardString = glfwGetClipboardString(glfwWindow);

            textToDraw.append(clipboardString);

            return;
        };

        // Space key pressed
        if(key == GLFW_KEY_SPACE)
        {
            textToDraw.append(" ");
            return;
        };

        // Unrecognized key pressed..
        if(pressedKeyName == nullptr)
            // Exit
            return;

        // Get the actual key's letter
        char actualKey = *pressedKeyName;

        // If shift is engaged..
        if(modBits & GLFW_MOD_SHIFT)
        {
            // If the pressed key is contained within alphabet character range..
            if(actualKey >= 97 && actualKey <= 122)
                // Subtract 32 from the letter so we get capital letters
                actualKey -= 32;

            // If the key is numeric..
            if(actualKey >= 48 && actualKey <= 57)
                // Subtract 16 to get the correct symbol
                actualKey -= 16;

        }

        textToDraw.append(1, actualKey);
    });


    while(glfwWindowShouldClose(glfwWindow) == false)
    {
        glfwPollEvents();

        glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        fontSprite.Bind();

        fontSprite.Draw(textToDraw,
                        { 1.0f, 0.0f, 0.0f, 1.0f });

        glfwSwapBuffers(glfwWindow);

    };
};