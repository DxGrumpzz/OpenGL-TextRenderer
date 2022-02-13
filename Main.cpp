#include <Windows.h>
#include <gdiplus.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string_view>
#include <iostream>
#include <fstream>
#include <vector>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>

#pragma comment(lib, "gdiplus.lib")


static int WindowWidth = 0;
static int WindowHeight = 0;


void APIENTRY GLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
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


void GLFWErrorCallback(int, const char* err_str)
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

    glfwSetFramebufferSizeCallback(glfwWindow, [](GLFWwindow* glfwWindow, int width, int height)
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



/// <summary>
/// Read all text inside a file
/// </summary>
/// <param name="filename"> Path to sid file </param>
/// <returns></returns>
std::string ReadAllText(const std::string& filename)
{
    // Open the file at the end so we can easily find its length
    std::ifstream fileStream = std::ifstream(filename, std::ios::ate);

    std::string fileContents;

    // Resize the buffer to fit content
    fileContents.resize(fileStream.tellg());

    fileStream.seekg(std::ios::beg);

    // Read file contents into the buffer
    fileStream.read(fileContents.data(), fileContents.size());

    return fileContents;
};



std::uint32_t GenerateTexture(const std::wstring_view& texturePath)
{
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken = 0;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    Gdiplus::Bitmap* image = new Gdiplus::Bitmap(texturePath.data());


    const std::size_t width = image->GetWidth();
    const std::size_t height = image->GetHeight();

    Gdiplus::Rect rc(0, 0, static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height));

    Gdiplus::BitmapData* bitmapData = new Gdiplus::BitmapData();

    image->RotateFlip(Gdiplus::RotateFlipType::Rotate180FlipX);

    image->LockBits(&rc, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, bitmapData);


    std::vector<std::uint32_t> pixels = std::vector<std::uint32_t>(width * height);

    for(std::size_t y = 0; y < height; y++)
    {
        memcpy(&pixels[y * width], reinterpret_cast<char*>(bitmapData->Scan0) + bitmapData->Stride * y, width * 4);

        for(int x = 0; x < width; x++)
        {
            std::uint32_t& pixel = pixels[y * width + x];
            pixel = (pixel & 0xff00ff00) | ((pixel & 0xff) << 16) | ((pixel & 0xff0000) >> 16);
        };
    };

    image->UnlockBits(bitmapData);

    delete bitmapData;
    delete image;

    Gdiplus::GdiplusShutdown(gdiplusToken);

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height), 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    return textureID;
};



