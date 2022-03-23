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


class Element
{

private:

    Element* _lastAddedLayoutElement = nullptr;

    Element* _lastAddedElement = nullptr;


public:

    // Only applied to structs
    std::unordered_map<std::string, Element> StructElements;


    // Only applied to Arrays
    std::vector<Element> ArrayElements;

    DataType ArrayDataType = DataType::None;

    std::size_t MaxArrayElements = static_cast<std::size_t>(-1);

    std::size_t AddedArrayElements = 0;





    DataType Type = DataType::None;

    std::size_t SizeInBytes = 0;

    std::size_t Offset = 0;


public:

    Element(DataType type,
            Element* lastAddedLayoutElement) :
        _lastAddedLayoutElement(lastAddedLayoutElement),
        Type(type),
        SizeInBytes(DataTypeToBytes(Type))
    {
    };


public:

    void Add(DataType dataType, const std::string_view& name)
    {
        wt::Assert((Type == DataType::Struct || Type == DataType::Array) && Type != DataType::None,
                   []()
        {
            return std::string("Trying to add element to non-struct or array type");
        });

        if(Type == DataType::Array)
        {
            ++AddedArrayElements;

            wt::Assert(AddedArrayElements > MaxArrayElements, []()
            {
                return std::string("Too many array elements");
            });
        };


        const auto& insertResult = StructElements.insert(std::make_pair(name, Element { dataType, _lastAddedLayoutElement }));

        Element& newElement = (StructElements.find(name.data()))->second;


        if(StructElements.size() == 1)
        {
            _lastAddedElement = &newElement;
            SizeInBytes = newElement.SizeInBytes;

            newElement.Offset = Offset;

            return;
        };


        if(dataType == DataType::Struct)
        {
            const std::size_t rawOffset = _lastAddedElement->Offset + _lastAddedElement->SizeInBytes;

            const std::size_t actualOffset = rawOffset + (16u - rawOffset % 16u) % 16u;

            newElement.Offset = actualOffset;
        }
        else if(dataType == DataType::Array)
        {
            const std::size_t rawOffset = _lastAddedElement->Offset + _lastAddedElement->SizeInBytes;

            const std::size_t actualOffset = rawOffset + (16u - rawOffset % 16u) % 16u;

            newElement.Offset = actualOffset;
        }
        else
        {
            const std::size_t rawOffset = _lastAddedElement->SizeInBytes + _lastAddedElement->Offset;

            // If the element crosses over a 16 byte boundary
            const bool crossesBoundary = CrossesBoundary(rawOffset, newElement.SizeInBytes);

            if(crossesBoundary == false)
            {
                newElement.Offset = rawOffset;
                SizeInBytes += newElement.SizeInBytes;
            }
            else
            {
                const std::size_t actualOffset = rawOffset + (16u - rawOffset % 16u) % 16u;

                const std::size_t padding = actualOffset - (SizeInBytes + Offset);

                newElement.Offset = actualOffset;
                SizeInBytes += newElement.SizeInBytes + padding;
            };
        };


        _lastAddedElement = &newElement;
    };


    Element& Get(const std::string_view& name)
    {
        auto findResult = StructElements.find(name.data());

        return findResult->second;
    };


    void SetArrayType(DataType arrayDataType)
    {
        wt::Assert(Type == DataType::Array,
                   []()
        {
            return "Trying to assign type on non array Element type";
        });

        wt::Assert(ArrayDataType == DataType::None,
                   []()
        {
            return "Trying to assign new type on defined array type";
        });

        wt::Assert(arrayDataType != DataType::None,
                   []()
        {
            return "Invalid array type";
        });



        ArrayDataType = arrayDataType;

        // if(arrayDataType == DataType::Struct)
        //     return;


        const std::size_t lastAddedLayoutElementSizeInBytes = _lastAddedLayoutElement != nullptr ?
            _lastAddedLayoutElement->SizeInBytes : 0;

        const std::size_t lastAddedLayoutElementOffset = _lastAddedLayoutElement != nullptr ?
            _lastAddedLayoutElement->Offset : 0;


        const std::size_t elementSizeInBytes = DataTypeToBytes(ArrayDataType);


        const bool crossesBoundary = CrossesBoundary(lastAddedLayoutElementSizeInBytes + lastAddedLayoutElementOffset, elementSizeInBytes);

        if(crossesBoundary == true)
        {
            const std::size_t rawOffset = _lastAddedLayoutElement != nullptr ?
                lastAddedLayoutElementOffset + lastAddedLayoutElementSizeInBytes : 0;

            const std::size_t actualOffset = rawOffset + (16u - rawOffset % 16u) % 16u;

            Offset = actualOffset;
        }
        else
        {
            const std::size_t rawOffset = _lastAddedLayoutElement != nullptr ?
                lastAddedLayoutElementOffset + lastAddedLayoutElementSizeInBytes : 0;

            Offset = rawOffset;
        };
    };


