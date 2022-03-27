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

    Vec2f,

    Vec4f,

    Mat4f,

    Array,

    Struct,

    None,
};


constexpr std::size_t DataTypeSizeInBytes(DataType type)
{
    switch(type)
    {
        case DataType::UInt32:
            return sizeof(std::uint32_t);
            break;

        case DataType::Vec2f:
            return sizeof(glm::vec2);
            break;

        case DataType::Vec4f:
            return sizeof(glm::vec4);
            break;

        case DataType::Mat4f:
            return sizeof(glm::mat4);
            break;


        case DataType::Array:
        case DataType::Struct:
            return 0;
            break;

        default:
        {
            wt::Assert(false, "No such type");
            __debugbreak();
        };
    };
};



class Layout;

class Element
{
    friend class Layout;

private:

    std::vector<std::pair<std::string, Element>> _structElements;

    std::vector<Element> _arrayElements;

    DataType _elementType = DataType::None;

    std::size_t _sizeInBytes = 0;

    std::size_t _offset = 0;

    DataType _arrayElementType = DataType::None;

    std::size_t _arrayElementCount = static_cast<std::size_t>(-1);



public:

    Element(DataType type) :
        _elementType(type),
        _sizeInBytes(DataTypeSizeInBytes(type))
    {
    };


public:

    Element& GetAtIndex(std::size_t index)
    {
        wt::Assert(_elementType == DataType::Array, []()
        {
            return "Cannot index into non-array element";
        });

        wt::Assert(_arrayElements.empty() == false, []()
        {
            return "Element array is empty";
        });

        wt::Assert(index >= 0 && index < _arrayElements.size(),
                   []()
        {
            return "Invalid index";
        });

        Element& element = _arrayElements[index];

        return element;
    };

    const Element& GetAtIndex(std::size_t index) const
    {
        return GetAtIndex(index);
    };


    Element* Get(const std::string_view& name)
    {
        wt::Assert(_elementType == DataType::Struct, [&]()
        {
            return "Attempting to retrieve struct member on non-struct element";
        });

        for(auto& [elementName, structElement] : _structElements)
        {
            if(elementName == name)
            {
                return &structElement;
            };
        };

        wt::Assert(false, [&]()
        {
            return std::string("No such element \"").append(name).append("\" was found");
        });

        return nullptr;
    };

    const Element* Get(const std::string_view& name) const
    {
        return Get(name);
    };


    void Add(DataType dataType, const std::string_view& name)
    {
        wt::Assert(name.empty() == false, []()
        {
            return "Invalid member name";
        });

        wt::Assert(_elementType == DataType::Struct, []()
        {
            return "Trying to add struct member to non-struct element";
        });

        wt::Assert(dataType != DataType::None, []()
        {
            return "Invalid data type";
        });

        wt::Assert([&]()
        {
            for(auto& [elementName, structElement] : _structElements)
            {
                if(elementName == name)
                {
                    return false;
                };
            };

            return true;
        },
                   [&]()
        {
            return std::string("Duplicate member name \"").append(name).append("\"");
        });


        _structElements.emplace_back(std::make_pair(name, Element(dataType)));
    };


    void SetArray(DataType arrayType, std::size_t elementCount)
    {
        wt::Assert(elementCount >= 1, []()
        {
            return "Invalid array size";
        });

        wt::Assert(arrayType != DataType::None, []()
        {
            return "Invalid data type";
        });

        _arrayElementType = arrayType;
        _arrayElementCount = elementCount;
    };

    Element& SetCustomArrayType(std::size_t elementCount)
    {
        SetArray(DataType::Struct, elementCount);

        return _arrayElements.emplace_back(_arrayElementType);
    };


public:

    std::size_t GetOffset() const
    {
        return _offset;
    };

};


class RawLayout
{
    friend class Layout;

private:

    std::vector<std::pair<std::string, Element>> _layoutElements;

public:

    Element& Add(DataType dataType, const std::string_view& name)
    {
        wt::Assert(name.empty() == false, []()
        {
            return "Invalid member name";
        });

        wt::Assert(dataType != DataType::None, []()
        {
            return "Invalid data type";
        });

        wt::Assert([&]()
        {
            for(auto& [elementName, structElement] : _layoutElements)
            {
                if(elementName == name)
                {
                    return false;
                };
            };

            return true;
        },
                   [&]()
        {
            return std::string("Duplicate member name \"").append(name).append("\"");
        });

        auto& insertResult = _layoutElements.emplace_back(std::make_pair(name, Element(dataType)));

        return insertResult.second;
    };

};