int main()
{
    constexpr std::uint32_t initialWindowWidth = 800;
    constexpr std::uint32_t initialWindowHeight = 600;


    GLFWwindow* glfwWindow = InitializeGLFWWindow(initialWindowWidth, initialWindowHeight, "OpenGL-TextRenderer");

    SetupOpenGL();

    const std::uint32_t fontTexture = GenerateTexture(L"Resources\\Consolas13x24.bmp");


    std::uint32_t vao = 0;
    glCreateVertexArrays(1, &vao);


    constexpr float vertices[] =
    {
        // Bottom left
        -416.0f, -72.0f,  0.0f, 0.0f,
        // Bottom right
        416.0f,  -72.0f,  1.0f, 0.0f,
        // Top right
        416.0f,   72.0f,  1.0f, 1.0f,

        // Top right
        416.0f,   72.0f,  1.0f, 1.0f,
        // Top left
        -416.0f,  72.0f,  0.0f, 1.0f,
        // Bottom left
        -416.0f, -72.0f,  0.0f, 0.0f,
    };


    /*
    constexpr float vertices[] =
    {
        // Bottom left
        -1.0f, -1.0f,  0.0f, 0.0f,
        // Bottom right
        1.0f,  -1.0f,  1.0f, 0.0f,
        // Top right
        1.0f,   1.0f,  1.0f, 1.0f,

        // Top right
        1.0f,   1.0f,  1.0f, 1.0f,
        // Top left
        -1.0f,  1.0f,  0.0f, 1.0f,
        // Bottom left
        -1.0f, -1.0f,  0.0f, 0.0f,
    };
    */



    std::uint32_t vertexPositionsVBO = 0;
    glCreateBuffers(1, &vertexPositionsVBO);
    glNamedBufferData(vertexPositionsVBO, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexArrayVertexBuffer(vao, 0, vertexPositionsVBO, 0, sizeof(float) * 4);

    // Vertex position
    glVertexArrayAttribFormat(vao, 0, 2, GL_FLOAT, false, 0);
    glVertexArrayAttribBinding(vao, 0, 0);
    glEnableVertexArrayAttrib(vao, 0);

    // Texture coordinate
    glVertexArrayAttribFormat(vao, 1, 2, GL_FLOAT, false, sizeof(float) * 2);
    glVertexArrayAttribBinding(vao, 1, 0);
    glEnableVertexArrayAttrib(vao, 1);



    const std::uint32_t vertexShaderID = glCreateShader(GL_VERTEX_SHADER);

    std::string vertexShaderText = ReadAllText("Shaders\\VertexShader.glsl");

    const char* vertexShaderSource = vertexShaderText.data();

    const int vertexShaderSourceLength = static_cast<int>(vertexShaderText.size());

    glShaderSource(vertexShaderID, 1, &vertexShaderSource, &vertexShaderSourceLength);

    glCompileShader(vertexShaderID);

    int vertexShaderCompiledSuccesfully = 0;
    glGetShaderiv(vertexShaderID, GL_COMPILE_STATUS, &vertexShaderCompiledSuccesfully);

    if(vertexShaderCompiledSuccesfully != GL_TRUE)
    {
        int bufferLength = 0;
        glGetShaderiv(vertexShaderID, GL_INFO_LOG_LENGTH, &bufferLength);

        std::string error;
        error.resize(bufferLength);

        glGetShaderInfoLog(vertexShaderID, bufferLength, &bufferLength, error.data());

        std::cerr << "Vertex shader compilation error:\n" << error << "\n";

        __debugbreak();
    };



    const std::uint32_t fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    std::string fragmentShaderText = ReadAllText("Shaders\\FragmentShader.glsl");

    const char* fragmentShaderSource = fragmentShaderText.data();

    const int fragmentShaderSourceLength = static_cast<int>(fragmentShaderText.size());

    glShaderSource(fragmentShaderID, 1, &fragmentShaderSource, &fragmentShaderSourceLength);

    glCompileShader(fragmentShaderID);

    int fragmentShaderCompiledSuccesfully = 0;
    glGetShaderiv(fragmentShaderID, GL_COMPILE_STATUS, &fragmentShaderCompiledSuccesfully);

    if(fragmentShaderCompiledSuccesfully != GL_TRUE)
    {
        int bufferLength = 0;
        glGetShaderiv(fragmentShaderID, GL_INFO_LOG_LENGTH, &bufferLength);

        std::string error;
        error.resize(bufferLength);

        glGetShaderInfoLog(fragmentShaderID, bufferLength, &bufferLength, error.data());

        std::cerr << "Fragment shader compilation error:\n" << error << "\n";

        __debugbreak();
    };



    const std::uint32_t shaderProgramID = glCreateProgram();

    glAttachShader(shaderProgramID, vertexShaderID);
    glAttachShader(shaderProgramID, fragmentShaderID);

    glLinkProgram(shaderProgramID);


    int programLinkedSuccesfully = 0;
    glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &programLinkedSuccesfully);

    if(programLinkedSuccesfully != GL_TRUE)
    {
        int bufferLength = 0;
        glGetProgramiv(shaderProgramID, GL_INFO_LOG_LENGTH, &bufferLength);

        std::string error;
        error.resize(bufferLength);

        glGetProgramInfoLog(shaderProgramID, bufferLength, &bufferLength, error.data());

        std::cerr << "Program linking error:\n" << error << "\n";

        __debugbreak();
    };

    glDeleteShader(vertexShaderID);
    glDeleteShader(fragmentShaderID);


    float delta = 0.0f;

    while(glfwWindowShouldClose(glfwWindow) == false)
    {
        glfwPollEvents();

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(vao);
        glUseProgram(shaderProgramID);




        const int projectionLocation = glGetUniformLocation(shaderProgramID, "Projection");

        if(projectionLocation == -1)
            __debugbreak();


        const int transformLocation = glGetUniformLocation(shaderProgramID, "Transform");

        if(transformLocation == -1)
            __debugbreak();



        // Adjust projection according to window width and height
        const glm::mat4 projection = glm::ortho(-static_cast<float>(WindowWidth), static_cast<float>(WindowWidth), -static_cast<float>(WindowHeight), static_cast<float>(WindowHeight), -1.0f, 1.0f);


        // glm::mat4 transform = glm::translate(glm::mat4(1.0f), { static_cast<int>(WindowWidth), static_cast<int>(WindowHeight) +, 0.0f });


        glm::mat4 transform = glm::mat4(1.0f);
        //glm::mat4 transform = glm::translate(glm::mat4(1.0f), { (-WindowWidth + (416)) + xOffset, (WindowHeight - (72)) - yOffset, 0.0f });
        // transform = glm::rotate(transform, delta, { 0, 0, 1 });


        glUniformMatrix4fv(projectionLocation, 1, false, glm::value_ptr(projection));
        glUniformMatrix4fv(transformLocation, 1, false, glm::value_ptr(transform));


        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(glfwWindow);



        
        if(glfwGetKey(glfwWindow, GLFW_KEY_SPACE) == GLFW_PRESS)
        {
            delta += 0.0002f;
        }
        if(glfwGetKey(glfwWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        {
            delta -= 0.0002f;
        };

        if(delta > glm::pi<float>() * 2)
        {
            delta = 0.0f;
        };

    };
};