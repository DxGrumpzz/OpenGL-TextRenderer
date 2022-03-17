#define WIN32_LEAN_AND_MEN

#include <glad/glad.h>
#include <Windows.h>
#include <string_view>

#include "ShaderProgram.hpp"
#include "FontSprite.hpp"
#include "ShaderStorageBuffer.hpp"
#include "WindowsUtilities.hpp"


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



enum class DataType
{
    UInt32,

    Vec4f,

    Mat4f,

    Array,

    Struct,

    None,
};



struct Element
{

public:

    // Only applied to structs and arrays
    std::vector<Element> Elements;


    DataType Type = DataType::None;
    
    std::size_t SizeInBytes = 0;

    std::size_t Offset = 0;


public:

    Element(DataType type) :
        Type(type)
    {

        switch(type)
        {
            case DataType::UInt32:
                SizeInBytes = sizeof(std::uint32_t);
                break;

            case DataType::Vec4f:
                SizeInBytes = sizeof(glm::vec4);
                break;

            case DataType::Mat4f:
                SizeInBytes = sizeof(glm::mat4);
                break;
        };

    };

public:

    void Add()
    {
        wt::Assert((Type == DataType::Struct || Type == DataType::Array) && Type == DataType::None,
                   []()
        {
            return std::string("Trying to add to non struct or array type");
        });




    };

};


class Layout
{
public:

    std::unordered_map<std::string, Element> _layoutElements;

    Element* _lastAddedElement = nullptr;



public:


    void Add(DataType dataType, const std::string_view& name)
    {
        wt::Assert(dataType != DataType::None, []()
        {
            return std::string("Invalid data type");
        });


        const auto& insertResult = _layoutElements.insert(std::make_pair(name, Element { dataType }));

        Element& newElement = (_layoutElements.find(name.data()))->second;


        if(_layoutElements.size() == 1)
        {
            _lastAddedElement = &newElement;
            return;
        };


        if(dataType == DataType::Struct)
        {
            // const Element& previousElement = (_layoutElements.end()- 2);

            std::size_t offset = 0;
            if(_lastAddedElement != nullptr)
            {
                offset = (_lastAddedElement->Offset + _lastAddedElement->SizeInBytes);
            };



            int _ = 0;
        }
        else
        {
            const std::size_t rawOffset = _lastAddedElement->Offset + _lastAddedElement->SizeInBytes;

            // If the element crosses over a 16 byte boundary
            const bool crossesBoundary = CrossesBoundary(rawOffset, newElement.SizeInBytes);

            if(crossesBoundary == false)
            {
                newElement.Offset = rawOffset;
            }
            else
            {
                const std::size_t actualOffset = rawOffset + (16u - rawOffset % 16u) % 16u;

                newElement.Offset = actualOffset;
            };
        };


        _lastAddedElement = &newElement;
    };


    Element& Get(const std::string_view& name)
    {
        auto findResult = _layoutElements.find(name.data());

        return findResult->second;
    };

    /// <summary>
    /// Check if an element crosses a 16-byte boundary
    /// </summary>
    /// <param name="offset"> </param>
    /// <param name="size"></param>
    /// <returns></returns>
    constexpr bool CrossesBoundary(std::size_t offset, std::size_t sizeInBytes)
    {
        const std::size_t end = offset + sizeInBytes;

        const std::size_t pageStart = offset / 16u;
        const std::size_t pageEnd = end / 16u;

        return ((pageStart != pageEnd) && (end % 16 != 0u)) || (sizeInBytes > 16u);
    };

};