class Layout
{
private:

    std::unordered_map<std::string, Element> _layoutElements;


public:

    // TODO: Maybe add move constructor for "RawLayout&&"?
    Layout(RawLayout& rawLayout)
    {
        const auto layout = CreateLayout(rawLayout);
        _layoutElements.insert(layout.begin(), layout.end());
    };


public:

    Element* Get(const std::string_view& name)
    {
        wt::Assert(_layoutElements.empty() == false, [&]()
        {
            return std::string("Layout is empty");
        });

        auto findResult = _layoutElements.find(name.data());

        wt::Assert(findResult != _layoutElements.end(), [&]()
        {
            return std::string("No such element \"").append(name).append("\" was found");
        });


        return &findResult->second;
    };

    const Element* Get(const std::string_view& name) const
    {
        return Get(name);
    };


private:

    std::vector<std::pair<std::string, Element>> CreateLayout(RawLayout& rawLayout, std::size_t offset = 0) const
    {
        std::vector<std::pair<std::string, Element>> layoutResult;

        std::size_t currentOffset = offset;


        for(auto& [name, element] : rawLayout._layoutElements)
        {
            if(element._elementType == DataType::Array)
            {
                if(element._arrayElementType == DataType::Struct)
                {
                    RawLayout rawArrayStructLayout;

                    for(std::size_t i = 0; i < element._arrayElementCount; ++i)
                    {
                        Element& rawStructLayout = rawArrayStructLayout.Add(DataType::Struct, std::string("[").append(std::to_string(i)).append("]"));

                        for(auto& [name, structElement] : element._arrayElements[0]._structElements)
                        {
                            rawStructLayout.Add(structElement._elementType, name);
                        };
                    };

                    const auto arrayStructLayout = CreateLayout(rawArrayStructLayout, currentOffset);


                    element._arrayElements.clear();

                    for(auto& arrayStructElement : arrayStructLayout)
                    {
                        element._arrayElements.emplace_back(arrayStructElement.second);
                    };

                    const auto arrayStructLayoutEnd = std::prev((arrayStructLayout.cend()));
                    currentOffset = arrayStructLayoutEnd->second._offset + arrayStructLayoutEnd->second._sizeInBytes;
                }
                else
                {
                    const std::size_t arrayElementSizeInBytes = DataTypeSizeInBytes(element._arrayElementType);
                    element._offset = GetCorrectOffset(currentOffset, arrayElementSizeInBytes);
                    element._sizeInBytes = arrayElementSizeInBytes * element._arrayElementCount;

                    RawLayout rawArrayLayout;

                    for(std::size_t i = 0; i < element._arrayElementCount; ++i)
                    {
                        rawArrayLayout.Add(element._arrayElementType, std::string("[").append(std::to_string(i)).append("]"));
                    };

                    const auto arrayLayout = CreateLayout(rawArrayLayout, currentOffset);

                    for(auto& arrayLayoutElement : arrayLayout)
                    {
                        element._arrayElements.emplace_back(arrayLayoutElement.second);
                    };

                    const auto arrayLayoutEnd = std::prev((arrayLayout.cend()));
                    currentOffset = arrayLayoutEnd->second._offset + arrayElementSizeInBytes;
                };
            }
            else if(element._elementType == DataType::Struct)
            {
                RawLayout rawStructLayout;

                for(auto& [name, structElement] : element._structElements)
                {
                    rawStructLayout.Add(structElement._elementType, name);
                };

                // TODO: Find a way to optimize struct size 

                // Calculate struct size
                const auto structLayoutInitial = CreateLayout(rawStructLayout, 0);

                auto structLayoutEnd = std::prev(structLayoutInitial.end());

                const std::size_t structSize = structLayoutEnd->second._offset + structLayoutEnd->second._sizeInBytes;


                element._offset = GetCorrectOffset(currentOffset, structSize);
                currentOffset = element._offset;

                element._sizeInBytes = structSize;


                const auto structLayoutFinal = CreateLayout(rawStructLayout, currentOffset);

                element._structElements.clear();

                for(auto& structLayoutElement : structLayoutFinal)
                {
                    element._structElements.emplace_back(structLayoutElement);
                };

                structLayoutEnd = std::prev(structLayoutFinal.cend());
                currentOffset = structLayoutEnd->second._offset + structLayoutEnd->second._sizeInBytes;
            }
            else
            {
                element._offset = GetCorrectOffset(currentOffset, element._sizeInBytes);
                currentOffset = element._offset + element._sizeInBytes;
            };


            layoutResult.emplace_back(std::make_pair(name, element));
        };

        return layoutResult;
    };


