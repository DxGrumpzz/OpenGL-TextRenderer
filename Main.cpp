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

#include "GLUtils/ShaderProgram.hpp"


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


struct SSBOElement
{
public:

    std::size_t Count = 0;

    std::size_t Offset = 0;


public:
    SSBOElement(const std::size_t offset, const std::size_t count = 1) :
        Count(count),
        Offset(offset)
    {
    };

};


class ShaderStorageBuffer
{

    friend class FontSprite;

private:

    std::unordered_map<std::string, SSBOElement> _ssboElements { };

    mutable std::uint32_t _bufferID = 0;

    std::uint32_t _bufferBindingIndex = 0;

    mutable std::size_t _sizeInBytes = 0;

private:

    ShaderStorageBuffer()
    {
    };


public:


    ShaderStorageBuffer(const std::string_view& ssboName, const ShaderProgram& shaderProgram, const std::size_t sizeInBytes, std::uint32_t bufferBindingIndex = 0) :
        _bufferBindingIndex(bufferBindingIndex),
        _sizeInBytes(sizeInBytes)
    {
        const bool queryResult = QuerySSBOData(ssboName, shaderProgram);

        if(queryResult == false)
            return;

        glCreateBuffers(1, &_bufferID);
        glNamedBufferData(_bufferID, sizeInBytes, nullptr, GL_DYNAMIC_COPY);

        Bind();

    };

    ShaderStorageBuffer(const ShaderStorageBuffer& copy) = delete;

    ShaderStorageBuffer(ShaderStorageBuffer&& copy) noexcept :
        _ssboElements(std::exchange(copy._ssboElements, {})),
        _bufferID(std::exchange(copy._bufferID, 0)),
        _bufferBindingIndex(std::exchange(copy._bufferBindingIndex, 0)),
        _sizeInBytes(std::exchange(copy._sizeInBytes, 0))

    {

    };


    ~ShaderStorageBuffer()
    {
        glDeleteBuffers(1, &_bufferID);
    };


public:

    void Bind() const
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, _bufferID);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, _bufferBindingIndex, _bufferID);
    };


    template<typename T>
    void SetValue(const std::string_view& name, const T& value) const
    {
        const auto findResult = _ssboElements.find(name.data());

        wt::Assert(findResult != _ssboElements.end(), [&]()
        {
            return std::string("No such variable name \"").append(name).append("\"");
        });

        Bind();

        const SSBOElement& ssboElement = findResult->second;

        const std::size_t& offset = ssboElement.Offset;

        glNamedBufferSubData(_bufferID, offset, sizeof(T), &value);
    };


    void Reallocate(const std::size_t newSizeInBytes) const
    {
        // Ensure that this buffer is bound as an SSBO
        Bind();

        std::uint32_t newBufferID = 0;

        glCreateBuffers(1, &newBufferID);
        glNamedBufferData(newBufferID, newSizeInBytes, nullptr, GL_DYNAMIC_COPY);

        glCopyNamedBufferSubData(_bufferID, newBufferID, 0, 0, _sizeInBytes);

        glDeleteBuffers(1, &_bufferID);

        _bufferID = newBufferID;
        _sizeInBytes = newSizeInBytes;

        // Re-bind the new buffer as an SSBO
        Bind();
    };


public:

    std::uint32_t GetBufferID() const 
    {
        return _bufferID;
    };


public:

    ShaderStorageBuffer& operator = (const ShaderStorageBuffer& copy) = delete;

    ShaderStorageBuffer& operator = (ShaderStorageBuffer&& copy) noexcept
    {
        _ssboElements = std::exchange(copy._ssboElements, {});
        _bufferID = std::exchange(copy._bufferID, 0);
        _bufferBindingIndex = std::exchange(copy._bufferBindingIndex, 0);
        _sizeInBytes = std::exchange(copy._sizeInBytes, 0);

        return *this;
    };