    Element& DefineArrayStruct()
    {
        if(ArrayElements.empty() == true)
        {
            Element initialElement = Element(DataType::Struct, _lastAddedLayoutElement);

            const std::size_t actualOffset = Offset + (16u - Offset % 16u) % 16u;

            initialElement.Offset = actualOffset;

            ArrayElements.push_back(initialElement);
        };

        return ArrayElements.back();
    };


    void SetArrayCount(std::size_t numberOfElements)
    {
        wt::Assert(Type == DataType::Array, []()
        {
            return "Trying to set array size on non-array element";
        });

        wt::Assert(ArrayDataType != DataType::None, []()
        {
            return "Array type must be defined to set size";
        });

        wt::Assert(numberOfElements >= 1, []()
        {
            return "Invalid array size";
        });

        wt::Assert(MaxArrayElements == -1, []()
        {
            return "Array size already defined";
        });


        if(ArrayDataType == DataType::Struct)
        {
            MaxArrayElements = numberOfElements;

            for(std::size_t i = 0; i < MaxArrayElements - 1; ++i)
            {
                Element lastAddedElementCopy = ArrayElements.back();

                const std::size_t offset = lastAddedElementCopy.Offset + lastAddedElementCopy.SizeInBytes;

                lastAddedElementCopy.Offset = offset;

                ArrayElements.push_back(lastAddedElementCopy);

                SizeInBytes += lastAddedElementCopy.SizeInBytes;
            };
        }
        else
        {
            MaxArrayElements = numberOfElements;

            Element initialElement = Element(ArrayDataType, _lastAddedLayoutElement);

            initialElement.Offset = Offset;

            ArrayElements.push_back(initialElement);

            SizeInBytes += initialElement.SizeInBytes;


            for(std::size_t i = 0; i < MaxArrayElements - 1; ++i)
            {
                const Element& lastAddedElement = ArrayElements.back();

                const std::size_t offset = lastAddedElement.Offset + lastAddedElement.SizeInBytes;

                Element newElement = Element(ArrayDataType, _lastAddedLayoutElement);

                newElement.Offset = offset;

                ArrayElements.push_back(newElement);

                SizeInBytes += newElement.SizeInBytes;
            };

        };

    };


    Element& GetElementAtIndex(std::size_t index)
    {
        wt::Assert(Type == DataType::Array, []()
        {
            return "Trying to index into non-array element";
        });

        wt::Assert(index >= 0, []()
        {
            return "Invalid index";
        });

        return ArrayElements[index];
    };


private:

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


    constexpr std::size_t DataTypeToBytes(DataType type)
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


        const auto& insertResult = _layoutElements.insert(std::make_pair(name, Element { dataType, _lastAddedElement }));

        Element& newElement = (_layoutElements.find(name.data()))->second;


        if(_layoutElements.size() == 1)
        {
            _lastAddedElement = &newElement;
            return;
        };



        if(dataType == DataType::Struct)
        {
            const std::size_t rawOffset = _lastAddedElement->Offset + _lastAddedElement->SizeInBytes;

            const std::size_t actualOffset = rawOffset + (16u - rawOffset % 16u) % 16u;

            newElement.Offset = actualOffset;
        }
        // else if(dataType == DataType::Array)
        // {
        //     const std::size_t rawOffset = _lastAddedElement->Offset + _lastAddedElement->SizeInBytes;
        // 
        //     const std::size_t actualOffset = rawOffset + (16u - rawOffset % 16u) % 16u;
        // 
        //     newElement.Offset = actualOffset;
        // }
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
    constexpr bool CrossesBoundary(std::size_t offset, std::size_t sizeInBytes) const
    {
        const std::size_t end = offset + sizeInBytes;

        const std::size_t pageStart = offset / 16u;
        const std::size_t pageEnd = end / 16u;

        return ((pageStart != pageEnd) && (end % 16 != 0u)) || (sizeInBytes > 16u);
    };


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


class Element2
{

public:

