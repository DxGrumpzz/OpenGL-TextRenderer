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
#include <array>

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



class FontTexture
{
private:

    std::uint32_t _textureID = 0;

    std::uint32_t _textureWidth = 0;
    std::uint32_t _textureHeight = 0;

    std::uint32_t _glyphWidth = 0;
    std::uint32_t _glyphHeight = 0;

    std::uint32_t _columns = 0;
    std::uint32_t _rows = 0;

    std::uint32_t _vao = 0;

    std::array<float, 24> _textureVertices { 0 };
    std::array<float, 24> _glyphVertices { 0 };


public:

    FontTexture(const std::uint32_t glyphWidth,
                const std::uint32_t glyphHeight,
                const std::wstring_view& texturePath) :
        _glyphWidth (glyphWidth),
        _glyphHeight(glyphHeight)
    {
        _textureID = LoadTexture(texturePath);

        _columns = _textureWidth / glyphWidth;
        _rows = _textureHeight / glyphHeight;


        // const float textureWidthAsFloat = static_cast<float>(_textureWidth);
        // const float textureHeightAsFloat = static_cast<float>(_textureHeight);

        const float textureWidthAsFloat = static_cast<float>(_textureWidth);
        const float textureHeightAsFloat = static_cast<float>(_textureHeight);

        // Subtract 32 (The space character) from the selected character to the correct index
        const int glyphIndex = 'T' - ' ';

        // Convert 1D character to 2D.
        const int glpyhX = glyphIndex % _columns;
        const int glpyhY = glyphIndex / _columns;

        const float textureCoordinateLeft = static_cast<float>(glpyhX) / _columns;
        const float textureCoordinateBottom = static_cast<float>((2 - glpyhY)) / _rows;

        const float textureCoordinateRight = static_cast<float>(glpyhX + 1) / _columns;
        const float textureCoordinateTop = static_cast<float>((2 - glpyhY) + 1) / _rows;


        _textureVertices =
        {
            // Top left
            0.0f,  0.0f,                                textureCoordinateLeft, textureCoordinateTop,
            // Top right
            textureWidthAsFloat, 0.0f,                  textureCoordinateRight, textureCoordinateTop,
            // Bottom left
            0.0f, textureHeightAsFloat,                 textureCoordinateLeft, textureCoordinateBottom,

            // Top right
            textureWidthAsFloat, 0.0f,                  textureCoordinateRight, textureCoordinateTop,
            // Bottom right
            textureWidthAsFloat, textureHeightAsFloat,  textureCoordinateRight, textureCoordinateBottom,
            // Bottom left
            0.0f, textureHeightAsFloat,                 textureCoordinateLeft, textureCoordinateBottom,
        };


        const float glyphWidthAsFloat = static_cast<float>(glyphWidth);
        const float glyphHeightAsFloat = static_cast<float>(glyphHeight);


        _glyphVertices =
        {
            // Top left
            0.0f,  0.0f,                            0.0f, 1.0f,
            // Top right                            
            glyphWidthAsFloat, 0.0f,                1.0f, 1.0f,
            // Bottom left                          
            0.0f, glyphHeightAsFloat,               0.0f, 0.0f,

            // Top right                            
            glyphWidthAsFloat, 0.0f,                1.0f, 1.0f,
            // Bottom right
            glyphWidthAsFloat, glyphHeightAsFloat,  1.0f, 0.0f,
            // Bottom left
            0.0f, glyphHeightAsFloat,               0.0f, 0.0f,
        };



        glCreateVertexArrays(1, &_vao);
        glBindVertexArray(_vao);

        std::uint32_t vertexPositionsVBO = 0;
        glCreateBuffers(1, &vertexPositionsVBO);
        glNamedBufferData(vertexPositionsVBO, sizeof(_textureVertices), _textureVertices.data(), GL_STATIC_DRAW);
        glVertexArrayVertexBuffer(_vao, 0, vertexPositionsVBO, 0, sizeof(float) * 4);

        // Vertex position
        glVertexArrayAttribFormat(_vao, 0, 2, GL_FLOAT, false, 0);
        glVertexArrayAttribBinding(_vao, 0, 0);
        glEnableVertexArrayAttrib(_vao, 0);

        // Texture coordinate
        glVertexArrayAttribFormat(_vao, 1, 2, GL_FLOAT, false, sizeof(float) * 2);
        glVertexArrayAttribBinding(_vao, 1, 0);
        glEnableVertexArrayAttrib(_vao, 1);

    };