private:

    bool QuerySSBOData(const std::string_view& ssboName, const ShaderProgram& shaderProgram)
    {
        // TODO: Add error checking

        // Get SSBO index
        GLint ssboIndex = glGetProgramResourceIndex(shaderProgram.GetProgramID(), GL_SHADER_STORAGE_BLOCK, ssboName.data());

        const bool assertResult = wt::Assert(ssboIndex != -1, [&]()
        {
            return std::string("No such SSBO \"").append(ssboName).append("\" ").append("on program \"").append(std::to_string(shaderProgram.GetProgramID()).append("\""));
        });

        if(assertResult == false)
            return false;

        // Get the longest variable name in the SSBO
        int ssboVariableMaxLength = 0;
        glGetProgramInterfaceiv(shaderProgram.GetProgramID(), GL_BUFFER_VARIABLE, GL_MAX_NAME_LENGTH, &ssboVariableMaxLength);


        // Get number of active SSBO variables
        static constexpr GLenum numberOfActiveVariablesProperty = GL_NUM_ACTIVE_VARIABLES;
        GLint numberOfVariables = 0;

        glGetProgramResourceiv(shaderProgram.GetProgramID(), GL_SHADER_STORAGE_BLOCK, ssboIndex, 1, &numberOfActiveVariablesProperty, 1, nullptr, &numberOfVariables);


        // Get SSBO variable indices
        static constexpr  GLenum activeVariablesProperty = GL_ACTIVE_VARIABLES;
        std::vector<GLint> variableIndices = std::vector<GLint> (numberOfVariables);

        _ssboElements.reserve(numberOfVariables);

        glGetProgramResourceiv(shaderProgram.GetProgramID(), GL_SHADER_STORAGE_BLOCK, ssboIndex, 1, &activeVariablesProperty, numberOfVariables, nullptr, variableIndices.data());


        // Get SSBO variable offsets
        static constexpr GLenum offsetProperty = GL_OFFSET;

        static constexpr GLenum arraySizeProperty = GL_ARRAY_SIZE;

        static constexpr GLenum nameLengthProperty = GL_NAME_LENGTH;


        for(std::size_t i = 0; i < variableIndices.size(); i++)
        {
            // Get SSBO variable array size
            int arraySize = 0;
            glGetProgramResourceiv(shaderProgram.GetProgramID(), GL_BUFFER_VARIABLE, variableIndices[i], 1, &arraySizeProperty, sizeof(arraySize), nullptr, &arraySize);

            int nameBufferLength = 0;
            glGetProgramResourceiv(shaderProgram.GetProgramID(), GL_BUFFER_VARIABLE, variableIndices[i], 1, &nameLengthProperty, sizeof(nameBufferLength), nullptr, &nameBufferLength);

            // SSBO variable name
            std::string name;
            name.resize(static_cast<std::size_t>(nameBufferLength) - 1);

            glGetProgramResourceName(shaderProgram.GetProgramID(), GL_BUFFER_VARIABLE, variableIndices[i], nameBufferLength, nullptr, name.data());


            GLint offset = 0;

            // SSBO variable offset
            glGetProgramResourceiv(shaderProgram.GetProgramID(), GL_BUFFER_VARIABLE, variableIndices[i], 1, &offsetProperty, sizeof(GLint), nullptr, &offset);

            // If the variable is an array..
            if(arraySize == 0 || arraySize != 1)
            {
                const std::size_t variableNameEnd = name.find('[');

                _ssboElements.insert(std::make_pair(std::string(name.cbegin(), name.cbegin() + variableNameEnd), SSBOElement(offset, arraySize)));
            }
            else
            {
                const std::size_t variableNameEnd = name.find('[');
                if(variableNameEnd != name.npos)
                {
                    _ssboElements.insert(std::make_pair(std::string(name.cbegin(), name.cbegin() + variableNameEnd), SSBOElement(offset, arraySize)));
                }
                else
                    _ssboElements.insert(std::make_pair(std::string(name.cbegin(), name.cend()), SSBOElement(offset, arraySize)));
            };
        };


        return true;
    };

};



/// <summary>
/// Draw text from a textue
/// </summary>
class FontSprite
{

private:

    /// <summary>
    /// The ID of the loaded texture
    /// </summary>
    std::uint32_t _textureID = 0;


    /// <summary>
    /// The total height of the font sprite
    /// </summary>
    std::uint32_t _fontSpriteWidth = 0;

    /// <summary>
    /// The total height of the font sprite
    /// </summary>
    std::uint32_t _fontSpriteHeight = 0;


    /// <summary>
    /// The width of a single glyph 
    /// </summary>
    std::uint32_t _glyphWidth = 0;

    /// <summary>
    /// The height of a single glyph 
    /// </summary>
    std::uint32_t _glyphHeight = 0;


    /// <summary>
    /// The number of character columns present in the font sprite
    /// </summary>
    std::uint32_t _columns = 0;

    /// <summary>
    /// The number of character rows present in the font sprite
    /// </summary>
    std::uint32_t _rows = 0;



    std::uint32_t _vao = 0;

    std::uint32_t _glyphVertexPositionsVBO = 0;

    std::reference_wrapper<const ShaderProgram> _shaderProgram;

    ShaderStorageBuffer _inputSSBO;


    /// <summary>
    /// Character capacity, how many character we can write before we need to reallocate the buffer
    /// </summary>
    mutable std::uint32_t _capacity = 0;


public:

    FontSprite(const std::uint32_t glyphWidth,
               const std::uint32_t glyphHeight,
               const ShaderProgram& shaderProgram,
               const std::wstring_view& texturePath,
               const std::uint32_t capacity = 32) :
        _glyphWidth (glyphWidth),
        _glyphHeight(glyphHeight),
        _shaderProgram(shaderProgram),
        _capacity(capacity)
    {
        _textureID = LoadTexture(texturePath);

        _columns = _fontSpriteWidth / glyphWidth;
        _rows = _fontSpriteHeight / glyphHeight;


        const float glyphWidthAsFloat = static_cast<float>(glyphWidth);
        const float glyphHeightAsFloat = static_cast<float>(glyphHeight);

        std::array<float, 12> glyphVertices =
        {
            // Top left
            0.0f,  0.0f,
            // Top right                           
            glyphWidthAsFloat, 0.0f,
            // Bottom left                         
            0.0f, glyphHeightAsFloat,

            // Top right                           
            glyphWidthAsFloat, 0.0f,
            // Bottom right
            glyphWidthAsFloat, glyphHeightAsFloat,
            // Bottom left
            0.0f, glyphHeightAsFloat,
        };



        glCreateVertexArrays(1, &_vao);
        glBindVertexArray(_vao);

        // Glyph vertex positions
        glCreateBuffers(1, &_glyphVertexPositionsVBO);
        glNamedBufferData(_glyphVertexPositionsVBO, sizeof(glyphVertices), glyphVertices.data(), GL_STATIC_DRAW);
        glVertexArrayVertexBuffer(_vao, 0, _glyphVertexPositionsVBO, 0, sizeof(float) * 2);

        glVertexArrayAttribFormat(_vao, 0, 2, GL_FLOAT, false, 0);
        glVertexArrayAttribBinding(_vao, 0, 0);
        glEnableVertexArrayAttrib(_vao, 0);


        // TODO: Find a way to compute SSBO size automatically
        const std::size_t inputSize = (sizeof(std::uint32_t) * 4) + (sizeof(glm::vec4) * 2) + (sizeof(std::uint32_t) * (_capacity));
        _inputSSBO = ShaderStorageBuffer("Input", shaderProgram, inputSize);

        _inputSSBO.SetValue("GlyphWidth", glyphWidth);
        _inputSSBO.SetValue("GlyphHeight", glyphHeight);

        _inputSSBO.SetValue("TextureWidth", _fontSpriteWidth);
        _inputSSBO.SetValue("TextureHeight", _fontSpriteHeight);

        constexpr glm::vec4 chromaKey = { 1.0f, 1.0f, 1.0f, 1.0f };
        _inputSSBO.SetValue("ChromaKey", chromaKey);

        constexpr glm::vec4 textColour = { 0.0f, 0.0f, 0.0f, 1.0f };
        _inputSSBO.SetValue("TextColour", textColour);

        // TODO: Refactor
        // TODO: OPtimization, cache the projection matrix and transform
    };


    ~FontSprite()
    {
        glDeleteBuffers(1, &_glyphVertexPositionsVBO);

        glDeleteTextures(1, &_textureID);

        glDeleteVertexArrays(1, &_vao);
    };


public:

    void Bind(const std::uint32_t textureUnit = 0) const
    {
        _shaderProgram.get().Bind();

        glActiveTexture(GL_TEXTURE0 + textureUnit);
        glBindTexture(GL_TEXTURE_2D, _textureID);

        glBindVertexArray(_vao);

        glBindBuffer(GL_ARRAY_BUFFER, _glyphVertexPositionsVBO);

        _inputSSBO.Bind();
    };


    void Draw(const std::string& text, const glm::vec4& textColour = { 0.0f, 0.0f, 0.0f, 1.0f }) const
    {
        if(text.empty() == true)
            return;

        // Allocate buffer memory if necessary
        if(text.size() > _capacity)
        {
            const std::size_t inputSize = (sizeof(std::uint32_t) * 4) + (sizeof(glm::vec4) * 2) + (sizeof(std::uint32_t) * (text.size() + (_capacity / 2)));
            _inputSSBO.Reallocate(inputSize);
        };



        // Calculate projection and transform
        const glm::mat4 screenSpaceProjection = glm::ortho(0.0f, static_cast<float>(WindowWidth), static_cast<float>(WindowHeight), 0.0f, -1.0f, 1.0f);
        const glm::mat4 transform = glm::translate(glm::mat4(1.0f), { 100, 100, 0.0f });

        // Update uniforms
        _shaderProgram.get().SetMatrix4("Projection", screenSpaceProjection);
        _shaderProgram.get().SetMatrix4("TextTransform", transform);


        // Set text colour
        _inputSSBO.SetValue("TextColour", textColour);


        // Copy the texts' character to the SSBO
        for(std::size_t index = 0;
            const auto & character : text)
        {
            const std::uint32_t& characterAsInt = character;

            glNamedBufferSubData(_inputSSBO.GetBufferID(), (sizeof(std::int32_t) * 4) + (sizeof(glm::vec4) * 2) + (sizeof(std::int32_t) * index), sizeof(characterAsInt), &characterAsInt);

            index++;
        };


        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, static_cast<std::int32_t>(text.size()));
    };