    constexpr std::size_t GetCorrectOffset(std::size_t offset, std::size_t sizeInBytes) const
    {
        const bool crossesBoundary = CrossesBoundary(offset, sizeInBytes);

        if(crossesBoundary == true)
        {
            const std::size_t rawOffset = offset;

            const std::size_t actualOffset = CalculateOffset(rawOffset);

            const std::size_t padding = actualOffset - offset;

            return actualOffset;
        }
        else
        {
            return offset;
        };
    };

    constexpr std::size_t CalculateOffset(std::size_t rawOffset) const
    {
        const std::size_t correctedOffset = rawOffset + (16u - rawOffset % 16u) % 16u;

        return correctedOffset;
    };

    /// <summary>
    /// Check if an element crosses a 16-byte boundary
    /// </summary>
    /// <param name="offset"> </param>
    /// <param name="size"></param>
    /// <returns></returns>
    constexpr bool CrossesBoundary(std::size_t offset, std::size_t sizeInBytes) const
    {
        const std::size_t end = offset + sizeInBytes;

        const std::size_t pageStart = offset / 16u;
        const std::size_t pageEnd = end / 16u;

        return ((pageStart != pageEnd) && (end % 16 != 0u)) || (sizeInBytes > 16u);
    };

};



void SSBOTest()
{
    // Raw layout
    {
        // Uint32 - vec4 padding
        {
            RawLayout rawLayout;

            rawLayout.Add(DataType::UInt32, "Uint_off_0");
            rawLayout.Add(DataType::Vec4f, "Vec4_off_16");


            Layout layout = Layout (rawLayout);

            if(layout.Get("Uint_off_0")->GetOffset() != 0)
                __debugbreak();
            if(layout.Get("Vec4_off_16")->GetOffset() != 16)
                __debugbreak();
        };


        // Contigious elements
        {
            RawLayout rawLayout;

            rawLayout.Add(DataType::UInt32, "Uint_off_0");
            rawLayout.Add(DataType::UInt32, "Uint_off_4");
            rawLayout.Add(DataType::UInt32, "Uint_off_8");
            rawLayout.Add(DataType::UInt32, "Uint_off_12");
            rawLayout.Add(DataType::UInt32, "Uint_off_16");


            Layout layout = Layout (rawLayout);

            if(layout.Get("Uint_off_0")->GetOffset() != 0)
                __debugbreak();
            if(layout.Get("Uint_off_4")->GetOffset() != 4)
                __debugbreak();
            if(layout.Get("Uint_off_8")->GetOffset() != 8)
                __debugbreak();
            if(layout.Get("Uint_off_12")->GetOffset() != 12)
                __debugbreak();
            if(layout.Get("Uint_off_16")->GetOffset() != 16)
                __debugbreak();
            int _ = 0;
        };


        {
            RawLayout rawLayout;

            rawLayout.Add(DataType::Vec2f, "Vec2_off_0");
            rawLayout.Add(DataType::UInt32, "Uint_off_8");

            Layout layout = Layout (rawLayout);

            if(layout.Get("Vec2_off_0")->GetOffset() != 0)
                __debugbreak();
            if(layout.Get("Uint_off_8")->GetOffset() != 8)
                __debugbreak();
        };


        {
            RawLayout rawLayout;

            rawLayout.Add(DataType::Vec2f, "Vec2_off_0");
            rawLayout.Add(DataType::Vec2f, "Vec2_off_8");

            Layout layout = Layout (rawLayout);

            if(layout.Get("Vec2_off_0")->GetOffset() != 0)
                __debugbreak();
            if(layout.Get("Vec2_off_8")->GetOffset() != 8)
                __debugbreak();
        };



        // Array 
        {
            RawLayout rawLayout;

            Element& rawArrayElement = rawLayout.Add(DataType::Array, "Uint_off_0");

            rawArrayElement.SetArray(DataType::UInt32, 4);
            Layout layout = Layout(rawLayout);

            Element& arrayElement = *layout.Get("Uint_off_0");

            if(layout.Get("Uint_off_0")->GetOffset() != 0)
                __debugbreak();

            if(arrayElement.GetAtIndex(0).GetOffset() != 0)
                __debugbreak();
            if(arrayElement.GetAtIndex(1).GetOffset() != 4)
                __debugbreak();
            if(arrayElement.GetAtIndex(2).GetOffset() != 8)
                __debugbreak();
            if(arrayElement.GetAtIndex(3).GetOffset() != 12)
                __debugbreak();

        };


        // Array 2
        {
            RawLayout rawLayout;

            Element& rawArrayElement = rawLayout.Add(DataType::Array, "Uint_off_0");

            rawArrayElement.SetArray(DataType::UInt32, 3);

            rawLayout.Add(DataType::UInt32, "Uint_off_12");

            Layout layout = Layout(rawLayout);

            Element& arrayElement = *layout.Get("Uint_off_0");

            if(layout.Get("Uint_off_0")->GetOffset() != 0)
                __debugbreak();

            if(arrayElement.GetAtIndex(0).GetOffset() != 0)
                __debugbreak();
            if(arrayElement.GetAtIndex(1).GetOffset() != 4)
                __debugbreak();
            if(arrayElement.GetAtIndex(2).GetOffset() != 8)
                __debugbreak();

            if(layout.Get("Uint_off_12")->GetOffset() != 12)
                __debugbreak();
        };


        // Array with padding
        {
            RawLayout rawLayout;

            rawLayout.Add(DataType::UInt32, "Uint_off_0");

            Element& rawArrayElement = rawLayout.Add(DataType::Array, "Array_off_16");

            rawArrayElement.SetArray(DataType::Vec4f, 3);

            Layout layout = Layout(rawLayout);

            Element& arrayElement = *layout.Get("Array_off_16");


            if(layout.Get("Uint_off_0")->GetOffset() != 0)
                __debugbreak();

            if(layout.Get("Array_off_16")->GetOffset() != 16)
                __debugbreak();

            if(arrayElement.GetAtIndex(0).GetOffset() != 16)
                __debugbreak();
            if(arrayElement.GetAtIndex(1).GetOffset() != 32)
                __debugbreak();
            if(arrayElement.GetAtIndex(2).GetOffset() != 48)
                __debugbreak();

        };



        // Struct
        {
            RawLayout structLayout;

            structLayout.Add(DataType::UInt32, "Uint_off_0");

            Element& rawStructElement = structLayout.Add(DataType::Struct, "Test_off_16");

            rawStructElement.Add(DataType::UInt32, "Test_Uint_off_16");
            rawStructElement.Add(DataType::Vec4f, "Test_Vec4_off_32");

            structLayout.Add(DataType::UInt32, "Uint_off_48");

            Layout layout = Layout(structLayout);

            Element& structElement = *layout.Get("Test_off_16");


            if(layout.Get("Uint_off_0")->GetOffset() != 0)
                __debugbreak();

            if(layout.Get("Test_off_16")->GetOffset() != 16)
                __debugbreak();

            if(structElement.Get("Test_Uint_off_16")->GetOffset() != 16)
                __debugbreak();

            if(structElement.Get("Test_Vec4_off_32")->GetOffset() != 32)
                __debugbreak();

            if(layout.Get("Uint_off_48")->GetOffset() != 48)
                __debugbreak();
        };


        // Multiple structs
        {
            RawLayout structLayout;

            structLayout.Add(DataType::UInt32, "Uint_off_0");

            Element& rawStructElement = structLayout.Add(DataType::Struct, "Test_off_16");

            rawStructElement.Add(DataType::UInt32, "Test_Uint_off_16");
            rawStructElement.Add(DataType::Vec4f, "Test_Vec4_off_32");


            Element& rawStructElement2 = structLayout.Add(DataType::Struct, "Test2_off_48");

            rawStructElement2.Add(DataType::UInt32, "Test2_Uint_off_48");
            rawStructElement2.Add(DataType::Mat4f, "Test2_Mat4_off_64");


            Layout layout = Layout(structLayout);

            Element& structElement = *layout.Get("Test_off_16");
            Element& structElement2 = *layout.Get("Test2_off_48");


            if(layout.Get("Uint_off_0")->GetOffset() != 0)
                __debugbreak();

            if(layout.Get("Test_off_16")->GetOffset() != 16)
                __debugbreak();

            if(structElement.Get("Test_Uint_off_16")->GetOffset() != 16)
                __debugbreak();

            if(structElement.Get("Test_Vec4_off_32")->GetOffset() != 32)
                __debugbreak();

            if(layout.Get("Test2_off_48")->GetOffset() != 48)
                __debugbreak();

            if(structElement2.Get("Test2_Uint_off_48")->GetOffset() != 48)
                __debugbreak();

            if(structElement2.Get("Test2_Mat4_off_64")->GetOffset() != 64)
                __debugbreak();


            int _ = 0;

        };


        // Array of structs
        {
            RawLayout structArrayLayout;

            Element& rawArrayElement = structArrayLayout.Add(DataType::Array, "Test_off_0");

            Element& arrayType = rawArrayElement.SetCustomArrayType(5);

            arrayType.Add(DataType::UInt32, "Test_Uint_off_0");
            arrayType.Add(DataType::Vec4f, "Test_Vec4_off_16");

            structArrayLayout.Add(DataType::UInt32, "Uint_off_160");


            Layout layout = Layout(structArrayLayout);

            Element& structArray = *layout.Get("Test_off_0");

            if(structArray.GetOffset() != 0)
                __debugbreak();

            for(std::size_t i = 0; i < 5; ++i)
            {
                if(structArray.GetAtIndex(i).GetOffset() != i * 32)
                    __debugbreak();


                if(structArray.GetAtIndex(i).Get("Test_Uint_off_0")->GetOffset() != i * 32)
                    __debugbreak();

                if(structArray.GetAtIndex(i).Get("Test_Vec4_off_16")->GetOffset() != (i * 32) + 16)
                    __debugbreak();
            };

            if(layout.Get("Uint_off_160")->GetOffset() != 160)
                __debugbreak();

        };


        // Array of structs with padding
        {
            RawLayout structArrayLayout;

            structArrayLayout.Add(DataType::UInt32, "Uint_off_0");

            Element& rawArrayElement = structArrayLayout.Add(DataType::Array, "Test_off_16");

            Element& arrayType = rawArrayElement.SetCustomArrayType(5);

            arrayType.Add(DataType::UInt32, "Test_Uint_off_16");
            arrayType.Add(DataType::Vec4f, "Test_Vec4_off_32");

            structArrayLayout.Add(DataType::UInt32, "Uint_off_176");


            Layout layout = Layout(structArrayLayout);

            Element& structArray = *layout.Get("Test_off_16");

            if(structArray.GetOffset() != 0)
                __debugbreak();

            for(std::size_t i = 0; i < 5; ++i)
            {
                if(structArray.GetAtIndex(i).GetOffset() != (i * 32) + 16)
                    __debugbreak();


                if(structArray.GetAtIndex(i).Get("Test_Uint_off_16")->GetOffset() != (i * 32) + 16)
                    __debugbreak();

                if(structArray.GetAtIndex(i).Get("Test_Vec4_off_32")->GetOffset() != ((i * 32) + 16) + 16)
                    __debugbreak();
            };

            if(layout.Get("Uint_off_176")->GetOffset() != 176)
                __debugbreak();

        };


        // Struct test
        {
            RawLayout rawLayout;

            rawLayout.Add(DataType::UInt32, "Uint_off_0");

            Element& rawStructElement = rawLayout.Add(DataType::Struct, "Struct_off_4");

            rawStructElement.Add(DataType::UInt32, "Struct.Uint_off_4");

            Layout layout = Layout(rawLayout);

            if(layout.Get("Uint_off_0")->GetOffset() != 0)
                __debugbreak();

            if(layout.Get("Struct_off_4")->GetOffset() != 4)
                __debugbreak();


            if(layout.Get("Struct_off_4")->Get("Struct.Uint_off_4")->GetOffset() != 4)
                __debugbreak();

        };

        int _ = 0;
    };


    /*
    return;
    // Structure layout
    {
        Layout structLayout;

        {
            structLayout.Add(DataType::UInt32, "Uint_off_0");
        };

        structLayout.Add(DataType::Struct, "Test");
        Element& structTest = structLayout.Get("Test");
        {

            structTest.Add(DataType::UInt32, "Test_Uint_0");
            structTest.Add(DataType::Vec4f, "Test_Vec4_1");
            structTest.Add(DataType::Mat4f, "Test_Mat4_2");
        };


        structLayout.Add(DataType::Struct, "Test2");
        Element& structTest2 = structLayout.Get("Test2");
        {
            structTest2.Add(DataType::UInt32, "Test2_Uint_0");
            structTest2.Add(DataType::UInt32, "Test2_Uint_1");

            structTest2.Add(DataType::Mat4f, "Test2_Mat4_2");
        };


        if(structTest.Get("Test_Uint_0").GetOffset() != 16)
            __debugbreak();
        if(structTest.Get("Test_Vec4_1").GetOffset() != 32)
            __debugbreak();
        if(structTest.Get("Test_Mat4_2").GetOffset() != 48)
            __debugbreak();

        if(structTest2.Get("Test2_Uint_0").GetOffset() != 112)
            __debugbreak();
        if(structTest2.Get("Test2_Uint_1").GetOffset() != 116)
            __debugbreak();
        if(structTest2.Get("Test2_Mat4_2").GetOffset() != 128)
            __debugbreak();

    }


    // Array layout
    {
        // Array layout with same element
        {
            Layout arrayLayout;

            arrayLayout.Add(DataType::Vec4f, "Uint_off_0");

            arrayLayout.Add(DataType::Array, "Uint_off_16");

            Element& arrayElement = arrayLayout.Get("Uint_off_16");
            arrayElement.SetArrayType(DataType::UInt32);
            arrayElement.SetArrayCount(5);

            arrayLayout.Add(DataType::UInt32, "Uint_off_40");


            if(arrayLayout.Get("Uint_off_0").GetOffset() != 0)
                __debugbreak();

            if(arrayLayout.Get("Uint_off_16").GetOffset() != 16)
                __debugbreak();

            if(arrayElement.GetElementAtIndex(0).GetOffset() != 16)
                __debugbreak();
            if(arrayElement.GetElementAtIndex(1).GetOffset() != 20)
                __debugbreak();
            if(arrayElement.GetElementAtIndex(2).GetOffset() != 24)
                __debugbreak();
            if(arrayElement.GetElementAtIndex(3).GetOffset() != 28)
                __debugbreak();
            if(arrayElement.GetElementAtIndex(4).GetOffset() != 32)
                __debugbreak();

            if(arrayLayout.Get("Uint_off_40").GetOffset() != 36)
                __debugbreak();
        }

        // Array layout
        {
            Layout arrayLayout;
            arrayLayout.Add(DataType::Vec2f, "Vec2_off_0");

            arrayLayout.Add(DataType::Array, "Uint_off_8");

            Element& arrayElement = arrayLayout.Get("Uint_off_8");
            arrayElement.SetArrayType(DataType::UInt32);
            arrayElement.SetArrayCount(5);

            arrayLayout.Add(DataType::UInt32, "Uint_off_28");


            if(arrayLayout.Get("Vec2_off_0").GetOffset() != 0)
                __debugbreak();

            if(arrayLayout.Get("Uint_off_8").GetOffset() != 8)
                __debugbreak();

            if(arrayElement.GetElementAtIndex(0).GetOffset() != 8)
                __debugbreak();
            if(arrayElement.GetElementAtIndex(1).GetOffset() != 12)
                __debugbreak();
            if(arrayElement.GetElementAtIndex(2).GetOffset() != 16)
                __debugbreak();
            if(arrayElement.GetElementAtIndex(3).GetOffset() != 20)
                __debugbreak();
            if(arrayElement.GetElementAtIndex(4).GetOffset() != 24)
                __debugbreak();

            if(arrayLayout.Get("Uint_off_28").GetOffset() != 28)
                __debugbreak();
        };


        // Array of structs
        {
            Layout arrayLayout;

            arrayLayout.Add(DataType::UInt32, "Uint_off_0");


            arrayLayout.Add(DataType::Array, "Test_off_16");

            Element& arrayElement = arrayLayout.Get("Test_off_16");

            arrayElement.SetArrayType(DataType::Struct);

            arrayElement.DefineArrayStruct().Add(DataType::UInt32, "Test_Uint_0");
            arrayElement.DefineArrayStruct().Add(DataType::Vec4f, "Test_Vec4_1");

            arrayElement.SetArrayCount(2);

            arrayLayout.Add(DataType::UInt32, "Uint_off_80");



            if(arrayLayout.Get("Uint_off_0").GetOffset() != 0)
                __debugbreak();


            if(arrayLayout.Get("Test_off_16").GetOffset() != 16)
                __debugbreak();

            if(arrayElement.GetElementAtIndex(0).GetOffset() != 16)
                __debugbreak();

            if(arrayElement.GetElementAtIndex(0).Get("Test_Uint_0").GetOffset() != 16)
                __debugbreak();
            if(arrayElement.GetElementAtIndex(0).Get("Test_Vec4_1").GetOffset() != 32)
                __debugbreak();

            if(arrayElement.GetElementAtIndex(1).Get("Test_Uint_0").GetOffset() != 48)
                __debugbreak();
            if(arrayElement.GetElementAtIndex(1).Get("Test_Vec4_1").GetOffset() != 64)
                __debugbreak();


            if(arrayLayout.Get("Uint_off_80").GetOffset() != 80)
                __debugbreak();


            int _ = 0;
        };

    };

    int _ = 0;
    */

    // layout.Add(DataType::UInt32, "Uint_off_4");
};