    DataType Type = DataType::None;

    std::size_t SizeInBytes = 0;

    std::size_t Offset = 0;


    std::vector<std::pair<std::string, Element2>> StructElements;

    std::vector<Element2> ArrayElements;

    DataType ArrayElementType = DataType::None;

    std::size_t ArrayElementCount = static_cast<std::size_t>(-1);



public:

    Element2(DataType type) :
        Type(type),
        SizeInBytes(DataTypeSizeInBytes(type))
    {
    };


public:

    Element2& GetAtIndex(std::size_t index)
    {
        Element2& element = ArrayElements[index];

        return element;
    };

    Element2& Get(const std::string_view& name)
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

    void Add(DataType dataType, const std::string_view& name)
    {
        StructElements.emplace_back(std::make_pair(name, Element2(dataType)));
    };

    void SetArray(DataType arrayType, std::size_t elementCount)
    {
        ArrayElementType = arrayType;
        ArrayElementCount = elementCount;

        //if(ArrayElementType == DataType::Struct)
        //{
        //    ArrayElements.emplace_back(Element2(DataType::None));
        //};
    };

    Element2& SetCustomArrayType()
    {
        return ArrayElements.back();
    };
};


class RawLayout
{
public:

    std::vector<std::pair<std::string, Element2>> _layoutElements;


public:

    Element2& Add(DataType dataType, const std::string_view& name)
    {
        auto& insertResult = _layoutElements.emplace_back(std::make_pair(name, Element2(dataType)));

        return insertResult.second;
    };

};


class Layout2
{

private:

    std::unordered_map<std::string, Element2> _layoutElements;


public:

    Layout2(const RawLayout& rawLayout)
    {
        const auto layout = CreateLayout(rawLayout);
        _layoutElements.insert(layout.begin(), layout.end());
    };


public:

    Element2& Get(const std::string_view& name)
    {
        auto findResult = _layoutElements.find(name.data());

        return findResult->second;
    };



private:

    std::vector<std::pair<std::string, Element2>> CreateLayout(const RawLayout& rawLayout, std::size_t offset = 0) const
    {
        std::vector<std::pair<std::string, Element2>> layoutResult;

        std::size_t currentOffset = offset;


        for(auto& rawLayoutElement : rawLayout._layoutElements)
        {
            Element2 element = rawLayoutElement.second;

            if(element.Type == DataType::Array)
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

                auto arrayLayout = CreateLayout(rawArrayLayout, currentOffset);


                for(auto & arrayLayoutElement : arrayLayout)
                {
                    element.ArrayElements.emplace_back(arrayLayoutElement.second);
                };

                auto arrayLayoutEnd = std::prev((arrayLayout.end()));


                layoutResult.emplace_back(std::make_pair(rawLayoutElement.first, element));
                currentOffset = arrayLayoutEnd->second.Offset;
            }
            else if(element.Type == DataType::Struct)
            {
                RawLayout rawStructLayout;

                for(auto& structElement : element.StructElements)
                {
                    rawStructLayout.Add(structElement.second.Type, structElement.first);
                };

                // Calculate struct size
                auto structLayoutInitial = CreateLayout(rawStructLayout, 0);

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

                auto structLayoutFinal = CreateLayout(rawStructLayout, currentOffset);

                element.StructElements.clear();

                for(auto& structLayoutElement : structLayoutFinal)
                {
                    element.StructElements.emplace_back(structLayoutElement);
                };

                structLayoutEnd = std::prev(structLayoutFinal.end());

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


void SSBOTest()
{
    // Raw layout
    {
        // Uint32 - vec4 padding
        {
            RawLayout rawLayout;

            rawLayout.Add(DataType::UInt32, "Uint_off_0");
            rawLayout.Add(DataType::Vec4f, "Vec4_off_16");


            Layout2 layout = Layout2 (rawLayout);

            if(layout.Get("Uint_off_0").Offset != 0)
                __debugbreak();
            if(layout.Get("Vec4_off_16").Offset != 16)
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


            Layout2 layout = Layout2 (rawLayout);

            if(layout.Get("Uint_off_0").Offset != 0)
                __debugbreak();
            if(layout.Get("Uint_off_4").Offset != 4)
                __debugbreak();
            if(layout.Get("Uint_off_8").Offset != 8)
                __debugbreak();
            if(layout.Get("Uint_off_12").Offset != 12)
                __debugbreak();
            if(layout.Get("Uint_off_16").Offset != 16)
                __debugbreak();
            int _ = 0;
        };


        {
            RawLayout rawLayout;

            rawLayout.Add(DataType::Vec2f, "Vec2_off_0");
            rawLayout.Add(DataType::UInt32, "Uint_off_8");

            Layout2 layout = Layout2 (rawLayout);

            if(layout.Get("Vec2_off_0").Offset != 0)
                __debugbreak();
            if(layout.Get("Uint_off_8").Offset != 8)
                __debugbreak();
        };


        {
            RawLayout rawLayout;

            rawLayout.Add(DataType::Vec2f, "Vec2_off_0");
            rawLayout.Add(DataType::Vec2f, "Vec2_off_8");

            Layout2 layout = Layout2 (rawLayout);

            if(layout.Get("Vec2_off_0").Offset != 0)
                __debugbreak();
            if(layout.Get("Vec2_off_8").Offset != 8)
                __debugbreak();
        };


        // Array 
        {
            RawLayout rawLayout;

            Element2& rawArrayElement = rawLayout.Add(DataType::Array, "Uint_off_0");

            rawArrayElement.SetArray(DataType::UInt32, 4);
            Layout2 layout = Layout2(rawLayout);

            Element2& arrayElement = layout.Get("Uint_off_0");

            if(layout.Get("Uint_off_0").Offset != 0)
                __debugbreak();

            if(arrayElement.GetAtIndex(0).Offset != 0)
                __debugbreak();
            if(arrayElement.GetAtIndex(1).Offset != 4)
                __debugbreak();
            if(arrayElement.GetAtIndex(2).Offset != 8)
                __debugbreak();
            if(arrayElement.GetAtIndex(3).Offset != 12)
                __debugbreak();

        };


        // Array with padding
        {
            RawLayout rawLayout;

            rawLayout.Add(DataType::UInt32, "Uint_off_0");

            Element2& rawArrayElement = rawLayout.Add(DataType::Array, "Array_off_16");

            rawArrayElement.SetArray(DataType::Vec4f, 3);

            Layout2 layout = Layout2(rawLayout);

            Element2& arrayElement = layout.Get("Array_off_16");


            if(layout.Get("Uint_off_0").Offset != 0)
                __debugbreak();

            if(layout.Get("Array_off_16").Offset != 16)
                __debugbreak();

            if(arrayElement.GetAtIndex(0).Offset != 16)
                __debugbreak();
            if(arrayElement.GetAtIndex(1).Offset != 32)
                __debugbreak();
            if(arrayElement.GetAtIndex(2).Offset != 48)
                __debugbreak();

        };


        // Struct
        {
            RawLayout structLayout;
            
            structLayout.Add(DataType::UInt32, "Uint_off_0");

            Element2& rawStructElement = structLayout.Add(DataType::Struct, "Test_off_16");
            
            rawStructElement.Add(DataType::UInt32, "Test_Uint_off_16");
            rawStructElement.Add(DataType::Vec4f, "Test_Vec4_off_32");

            structLayout.Add(DataType::UInt32, "Uint_off_48");

            Layout2 layout = Layout2(structLayout);
            
            Element2& structElement = layout.Get("Test_off_16");
            

            if(layout.Get("Uint_off_0").Offset != 0)
                __debugbreak();

            if(layout.Get("Test_off_16").Offset != 16)
                __debugbreak();

                if(structElement.Get("Test_Uint_off_16").Offset != 16)
                    __debugbreak();

                if(structElement.Get("Test_Vec4_off_32").Offset != 32)
                    __debugbreak();

            if(layout.Get("Uint_off_48").Offset != 48)
                __debugbreak();
        };


        // Multiple structs
        {
            RawLayout structLayout;

            structLayout.Add(DataType::UInt32, "Uint_off_0");

            Element2& rawStructElement = structLayout.Add(DataType::Struct, "Test_off_16");

            rawStructElement.Add(DataType::UInt32, "Test_Uint_off_16");
            rawStructElement.Add(DataType::Vec4f, "Test_Vec4_off_32");


            Element2& rawStructElement2 = structLayout.Add(DataType::Struct, "Test2_off_48");

            rawStructElement2.Add(DataType::UInt32, "Test2_Uint_off_48");
            rawStructElement2.Add(DataType::Mat4f, "Test2_Mat4_off_64");


            Layout2 layout = Layout2(structLayout);

            Element2& structElement = layout.Get("Test_off_16");
            Element2& structElement2 = layout.Get("Test2_off_48");


            if(layout.Get("Uint_off_0").Offset != 0)
                __debugbreak();

            if(layout.Get("Test_off_16").Offset != 16)
                __debugbreak();

                if(structElement.Get("Test_Uint_off_16").Offset != 16)
                    __debugbreak();

                if(structElement.Get("Test_Vec4_off_32").Offset != 32)
                    __debugbreak();

            if(layout.Get("Test2_off_48").Offset != 48)
                __debugbreak();

                if(structElement2.Get("Test2_Uint_off_48").Offset != 48)
                    __debugbreak();

                if(structElement2.Get("Test2_Mat4_off_64").Offset != 64)
                    __debugbreak();


            int _ = 0;

        };

        int _ = 0;
    };


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


        if(structTest.Get("Test_Uint_0").Offset != 16)
            __debugbreak();
        if(structTest.Get("Test_Vec4_1").Offset != 32)
            __debugbreak();
        if(structTest.Get("Test_Mat4_2").Offset != 48)
            __debugbreak();

        if(structTest2.Get("Test2_Uint_0").Offset != 112)
            __debugbreak();
        if(structTest2.Get("Test2_Uint_1").Offset != 116)
            __debugbreak();
        if(structTest2.Get("Test2_Mat4_2").Offset != 128)
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


            if(arrayLayout.Get("Uint_off_0").Offset != 0)
                __debugbreak();

            if(arrayLayout.Get("Uint_off_16").Offset != 16)
                __debugbreak();

            if(arrayElement.GetElementAtIndex(0).Offset != 16)
                __debugbreak();
            if(arrayElement.GetElementAtIndex(1).Offset != 20)
                __debugbreak();
            if(arrayElement.GetElementAtIndex(2).Offset != 24)
                __debugbreak();
            if(arrayElement.GetElementAtIndex(3).Offset != 28)
                __debugbreak();
            if(arrayElement.GetElementAtIndex(4).Offset != 32)
                __debugbreak();

            if(arrayLayout.Get("Uint_off_40").Offset != 36)
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


            if(arrayLayout.Get("Vec2_off_0").Offset != 0)
                __debugbreak();

            if(arrayLayout.Get("Uint_off_8").Offset != 8)
                __debugbreak();

            if(arrayElement.GetElementAtIndex(0).Offset != 8)
                __debugbreak();
            if(arrayElement.GetElementAtIndex(1).Offset != 12)
                __debugbreak();
            if(arrayElement.GetElementAtIndex(2).Offset != 16)
                __debugbreak();
            if(arrayElement.GetElementAtIndex(3).Offset != 20)
                __debugbreak();
            if(arrayElement.GetElementAtIndex(4).Offset != 24)
                __debugbreak();

            if(arrayLayout.Get("Uint_off_28").Offset != 28)
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



            if(arrayLayout.Get("Uint_off_0").Offset != 0)
                __debugbreak();


            if(arrayLayout.Get("Test_off_16").Offset != 16)
                __debugbreak();

            if(arrayElement.GetElementAtIndex(0).Offset != 16)
                __debugbreak();

            if(arrayElement.GetElementAtIndex(0).Get("Test_Uint_0").Offset != 16)
                __debugbreak();
            if(arrayElement.GetElementAtIndex(0).Get("Test_Vec4_1").Offset != 32)
                __debugbreak();

            if(arrayElement.GetElementAtIndex(1).Get("Test_Uint_0").Offset != 48)
                __debugbreak();
            if(arrayElement.GetElementAtIndex(1).Get("Test_Vec4_1").Offset != 64)
                __debugbreak();


            if(arrayLayout.Get("Uint_off_80").Offset != 80)
                __debugbreak();


            int _ = 0;
        };

    };

    int _ = 0;

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