private:

    std::uint32_t LoadTexture(const std::wstring_view& texturePath)
    {
        // TODO: Make this texture loader more generic, in-case we may want to move to STBI or something

        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        ULONG_PTR gdiplusToken = 0;
        Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

        Gdiplus::Bitmap* image = new Gdiplus::Bitmap(texturePath.data());


        _fontSpriteWidth = image->GetWidth();
        _fontSpriteHeight = image->GetHeight();

        Gdiplus::Rect rc(0, 0, static_cast<int>(_fontSpriteWidth), static_cast<int>(_fontSpriteHeight));

        Gdiplus::BitmapData* bitmapData = new Gdiplus::BitmapData();

        image->RotateFlip(Gdiplus::RotateFlipType::Rotate180FlipX);

        image->LockBits(&rc, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, bitmapData);


        std::vector<std::uint32_t> pixels = std::vector<std::uint32_t>(static_cast<std::size_t>(_fontSpriteWidth) * _fontSpriteHeight);

        for(std::size_t y = 0; y < _fontSpriteHeight; y++)
        {
            memcpy(&pixels[y * _fontSpriteWidth], reinterpret_cast<char*>(bitmapData->Scan0) + static_cast<std::size_t>(bitmapData->Stride) * y, static_cast<std::size_t>(_fontSpriteWidth) * 4);

            for(std::size_t x = 0; x < _fontSpriteWidth; x++)
            {
                std::uint32_t& pixel = pixels[y * _fontSpriteWidth + x];
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

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<int>(_fontSpriteWidth), static_cast<int>(_fontSpriteHeight), 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

        return textureID;
    };


    std::uint32_t AllocateAndBindBuffer(const std::uint32_t characterCapacity,
                                        const std::uint32_t textureWidth, const std::uint32_t textureHeight,
                                        const std::uint32_t glyphWidth, const std::uint32_t glyphHeight) const
    {
        if(characterCapacity > _capacity)
            _capacity = characterCapacity;


        std::uint32_t ssbo = 0;


        // Texture coordinates SSBO
        glCreateBuffers(1, &ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);

        // Allocate SSBO memory for 2 32 bit ints, and 'n' of texture coordinate arrays
        glNamedBufferData(ssbo, (sizeof(std::uint32_t) * 4) + (sizeof(glm::vec4) * 2) + (sizeof(std::uint32_t) * _capacity), nullptr, GL_DYNAMIC_DRAW);

        std::size_t bytesWritten = 0;

        // Glyph width
        bytesWritten += NamedBufferSubData(ssbo, 0, sizeof(glyphWidth), &glyphWidth);

        // Glyph height
        bytesWritten += NamedBufferSubData(ssbo, bytesWritten, sizeof(glyphHeight), &glyphHeight);

        // Texture width
        bytesWritten += NamedBufferSubData(ssbo, bytesWritten, sizeof(textureWidth), &textureWidth);

        // Texture height                                      
        bytesWritten += NamedBufferSubData(ssbo, bytesWritten, sizeof(textureHeight), &textureHeight);


        constexpr glm::vec4 chromaKey = { 1.0f, 1.0f, 1.0f, 1.0f };

        // Chroma key                                      
        bytesWritten += NamedBufferSubData(ssbo, bytesWritten, sizeof(glm::vec4), &chromaKey);


        constexpr glm::vec4 textColour = { 0.0f, 0.0f, 0.0f, 1.0f };

        // Text colour
        bytesWritten += NamedBufferSubData(ssbo, bytesWritten, sizeof(glm::vec4), &textColour);


        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

        return ssbo;
    };


    std::size_t NamedBufferSubData(std::uint32_t buffer, std::int64_t offset, std::int64_t size, const void* data) const
    {
        glNamedBufferSubData(buffer, offset, size, data);

        return size;
    };

};



int main()
{
    constexpr std::uint32_t initialWindowWidth = 800;
    constexpr std::uint32_t initialWindowHeight = 600;

    GLFWwindow* glfwWindow = InitializeGLFWWindow(initialWindowWidth, initialWindowHeight, "OpenGL-TextRenderer");


    SetupOpenGL();



    const ShaderProgram shaderProgram = ShaderProgram("Shaders\\FontSpriteVertexShader.glsl", "Shaders\\FontSpriteFragmentShader.glsl");

    const FontSprite fontSprite = FontSprite(13, 24, shaderProgram, L"Resources\\Consolas13x24.bmp");


    static std::string textToDraw = "Type anything!";


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