    void Bind(const std::uint32_t textureUnit = 0) const
    {
        glActiveTexture(GL_TEXTURE0 + textureUnit);
        glBindTexture(GL_TEXTURE_2D, _textureID);

        glBindVertexArray(_vao);
    };


    void Draw(const std::uint32_t shaderProgramID) const
    {
        glUseProgram(shaderProgramID);


        const int projectionLocation = glGetUniformLocation(shaderProgramID, "Projection");

        if(projectionLocation == -1)
            __debugbreak();


        const int transformLocation = glGetUniformLocation(shaderProgramID, "Transform");

        if(transformLocation == -1)
            __debugbreak();



        const glm::mat4 screenSpaceProjection = glm::ortho(0.0f, static_cast<float>(WindowWidth), static_cast<float>(WindowHeight), 0.0f, -1.0f, 1.0f);


        const glm::mat4 transform = glm::translate(glm::mat4(1.0f), { 100, 100, 0.0f });


        glUniformMatrix4fv(projectionLocation, 1, false, glm::value_ptr(screenSpaceProjection));
        glUniformMatrix4fv(transformLocation, 1, false, glm::value_ptr(transform));

        glDrawArrays(GL_TRIANGLES, 0, 6);
    };


    std::uint32_t LoadTexture(const std::wstring_view& texturePath)
    {
        // TODO: Make this texture loader more generic, in-case we may want to move to STBI or something

        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        ULONG_PTR gdiplusToken = 0;
        Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

        Gdiplus::Bitmap* image = new Gdiplus::Bitmap(texturePath.data());


        _textureWidth = image->GetWidth();
        _textureHeight = image->GetHeight();

        Gdiplus::Rect rc(0, 0, static_cast<std::uint32_t>(_textureWidth), static_cast<std::uint32_t>(_textureHeight));

        Gdiplus::BitmapData* bitmapData = new Gdiplus::BitmapData();

        image->RotateFlip(Gdiplus::RotateFlipType::Rotate180FlipX);

        image->LockBits(&rc, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, bitmapData);


        std::vector<std::uint32_t> pixels = std::vector<std::uint32_t>(_textureWidth * _textureHeight);

        for(std::size_t y = 0; y < _textureHeight; y++)
        {
            memcpy(&pixels[y * _textureWidth], reinterpret_cast<char*>(bitmapData->Scan0) + bitmapData->Stride * y, _textureWidth * 4);

            for(std::size_t x = 0; x < _textureWidth; x++)
            {
                std::uint32_t& pixel = pixels[y * _textureWidth + x];
                pixel = (pixel & 0xff00ff00) | ((pixel & 0xff) << 16) | ((pixel & 0xff0000) >> 16);
            };
        };

        image->UnlockBits(bitmapData);

        delete bitmapData;
        delete image;

        Gdiplus::GdiplusShutdown(gdiplusToken);

        std::uint32_t textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<std::uint32_t>(_textureWidth), static_cast<std::uint32_t>(_textureHeight), 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

        return textureID;
    };
};



int main()
{
    constexpr std::uint32_t initialWindowWidth = 800;
    constexpr std::uint32_t initialWindowHeight = 600;


    GLFWwindow* glfwWindow = InitializeGLFWWindow(initialWindowWidth, initialWindowHeight, "OpenGL-TextRenderer");

    SetupOpenGL();


    const FontTexture fontTexture = FontTexture(13, 24, L"Resources\\Consolas13x24.bmp");



    #pragma region Shader program initialization

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

    #pragma endregion



    while(glfwWindowShouldClose(glfwWindow) == false)
    {
        glfwPollEvents();

        //glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);


        fontTexture.Bind(0);

        fontTexture.Draw(shaderProgramID);

        glfwSwapBuffers(glfwWindow);

    };
};