void SSBOTest()
{
    Layout layout;

    layout.Add(DataType::UInt32, "Uint_off_0");
    layout.Add(DataType::UInt32, "Uint_off_4");
    layout.Add(DataType::UInt32, "Uint_off_8");
    layout.Add(DataType::UInt32, "Uint_off_12");
    layout.Add(DataType::Vec4f,  "Vec3f_off_16");
    layout.Add(DataType::Mat4f,  "Mat4f_off_32");
    layout.Add(DataType::UInt32, "Uint_off_96");
    layout.Add(DataType::Struct, "Test");

    // Element& structTest = layout.Get("Test");

    if(layout.Get("Uint_off_0").Offset != 0)
        __debugbreak();
    if(layout.Get("Uint_off_4").Offset != 4)
        __debugbreak();
    if(layout.Get("Uint_off_8").Offset != 8)
        __debugbreak();
    if(layout.Get("Uint_off_12").Offset != 12)
        __debugbreak();
    if(layout.Get("Vec3f_off_16").Offset != 16)
        __debugbreak();
    if(layout.Get("Mat4f_off_32").Offset != 32)
        __debugbreak();
    if(layout.Get("Uint_off_96").Offset != 96)
        __debugbreak();

    int _ = 0;

    // layout.Add(DataType::UInt32, "Uint_off_4");


    /*
    std::vector<Element> elements;

    elements.emplace_back("GlyphWidth", sizeof(std::uint32_t));

    elements.emplace_back("GlyphHeight", sizeof(std::uint32_t));

    elements.emplace_back("TextureWidth", sizeof(std::uint32_t));

    elements.emplace_back("TextureHeight", sizeof(std::uint32_t));

    elements.emplace_back("ChromaKey", sizeof(glm::vec4));

    elements.emplace_back("TextColour", sizeof(glm::vec4));

    elements.emplace_back("Characters", sizeof(std::uint32_t));
    */


    // elements.emplace_back("GlyphWidth", sizeof(std::uint32_t));
    // elements.emplace_back("GlyphHeight", sizeof(std::uint32_t));

    // elements.emplace_back("TextureWidth", sizeof(std::uint32_t));
    // elements.emplace_back("TextureHeight", sizeof(std::uint32_t));

    // elements.emplace_back("ChromaKey", sizeof(glm::vec4));
    // elements.emplace_back("TextColour", sizeof(glm::vec4));

    // static constexpr std::uint32_t characters = 8;
    // elements.emplace_back("Characters", sizeof(uint32_t), characters);

    /*
    const auto largestElementIterator = std::max_element(elements.cbegin(), elements.cend(),
                                                         [](const Element& element1, const Element& element2)
    {
        const bool result = element1.SizeInBytes < element2.SizeInBytes;

        return result;
    });

    const std::size_t largestElementSize = largestElementIterator->SizeInBytes;


    std::size_t offset = 0;

    // Calculate offsets without padding
    for(Element& element : elements)
    {
        element.Offset = offset;

        offset = element.Offset + element.SizeInBytes;
    };




    for(std::size_t i = 0; i < elements.size() - 1; ++i)
    {
        Element& currentElement = elements[i];
        Element& nextElement = elements[i + 1];

        const std::size_t offsetDifference = nextElement.Offset - currentElement.Offset;

        if(offsetDifference < largestElementSize)
        {
            const std::size_t padding = largestElementSize - offsetDifference;


            // nextElement.Offset = currentElement.SizeInBytes + currentElement.Padding + currentElement.Offset;


            std::size_t _offset = nextElement.Offset;

            for(std::size_t j = i + 1; j < elements.size(); ++j)
            {
                Element& element = elements[j];

                element.Offset = _offset;

                _offset = element.Offset + element.SizeInBytes;
            };

            int _ = 0;
        };

        int _ = 0;
    };

    int _ = 0;

    */

    /*
    std::size_t previousComputedTotalBytes = 0;

    std::int64_t byteCount = largestElementSize;

    for(std::size_t i = 0; i < elements.size() - 1; i++)
    {
        const Element& element = elements[i];

        const std::size_t computedBytes = (element.SizeInBytes * element.Count);
        const std::size_t computedTotalBytes = previousComputedTotalBytes + (element.SizeInBytes * element.Count);

        byteCount -= computedBytes;

        if(byteCount == 0)
        {
            byteCount = largestElementSize;
        }
        else if(byteCount < 0)
        {
            const std::size_t padding = computedBytes - (computedTotalBytes - computedBytes);

            int _ = 0;
        };

        previousComputedTotalBytes = computedTotalBytes;
    };
    */

};



int main()
{
    constexpr std::size_t offset = 16;

    // constexpr std::size_t actualOffset1 = (16u - (offset % 16u));
    // constexpr std::size_t actualOffset2 = actualOffset1 % 16u;
    // constexpr std::size_t actualOffset3 = offset + actualOffset2;
        
    constexpr std::size_t actualOffset1 = (16u - (offset % 16u));
    constexpr std::size_t actualOffset2 = actualOffset1 % 16u;
    constexpr std::size_t actualOffset3 = offset + actualOffset2;


    SSBOTest();

    constexpr std::uint32_t initialWindowWidth = 800;
    constexpr std::uint32_t initialWindowHeight = 600;

    GLFWwindow* glfwWindow = InitializeGLFWWindow(initialWindowWidth, initialWindowHeight, "OpenGL-TextRenderer");


    SetupOpenGL();

    const ShaderProgram shaderProgram = ShaderProgram("Shaders\\FontSpriteVertexShader.glsl", "Shaders\\FontSpriteFragmentShader.glsl");

    FontSprite fontSprite = FontSprite(13, 24, shaderProgram, L"Resources\\Consolas13x24.bmp");


    // Calculate projection and transform
    fontSprite.ScreenSpaceProjection = glm::ortho(0.0f, static_cast<float>(WindowWidth), static_cast<float>(WindowHeight), 0.0f, -1.0f, 1.0f);

    fontSprite.Transform = glm::translate(glm::mat4(1.0f), { 100, 100, 0.0f });


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