#define WIN32_LEAN_AND_MEN

#include <glad/glad.h>
#include <Windows.h>
#include <string_view>

#include "ShaderProgram.hpp"
#include "FontSprite.hpp"
#include "ShaderStorageBuffer.hpp"
#include "WindowsUtilities.hpp"

#include "DynamicSSBO.Test.hpp"


static int WindowWidth = 0;
static int WindowHeight = 0;


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
GLFWwindow* InitializeGLFWWindow(int windowWidth, int windowHeight, const std::string_view& windowTitle)
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
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(GLDebugCallback, nullptr);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
};

struct WindowData
{
    std::string TextToDraw = "";

    std::int64_t CaretIndex = 0;
};


int main()
{
    constexpr std::uint32_t initialWindowWidth = 800;
    constexpr std::uint32_t initialWindowHeight = 600;

    GLFWwindow* glfwWindow = InitializeGLFWWindow(initialWindowWidth, initialWindowHeight, "OpenGL-TextRenderer");


    auto windowData = WindowData();

    windowData.TextToDraw = "Type\nanything!";
    windowData.CaretIndex = windowData.TextToDraw.size();

    glfwSetWindowUserPointer(glfwWindow, &windowData);


    auto& textToDraw = windowData.TextToDraw;
    auto& caretIndex = windowData.CaretIndex;



    SetupOpenGL();

    // TestDynamicSSBOStd430();
    // TestDynamicSSBOStd140();

    const ShaderProgram fontSpriteShaderProgram = ShaderProgram("Shaders\\FontSpriteVertexShader.glsl", "Shaders\\FontSpriteFragmentShader.glsl");
    const ShaderProgram caretShaderProgram = ShaderProgram("Shaders\\CaretVertexShader.glsl", "Shaders\\CaretFragmentShader.glsl");

    // ShaderStorageBuffer s = ShaderStorageBuffer("Input", shaderProgram, 300);


    FontSprite fontSprite = FontSprite(13, 24, fontSpriteShaderProgram, L"Resources\\Consolas13x24.bmp");

    const auto projection = glm::ortho(0.0f, static_cast<float>(WindowWidth), static_cast<float>(WindowHeight), 0.0f, -1.0f, 1.0f);

    // Calculate projection and transform
    fontSprite.ScreenSpaceProjection = projection;
    fontSprite.Transform = glm::translate(glm::mat4(1.0f), { 100, 100, 0.0f });


    // Keyboard input handler
    glfwSetKeyCallback(glfwWindow, [](GLFWwindow* glfwWindow, int key, int scanCode, int actions, int modBits)
    {
        WindowData& windowData = *static_cast<WindowData*>(glfwGetWindowUserPointer(glfwWindow));

        auto& textToDraw = windowData.TextToDraw;
        auto& caretIndex = windowData.CaretIndex;

        if(actions == GLFW_PRESS)
            return;

        if(key == GLFW_KEY_BACKSPACE)
        {
            if(textToDraw.empty() == true)
                return;

            textToDraw.erase(textToDraw.cbegin() + caretIndex);
            caretIndex = std::max<std::int64_t>(--caretIndex, 0);

            OutputDebugStringA(std::to_string(caretIndex).c_str());
        };


        const char* pressedKeyName = glfwGetKeyName(key, scanCode);

        // Paste
        if((modBits & GLFW_MOD_CONTROL) &&
           (key == GLFW_KEY_V))
        {
            const std::string clipboardString = glfwGetClipboardString(glfwWindow);
            textToDraw.insert(textToDraw.cbegin() + caretIndex, clipboardString.cbegin(), clipboardString.cend());

            caretIndex += clipboardString.size();
            return;
        };

        // Space key pressed
        if(key == GLFW_KEY_SPACE)
        {
            textToDraw.insert(textToDraw.cbegin() + caretIndex, ' ');
            ++caretIndex;
            return;
        };

        // Newline
        if(key == GLFW_KEY_ENTER)
        {
            textToDraw.append("\n");
            return;
        };



        if(key == GLFW_KEY_RIGHT)
        {
            caretIndex = std::clamp<std::int64_t>(++caretIndex, 0, textToDraw.size());
        };
        if(key == GLFW_KEY_LEFT)
        {
            caretIndex = std::clamp<std::int64_t>(--caretIndex, 0, textToDraw.size());
        };

        // Unrecognized key pressed..
        if(pressedKeyName == nullptr)
            // Exit
            return;

        // Get the actual character
        char character = *pressedKeyName;

        // If shift is engaged..
        if(modBits & GLFW_MOD_SHIFT)
        {
            // If the pressed key is contained within alphabet character range..
            if(character >= 'a' && character <= 'z')
                // Subtract 32 from the letter so we get capital letters
                character -= ' ';

            // If the key is numeric..
            if(character >= '0' && character <= '9')
                // Subtract 16 to get the correct symbol
                character -= 16;
        };


        textToDraw.insert(textToDraw.cbegin() + caretIndex, character);
        ++caretIndex;
    });





    const float glyphHeightAsFloat = static_cast<float>(fontSprite.GetGlyphHeight());
    constexpr float caretWidth = 2.0f;

    const std::array<glm::vec2, 6> caretVertexCoordinates
    {
        // Top left
        glm::vec2{ 0.0f, 0.0f },

        // Top right
        glm::vec2{ caretWidth, 0.0f },

        // Bottom left
        glm::vec2{ 0.0f, glyphHeightAsFloat },


        // Top right
        glm::vec2{ caretWidth, 0.0f },

        // Bottom right
        glm::vec2{ caretWidth, glyphHeightAsFloat },

        // Bottom left
        glm::vec2{ 0.0f, glyphHeightAsFloat },
    };


    std::uint32_t caretVAO = 0;
    glCreateVertexArrays(1, &caretVAO);
    glBindVertexArray(caretVAO);


    std::uint32_t caretVBO = 0;
    glCreateBuffers(1, &caretVBO);
    glNamedBufferData(caretVBO, sizeof(caretVertexCoordinates), caretVertexCoordinates.data(), GL_STATIC_DRAW);
    glVertexArrayVertexBuffer(caretVAO, 0, caretVBO, 0, sizeof(glm::vec2));

    glVertexArrayAttribFormat(caretVAO, 0, 2, GL_FLOAT, false, 0);
    glVertexArrayAttribBinding(caretVAO, 0, 0);
    glEnableVertexArrayAttrib(caretVAO, 0);



    caretShaderProgram.SetMatrix4("Projection", projection);


    float delta = 0.0f;

    while(glfwWindowShouldClose(glfwWindow) == false)
    {
        glfwPollEvents();

        glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);


        fontSprite.Bind();

        const auto transformCopy = fontSprite.Transform;
        fontSprite.Transform *= glm::rotate(glm::mat4(1.0f), delta, { 0.0f, 0.0f, 1.0f });

        fontSprite.Draw(textToDraw,
                        { 1.0f, 0.0f, 0.0f, 1.0f });



        glm::vec3 caretTranslation = glm::vec4(0.0f);

        for(std::int64_t i = 0; i < caretIndex; ++i)
        {
            if(textToDraw[i] == '\n')
            {
                ++caretTranslation.y;
                caretTranslation.x = 0;
            }
            else
                ++caretTranslation.x;
        };

        caretTranslation.x *= fontSprite.GetGlyphWidth();
        caretTranslation.y *= fontSprite.GetGlyphHeight();

        caretShaderProgram.Bind();
        caretShaderProgram.SetMatrix4("Transform", glm::translate(fontSprite.Transform, caretTranslation));

        glBindVertexArray(caretVAO);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        fontSprite.Transform = transformCopy;


        glfwSwapBuffers(glfwWindow);

        delta += 0.001f;


        if(delta >= glm::pi<float>() * 2)
            delta = 0.0f;
    };
};