int main()
{
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


/*

class Element
{
public:

    DataType Type = DataType::None;

    std::size_t SizeInBytes = 0;

    std::size_t Offset = 0;


    std::vector<std::pair<std::string, Element>> StructElements;

    std::vector<Element> ArrayElements;

    DataType ArrayElementType = DataType::None;

    std::size_t ArrayElementCount = static_cast<std::size_t>(-1);



public:

    Element(DataType type) :
        Type(type),
        SizeInBytes(DataTypeSizeInBytes(type))
    {
    };


public:

    Element& GetAtIndex(std::size_t index)
    {
        Element& element = ArrayElements[index];

        return element;
    };

    const Element& GetAtIndex(std::size_t index) const
    {
        return GetAtIndex(index);
    };


    Element& Get(const std::string_view& name)
    {
        for(auto& structElement : StructElements)
        {
            if(structElement.first == name)
            {
                return structElement.second;
            };
        };

        wt::Assert(false, [&]()
        {
            return std::string("No such element \"").append(name).append("\" was found");
        });
    };

    const Element& Get(const std::string_view& name) const
    {
        return Get(name);
    };


    void Add(DataType dataType, const std::string_view& name)
    {
        StructElements.emplace_back(std::make_pair(name, Element(dataType)));
    };

    void SetArray(DataType arrayType, std::size_t elementCount)
    {
        ArrayElementType = arrayType;
        ArrayElementCount = elementCount;
    };

    Element& SetCustomArrayType(std::size_t elementCount)
    {
        ArrayElementType = DataType::Struct;
        ArrayElementCount = elementCount;

        return ArrayElements.emplace_back(ArrayElementType);
    };
};



class RawLayout
{
public:

    std::vector<std::pair<std::string, Element>> _layoutElements;

public:

    Element& Add(DataType dataType, const std::string_view& name)
    {
        auto& insertResult = _layoutElements.emplace_back(std::make_pair(name, Element(dataType)));

        return insertResult.second;
    };

};


class Layout
{

private:

    std::unordered_map<std::string, Element> _layoutElements;


public:

    // TODO: Maybe add move constructor for "RawLayout&&"?
    Layout(RawLayout& rawLayout)
    {
        const auto layout = CreateLayout(rawLayout);
        _layoutElements.insert(layout.begin(), layout.end());
    };


public:

    Element& Get(const std::string_view& name)
    {
        auto findResult = _layoutElements.find(name.data());

        return findResult->second;
    };

    const Element& Get(const std::string_view& name) const
    {
        return Get(name);
    };


private:

    std::vector<std::pair<std::string, Element>> CreateLayout(RawLayout& rawLayout, std::size_t offset = 0) const
    {
        std::vector<std::pair<std::string, Element>> layoutResult;

        std::size_t currentOffset = offset;


        for(auto& rawLayoutElement : rawLayout._layoutElements)
        {
            Element& element = rawLayoutElement.second;

            if(element.Type == DataType::Array)
            {
                if(element.ArrayElementType == DataType::Struct)
                {
                    RawLayout rawArrayStructLayout;

                    for(std::size_t i = 0; i < element.ArrayElementCount; ++i)
                    {
                        Element& rawStructLayout = rawArrayStructLayout.Add(DataType::Struct, std::string("[").append(std::to_string(i)).append("]"));

                        for(auto& structElement : element.ArrayElements[0].StructElements)
                        {
                            rawStructLayout.Add(structElement.second.Type, structElement.first);
                        };
                    };

                    const auto arrayStructLayout = CreateLayout(rawArrayStructLayout, currentOffset);


                    element.ArrayElements.clear();

                    for(auto& arrayStructElement : arrayStructLayout)
                    {
                        element.ArrayElements.emplace_back(arrayStructElement.second);
                    };


                    const auto arrayLayoutEnd = std::prev((arrayStructLayout.cend()));

                    layoutResult.emplace_back(std::make_pair(rawLayoutElement.first, element));
                    currentOffset = arrayLayoutEnd->second.Offset + arrayLayoutEnd->second.SizeInBytes;
                }
                else
                {
                    const bool crossesBoundary = CrossesBoundary(currentOffset, DataTypeSizeInBytes(element.ArrayElementType));

                    if(crossesBoundary == true)
                    {
                        const std::size_t rawOffset = currentOffset;

                        const std::size_t actualOffset = GetCorrectOffset(rawOffset);

                        const std::size_t padding = actualOffset - currentOffset;

                        element.Offset = actualOffset;
                    }
                    else
                    {
                        element.Offset = currentOffset;
                    };


                    RawLayout rawArrayLayout;

                    for(std::size_t i = 0; i < element.ArrayElementCount; ++i)
                    {
                        rawArrayLayout.Add(element.ArrayElementType, std::string("[").append(std::to_string(i)).append("]"));
                    };

                    const auto arrayLayout = CreateLayout(rawArrayLayout, currentOffset);


                    for(auto& arrayLayoutElement : arrayLayout)
                    {
                        element.ArrayElements.emplace_back(arrayLayoutElement.second);
                    };

                    const auto arrayLayoutEnd = std::prev((arrayLayout.cend()));


                    layoutResult.emplace_back(std::make_pair(rawLayoutElement.first, element));
                    currentOffset = arrayLayoutEnd->second.Offset;
                };
            }
            else if(element.Type == DataType::Struct)
            {
                RawLayout rawStructLayout;

                for(auto& structElement : element.StructElements)
                {
                    rawStructLayout.Add(structElement.second.Type, structElement.first);
                };

                // TODO: Find a way to optimize struct size

                // Calculate struct size
                const auto structLayoutInitial = CreateLayout(rawStructLayout, 0);
                auto structLayoutEnd = std::prev(structLayoutInitial.end());
                const std::size_t structSize = structLayoutEnd->second.Offset + structLayoutEnd->second.SizeInBytes;


                const bool crossesBoundary = CrossesBoundary(currentOffset, structSize);

                if(crossesBoundary == true)
                {
                    const std::size_t rawOffset = currentOffset;

                    const std::size_t actualOffset = GetCorrectOffset(rawOffset);

                    const std::size_t padding = actualOffset - currentOffset;

                    currentOffset = actualOffset;
                    element.Offset = currentOffset;
                };

                element.SizeInBytes = structSize;


                const auto structLayoutFinal = CreateLayout(rawStructLayout, currentOffset);

                element.StructElements.clear();

                for(auto& structLayoutElement : structLayoutFinal)
                {
                    element.StructElements.emplace_back(structLayoutElement);
                };

                structLayoutEnd = std::prev(structLayoutFinal.cend());

                layoutResult.emplace_back(std::make_pair(rawLayoutElement.first, element));
                currentOffset = structLayoutEnd->second.Offset + structLayoutEnd->second.SizeInBytes;
            }
            else
            {
                const bool crossesBoundary = CrossesBoundary(currentOffset, element.SizeInBytes);

                if(crossesBoundary == true)
                {
                    const std::size_t rawOffset = currentOffset;

                    const std::size_t actualOffset = GetCorrectOffset(rawOffset);

                    const std::size_t padding = actualOffset - currentOffset;

                    element.Offset = actualOffset;
                }
                else
                {
                    element.Offset = currentOffset;
                };

                layoutResult.emplace_back(std::make_pair(rawLayoutElement.first, element));
                currentOffset = element.Offset + element.SizeInBytes;
            };

        };

        return layoutResult;
    };



    constexpr std::size_t GetCorrectOffset(std::size_t rawOffset) const
    {
        const std::size_t correctedOffset = rawOffset + (16u - rawOffset % 16u) % 16u;

        return correctedOffset;
    };

    /// <summary>
    /// Check if an element crosses a 16-byte boundary
    /// </summary>
    /// <param name="offset"> </param>
    /// <param name="size"></param>
    /// <returns></returns>
    constexpr bool CrossesBoundary(std::size_t offset, std::size_t sizeInBytes) const
    {
        const std::size_t end = offset + sizeInBytes;

        const std::size_t pageStart = offset / 16u;
        const std::size_t pageEnd = end / 16u;

        return ((pageStart != pageEnd) && (end % 16 != 0u)) || (sizeInBytes > 16u);
    };

};
*/