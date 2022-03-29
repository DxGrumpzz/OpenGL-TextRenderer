#define WIN32_LEAN_AND_MEN

#include <glad/glad.h>
#include <Windows.h>
#include <string_view>

#include "ShaderProgram.hpp"
#include "FontSprite.hpp"
#include "ShaderStorageBuffer.hpp"
#include "WindowsUtilities.hpp"
#include <variant>


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
            return static_cast<std::size_t>(-1);
        };
    };
};



class SSBOLayout;

class IElement
{
    friend class SSBOLayout;

protected:

    std::size_t _offset = 0;

    DataType _elementType = DataType::None;

    std::size_t _sizeInBytes = 0;


protected:

    IElement() = default;

public:


    virtual ~IElement()
    {
    };

public:

    std::size_t GetOffset() const
    {
        return _offset;
    }

    std::size_t GetSizeInBytes() const
    {
        return _sizeInBytes;
    };

    DataType GetElementType() const
    {
        return _elementType;
    };
};


class ScalarElement : public IElement
{
public:

    ScalarElement(DataType type)
    {
        _elementType = type;
        _sizeInBytes = DataTypeSizeInBytes(type);
    };

public:

    template<typename T>
    void Set(const T& value)
    {
        switch(_elementType)
        {
            case DataType::UInt32:
            {
                wt::Assert(std::is_same <T, std::uint32_t>::value == true, []()
                {
                    return std::string("Invalid value type. Expected \"DataType::UInt32\"");
                });

                break;
            };

            case DataType::Vec2f:
                break;
            case DataType::Vec4f:
                break;
            case DataType::Mat4f:
                break;

            default:
                wt::Assert(false, "Invalid element");
        };
    };
};

class StructElement : public IElement
{
    class ArrayElement;

    friend class SSBOLayout;

private:

    std::vector<std::pair<std::string, std::shared_ptr<IElement>>> _structElements;


public:

    StructElement()
    {
        _elementType = DataType::Struct;
        _sizeInBytes = DataTypeSizeInBytes(_elementType);
    };

public:

    template<typename TElement>
    std::shared_ptr<TElement> Get(const std::string_view& name)
    {
        wt::Assert(_elementType == DataType::Struct, [&]()
        {
            return "Attempting to retrieve struct member on non-struct element";
        });

        for(auto& [elementName, structElement] : _structElements)
        {
            if(elementName == name)
            {
                auto castResult = std::dynamic_pointer_cast<TElement, IElement>(structElement);

                wt::Assert(castResult != nullptr, "Invalid element cast");

                return castResult;
            };
        };

        wt::Assert(false, [&]()
        {
            return std::string("No such element \"").append(name).append("\" was found");
        });

        return nullptr;
    };

    template<typename TElement>
    const std::shared_ptr<TElement> Get(const std::string_view& name) const
    {
        return Get<TElement>(name);
    };



    template<typename TElement, DataType dataType>
    std::shared_ptr<TElement> Add(const std::string_view& name)
    {
        if constexpr(std::is_same<TElement, StructElement>::value == true && dataType != DataType::Struct)
        {
            static_assert(false, "Incompatable (Struct) element types");
        }
        else if constexpr(std::is_same<TElement, ArrayElement>::value == true && dataType != DataType::Array)
        {
            static_assert(false, "Incompatable (Array) element types");
        }
        else if constexpr(std::is_same<TElement, ScalarElement>::value == true && (dataType == DataType::None || dataType == DataType::Struct || dataType == DataType::Array))
        {
            static_assert(false, "Incompatable (Scalar) element types");
        };


        const auto elementResult = Add(dataType, name);

        return std::dynamic_pointer_cast<TElement, IElement>(elementResult);
    };


    template<typename TElement>
    requires (std::is_base_of<IElement, TElement>::value == true)
        std::shared_ptr<TElement> Add(const std::string_view& name)
    {
        if constexpr(std::is_same<TElement, StructElement>::value == true)
        {
            const auto elementResult = Add(DataType::Struct, name);
            return std::dynamic_pointer_cast<StructElement, IElement>(elementResult);
        }
        else if constexpr(std::is_same<TElement, ArrayElement>::value == true)
        {
            const auto  elementResult = Add(DataType::Struct, name);
            return std::dynamic_pointer_cast<ArrayElement, IElement>(elementResult);
        }
        else
        {
            static_assert(false, "Incompatable (Scalar) element types");
        };
    };


private:

    std::shared_ptr<IElement> Add(DataType dataType, const std::string_view& name)
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



        if(dataType == DataType::Struct)
        {
            auto elementResult = _structElements.emplace_back(std::make_pair(name, std::make_shared<StructElement>()));
            return elementResult.second;
        }
        else if(dataType == DataType::Array)
        {
            // TODO: This throws an error because "ArrayElement" is not defined, forward declaration won't work
            // auto elementResult = _structElements.emplace_back(std::make_pair(name, std::make_shared<ArrayElement>()));
            // return elementResult.second;
        };

        auto elementResult = _structElements.emplace_back(std::make_pair(name, std::make_shared<ScalarElement>(dataType)));
        return elementResult.second;
    };

};

class ArrayElement : public IElement
{
    friend class SSBOLayout;

private:

    std::vector<std::shared_ptr<IElement>> _arrayElements;

    DataType _arrayElementType = DataType::None;

    std::size_t _arrayElementCount = static_cast<std::size_t>(-1);


public:

    ArrayElement()
    {
        _elementType = DataType::Array;
        _sizeInBytes = DataTypeSizeInBytes(_elementType);
    };

public:

    template<typename TElement>
    std::shared_ptr<TElement> GetAtIndex(std::size_t index)
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

        auto element = _arrayElements[index];

        return std::dynamic_pointer_cast<TElement, IElement>(element);
    };

    template<typename TElement>
    const std::shared_ptr<TElement> GetAtIndex(std::size_t index) const
    {
        return GetAtIndex<TElement>(index);
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

    std::shared_ptr<StructElement> SetCustomArrayType(std::size_t elementCount)
    {
        SetArray(DataType::Struct, elementCount);

        return std::dynamic_pointer_cast<StructElement, IElement>(_arrayElements.emplace_back(std::make_shared<StructElement>()));
    };



public:

    DataType GetArrayElementType()
    {
        return _arrayElementType;
    };

    std::size_t GetElementCount()
    {
        return _arrayElementCount;
    };
};



class RawLayout
{
    friend class SSBOLayout;

private:

    std::vector<std::pair<std::string, std::shared_ptr<IElement>>> _layoutElements;

public:


    template<typename TElement, DataType dataType>
    std::shared_ptr<TElement> Add(const std::string_view& name)
    {
        if constexpr(std::is_same<TElement, StructElement>::value == true && dataType != DataType::Struct)
        {
            static_assert(false, "Incompatable (Struct) element types");
        }
        else if constexpr(std::is_same<TElement, ArrayElement>::value == true && dataType != DataType::Array)
        {
            static_assert(false, "Incompatable (Array) element types");
        }
        else if constexpr(std::is_same<TElement, ScalarElement>::value == true && (dataType == DataType::None || dataType == DataType::Struct || dataType == DataType::Array))
        {
            static_assert(false, "Incompatable (Scalar) element types");
        };


        const auto elementResult = Add(dataType, name);

        return std::dynamic_pointer_cast<TElement, IElement>(elementResult);
    };

    template<typename TElement>
    requires (std::is_base_of<IElement, TElement>::value == true)
        std::shared_ptr<TElement> Add(const std::string_view& name)
    {
        if constexpr(std::is_same<TElement, StructElement>::value == true)
        {
            const auto elementResult = Add(DataType::Struct, name);
            return std::dynamic_pointer_cast<StructElement, IElement>(elementResult);
        }
        else if constexpr(std::is_same<TElement, ArrayElement>::value == true)
        {
            const auto  elementResult = Add(DataType::Struct, name);
            return std::dynamic_pointer_cast<ArrayElement, IElement>(elementResult);
        }
        else
        {
            static_assert(false, "Incompatable (Scalar) element types");
        };
    };


private:

    std::shared_ptr<IElement> Add(DataType dataType, const std::string_view& name)
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


        if(dataType == DataType::Struct)
        {
            auto insertResult = _layoutElements.emplace_back(std::make_pair(name, std::make_shared<StructElement>()));
            return insertResult.second;
        }
        else if(dataType == DataType::Array)
        {
            auto insertResult = _layoutElements.emplace_back(std::make_pair(name, std::make_shared<ArrayElement>()));
            return insertResult.second;
        };


        auto insertResult = _layoutElements.emplace_back(std::make_pair(name, std::make_shared<ScalarElement>(dataType)));
        return insertResult.second;
    };

};


class SSBOLayout
{
private:

    std::unordered_map<std::string, std::shared_ptr<IElement>> _layoutElements;


public:

    // TODO: Maybe add move constructor for "RawLayout&&"?
    SSBOLayout(RawLayout& rawLayout)
    {
        const auto layout = CreateLayout(rawLayout);
        _layoutElements.insert(layout.begin(), layout.end());
    };


public:

    template<typename TElement>
    std::shared_ptr<TElement> Get(const std::string_view& name)
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

        auto element = std::dynamic_pointer_cast<TElement, IElement>(findResult->second);

        wt::Assert(element != nullptr, "Invalid element cast");

        return element;
    };

    template<typename TElement>
    const std::shared_ptr<TElement> Get(const std::string_view& name) const
    {
        auto element = Get<TElement>(name);

        return element;
    };


private:

    std::vector<std::pair<std::string, std::shared_ptr<IElement>>> CreateLayout(RawLayout& rawLayout, std::size_t offset = 0) const
    {
        std::vector<std::pair<std::string, std::shared_ptr<IElement>>> layoutResult;

        std::size_t currentOffset = offset;


        for(const auto& [name, element] : rawLayout._layoutElements)
        {
            if(element->GetElementType() == DataType::Array)
            {
                auto arrayElement = std::dynamic_pointer_cast<ArrayElement, IElement>(element);

                if(arrayElement->GetArrayElementType() == DataType::Struct)
                {
                    RawLayout rawArrayStructLayout;

                    for(std::size_t i = 0; i < arrayElement->GetElementCount(); ++i)
                    {
                        auto rawStructLayout = std::dynamic_pointer_cast<StructElement, IElement>(rawArrayStructLayout.Add(DataType::Struct, std::string("[").append(std::to_string(i)).append("]")));

                        auto structLayout = std::dynamic_pointer_cast<StructElement, IElement>(arrayElement->_arrayElements[0]);

                        for(auto& [structElementName, structElement] : structLayout->_structElements)
                        {
                            rawStructLayout->Add(structElement->_elementType, structElementName);
                        };
                    };

                    const auto arrayStructLayout = CreateLayout(rawArrayStructLayout, currentOffset);


                    arrayElement->_arrayElements.clear();

                    for(auto& arrayStructElement : arrayStructLayout)
                    {
                        arrayElement->_arrayElements.emplace_back(arrayStructElement.second);
                    };

                    const auto arrayStructLayoutEnd = std::prev((arrayStructLayout.cend()));
                    currentOffset = arrayStructLayoutEnd->second->_offset + arrayStructLayoutEnd->second->_sizeInBytes;
                }
                else
                {
                    const std::size_t arrayElementSizeInBytes = DataTypeSizeInBytes(arrayElement->_arrayElementType);
                    arrayElement->_offset = GetCorrectOffset(currentOffset, arrayElementSizeInBytes);
                    arrayElement->_sizeInBytes = arrayElementSizeInBytes * arrayElement->_arrayElementCount;

                    RawLayout rawArrayLayout;

                    for(std::size_t i = 0; i < arrayElement->_arrayElementCount; ++i)
                    {
                        rawArrayLayout.Add(arrayElement->_arrayElementType, std::string("[").append(std::to_string(i)).append("]"));
                    };

                    const auto arrayLayout = CreateLayout(rawArrayLayout, currentOffset);

                    for(auto& arrayLayoutElement : arrayLayout)
                    {
                        arrayElement->_arrayElements.emplace_back(arrayLayoutElement.second);
                    };

                    const auto arrayLayoutEnd = std::prev((arrayLayout.cend()));
                    currentOffset = arrayLayoutEnd->second->_offset + arrayElementSizeInBytes;
                };

                layoutResult.emplace_back(std::make_pair(name, arrayElement));
            }
            else if(element->GetElementType() == DataType::Struct)
            {
                auto elementAsStruct = std::dynamic_pointer_cast<StructElement, IElement>(element);


                RawLayout rawStructLayout;

                for(auto& [structElementName, structElement] : elementAsStruct->_structElements)
                {
                    rawStructLayout.Add(structElement->_elementType, structElementName);
                };

                // TODO: Find a way to optimize struct size 

                // Calculate struct size
                const auto structLayoutInitial = CreateLayout(rawStructLayout, 0);

                auto structLayoutEnd = std::prev(structLayoutInitial.end());

                const std::size_t structSize = structLayoutEnd->second->_offset + structLayoutEnd->second->_sizeInBytes;


                elementAsStruct->_offset = GetCorrectOffset(currentOffset, structSize);
                currentOffset = elementAsStruct->_offset;

                elementAsStruct->_sizeInBytes = structSize;


                const auto structLayoutFinal = CreateLayout(rawStructLayout, currentOffset);

                elementAsStruct->_structElements.clear();

                for(auto& structLayoutElement : structLayoutFinal)
                {
                    elementAsStruct->_structElements.emplace_back(structLayoutElement);
                };

                structLayoutEnd = std::prev(structLayoutFinal.cend());
                currentOffset = structLayoutEnd->second->_offset + structLayoutEnd->second->_sizeInBytes;


                layoutResult.emplace_back(std::make_pair(name, elementAsStruct));
            }
            else
            {
                element->_offset = GetCorrectOffset(currentOffset, element->_sizeInBytes);
                currentOffset = element->_offset + element->_sizeInBytes;

                layoutResult.emplace_back(std::make_pair(name, element));
            };


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

            rawLayout.Add<ScalarElement, DataType::UInt32>("Uint_off_0");
            rawLayout.Add<ScalarElement, DataType::Vec4f>("Vec4_off_16");


            SSBOLayout layout = SSBOLayout (rawLayout);

            if(layout.Get<ScalarElement>("Uint_off_0")->GetOffset() != 0)
                __debugbreak();
            if(layout.Get<ScalarElement>("Vec4_off_16")->GetOffset() != 16)
                __debugbreak();
        };


        // Contigious elements
        {
            RawLayout rawLayout;

            rawLayout.Add<ScalarElement, DataType::UInt32>("Uint_off_0");
            rawLayout.Add<ScalarElement, DataType::UInt32>("Uint_off_4");
            rawLayout.Add<ScalarElement, DataType::UInt32>("Uint_off_8");
            rawLayout.Add<ScalarElement, DataType::UInt32>("Uint_off_12");
            rawLayout.Add<ScalarElement, DataType::UInt32>("Uint_off_16");


            SSBOLayout layout = SSBOLayout (rawLayout);

            if(layout.Get<ScalarElement>("Uint_off_0")->GetOffset() != 0)
                __debugbreak();
            if(layout.Get<ScalarElement>("Uint_off_4")->GetOffset() != 4)
                __debugbreak();
            if(layout.Get<ScalarElement>("Uint_off_8")->GetOffset() != 8)
                __debugbreak();
            if(layout.Get<ScalarElement>("Uint_off_12")->GetOffset() != 12)
                __debugbreak();
            if(layout.Get<ScalarElement>("Uint_off_16")->GetOffset() != 16)
                __debugbreak();
            int _ = 0;
        };


        {
            RawLayout rawLayout;

            rawLayout.Add<ScalarElement, DataType::Vec2f>("Vec2_off_0");
            rawLayout.Add<ScalarElement, DataType::UInt32>("Uint_off_8");

            SSBOLayout layout = SSBOLayout (rawLayout);

            if(layout.Get<ScalarElement>("Vec2_off_0")->GetOffset() != 0)
                __debugbreak();
            if(layout.Get<ScalarElement>("Uint_off_8")->GetOffset() != 8)
                __debugbreak();
        };


        {
            RawLayout rawLayout;

            rawLayout.Add<ScalarElement, DataType::Vec2f>("Vec2_off_0");
            rawLayout.Add<ScalarElement, DataType::Vec2f>("Vec2_off_8");

            SSBOLayout layout = SSBOLayout (rawLayout);

            if(layout.Get<ScalarElement>("Vec2_off_0")->GetOffset() != 0)
                __debugbreak();
            if(layout.Get<ScalarElement>("Vec2_off_8")->GetOffset() != 8)
                __debugbreak();
        };


        // Array 
        {
            RawLayout rawLayout;

            auto rawArrayElement = rawLayout.Add<ArrayElement, DataType::Array>("Uint_off_0");

            rawArrayElement->SetArray(DataType::UInt32, 4);

            SSBOLayout layout = SSBOLayout(rawLayout);

            auto arrayElement = layout.Get<ArrayElement>("Uint_off_0");

            auto s = layout.Get<ArrayElement>("Uint_off_0");

            if(layout.Get<ArrayElement>("Uint_off_0")->GetOffset() != 0)
                __debugbreak();

            if(arrayElement->GetAtIndex<ScalarElement>(0)->GetOffset() != 0)
                __debugbreak();
            if(arrayElement->GetAtIndex<ScalarElement>(1)->GetOffset() != 4)
                __debugbreak();
            if(arrayElement->GetAtIndex<ScalarElement>(2)->GetOffset() != 8)
                __debugbreak();
            if(arrayElement->GetAtIndex<ScalarElement>(3)->GetOffset() != 12)
                __debugbreak();

        };

        // Array 2
        {
            RawLayout rawLayout;

            auto rawArrayElement = rawLayout.Add<ArrayElement, DataType::Array>("Uint_off_0");

            rawArrayElement->SetArray(DataType::UInt32, 3);

            rawLayout.Add<ScalarElement, DataType::UInt32>("Uint_off_12");

            SSBOLayout layout = SSBOLayout(rawLayout);

            auto arrayElement = layout.Get<ArrayElement>("Uint_off_0");

            if(layout.Get<ArrayElement>("Uint_off_0")->GetOffset() != 0)
                __debugbreak();

            if(arrayElement->GetAtIndex<ScalarElement>(0)->GetOffset() != 0)
                __debugbreak();
            if(arrayElement->GetAtIndex<ScalarElement>(1)->GetOffset() != 4)
                __debugbreak();
            if(arrayElement->GetAtIndex<ScalarElement>(2)->GetOffset() != 8)
                __debugbreak();

            if(layout.Get<ScalarElement>("Uint_off_12")->GetOffset() != 12)
                __debugbreak();
        };


        // Array with padding
        {
            RawLayout rawLayout;

            rawLayout.Add<ScalarElement, DataType::UInt32>("Uint_off_0");

            auto rawArrayElement = rawLayout.Add<ArrayElement, DataType::Array>("Array_off_16");

            rawArrayElement->SetArray(DataType::Vec4f, 3);

            SSBOLayout layout = SSBOLayout(rawLayout);

            auto arrayElement = layout.Get<ArrayElement>("Array_off_16");


            if(layout.Get<ScalarElement>("Uint_off_0")->GetOffset() != 0)
                __debugbreak();

            if(layout.Get<ArrayElement>("Array_off_16")->GetOffset() != 16)
                __debugbreak();

            if(arrayElement->GetAtIndex<ScalarElement>(0)->GetOffset() != 16)
                __debugbreak();
            if(arrayElement->GetAtIndex<ScalarElement>(1)->GetOffset() != 32)
                __debugbreak();
            if(arrayElement->GetAtIndex<ScalarElement>(2)->GetOffset() != 48)
                __debugbreak();

        };


        // Struct
        {
            RawLayout structLayout;

            structLayout.Add<ScalarElement, DataType::UInt32>("Uint_off_0");

            auto rawStructElement = structLayout.Add<StructElement, DataType::Struct>("Test_off_16");

            rawStructElement->Add<ScalarElement, DataType::UInt32>("Test_Uint_off_16");
            rawStructElement->Add<ScalarElement, DataType::Vec4f>("Test_Vec4_off_32");

            structLayout.Add<ScalarElement, DataType::UInt32>("Uint_off_48");

            SSBOLayout layout = SSBOLayout(structLayout);

            auto structElement = layout.Get<StructElement>("Test_off_16");


            if(layout.Get<ScalarElement>("Uint_off_0")->GetOffset() != 0)
                __debugbreak();

            if(layout.Get<StructElement>("Test_off_16")->GetOffset() != 16)
                __debugbreak();

            if(structElement->Get<ScalarElement>("Test_Uint_off_16")->GetOffset() != 16)
                __debugbreak();

            if(structElement->Get<ScalarElement>("Test_Vec4_off_32")->GetOffset() != 32)
                __debugbreak();

            if(layout.Get<ScalarElement>("Uint_off_48")->GetOffset() != 48)
                __debugbreak();
        };


        // Multiple structs
        {
            RawLayout structLayout;

            structLayout.Add<ScalarElement, DataType::UInt32>("Uint_off_0");

            auto rawStructElement = structLayout.Add<StructElement, DataType::Struct>("Test_off_16");

            rawStructElement->Add<ScalarElement, DataType::UInt32>("Test_Uint_off_16");
            rawStructElement->Add<ScalarElement, DataType::Vec4f>("Test_Vec4_off_32");


            auto rawStructElement2 = structLayout.Add<StructElement, DataType::Struct>("Test2_off_48");

            rawStructElement2->Add<ScalarElement, DataType::UInt32>("Test2_Uint_off_48");
            rawStructElement2->Add<ScalarElement, DataType::Vec4f>("Test2_Mat4_off_64");


            SSBOLayout layout = SSBOLayout(structLayout);

            auto structElement = layout.Get<StructElement>("Test_off_16");
            auto structElement2 = layout.Get<StructElement>("Test2_off_48");


            if(layout.Get<ScalarElement>("Uint_off_0")->GetOffset() != 0)
                __debugbreak();

            if(layout.Get<StructElement>("Test_off_16")->GetOffset() != 16)
                __debugbreak();

            if(structElement->Get<ScalarElement>("Test_Uint_off_16")->GetOffset() != 16)
                __debugbreak();

            if(structElement->Get<ScalarElement>("Test_Vec4_off_32")->GetOffset() != 32)
                __debugbreak();

            if(layout.Get<StructElement>("Test2_off_48")->GetOffset() != 48)
                __debugbreak();

            if(structElement2->Get<ScalarElement>("Test2_Uint_off_48")->GetOffset() != 48)
                __debugbreak();

            if(structElement2->Get<ScalarElement>("Test2_Mat4_off_64")->GetOffset() != 64)
                __debugbreak();


            int _ = 0;

        };


        // Array of structs
        {
            RawLayout structArrayLayout;

            auto rawArrayElement = structArrayLayout.Add<ArrayElement, DataType::Array>("Test_off_0");

            auto arrayType = rawArrayElement->SetCustomArrayType(5);

            arrayType->Add<ScalarElement, DataType::UInt32>("Test_Uint_off_0");
            arrayType->Add<ScalarElement, DataType::Vec4f>("Test_Vec4_off_16");


            structArrayLayout.Add<ScalarElement, DataType::UInt32>("Uint_off_160");


            SSBOLayout layout = SSBOLayout(structArrayLayout);

            auto structArray = layout.Get<ArrayElement>("Test_off_0");

            if(structArray->GetOffset() != 0)
                __debugbreak();

            for(std::size_t i = 0; i < 5; ++i)
            {
                if(structArray->GetAtIndex<StructElement>(i)->GetOffset() != i * 32)
                    __debugbreak();


                if(structArray->GetAtIndex<StructElement>(i)->Get<ScalarElement>("Test_Uint_off_0")->GetOffset() != i * 32)
                    __debugbreak();

                if(structArray->GetAtIndex<StructElement>(i)->Get<ScalarElement>("Test_Vec4_off_16")->GetOffset() != (i * 32) + 16)
                    __debugbreak();
            };

            if(layout.Get<ScalarElement>("Uint_off_160")->GetOffset() != 160)
                __debugbreak();

        };


        // Array of structs with padding
        {
            RawLayout structArrayLayout;

            structArrayLayout.Add<ScalarElement, DataType::UInt32>("Uint_off_0");

            auto rawArrayElement = structArrayLayout.Add<ArrayElement, DataType::Array>("Test_off_16");

            auto arrayType = rawArrayElement->SetCustomArrayType(5);

            arrayType->Add<ScalarElement, DataType::UInt32>("Test_Uint_off_16");
            arrayType->Add<ScalarElement, DataType::Vec4f>("Test_Vec4_off_32");

            structArrayLayout.Add<ScalarElement, DataType::UInt32>("Uint_off_176");


            SSBOLayout layout = SSBOLayout(structArrayLayout);

            auto structArray = layout.Get<ArrayElement>("Test_off_16");

            if(structArray->GetOffset() != 0)
                __debugbreak();

            for(std::size_t i = 0; i < 5; ++i)
            {
                if(structArray->GetAtIndex<StructElement>(i)->GetOffset() != (i * 32) + 16)
                    __debugbreak();


                if(structArray->GetAtIndex<StructElement>(i)->Get<ScalarElement>("Test_Uint_off_16")->GetOffset() != (i * 32) + 16)
                    __debugbreak();

                if(structArray->GetAtIndex<StructElement>(i)->Get<ScalarElement>("Test_Vec4_off_32")->GetOffset() != ((i * 32) + 16) + 16)
                    __debugbreak();
            };

            if(layout.Get<ScalarElement>("Uint_off_176")->GetOffset() != 176)
                __debugbreak();

        };


        // Struct test
        {
            RawLayout rawLayout;

            rawLayout.Add<ScalarElement, DataType::UInt32>("Uint_off_0");

            auto rawStructElement = rawLayout.Add<StructElement, DataType::Struct>("Struct_off_4");

            rawStructElement->Add < ScalarElement, DataType::UInt32>("Struct.Uint_off_4");

            SSBOLayout layout = SSBOLayout(rawLayout);

            if(layout.Get<ScalarElement>("Uint_off_0")->GetOffset() != 0)
                __debugbreak();

            if(layout.Get<StructElement>("Struct_off_4")->GetOffset() != 4)
                __debugbreak();


            if(layout.Get<StructElement>("Struct_off_4")->Get<ScalarElement>("Struct.Uint_off_4")->GetOffset() != 4)
                __debugbreak();

        };

        int _ = 0;
    };


    {
        RawLayout rawLayout;

        auto rawStructLayout = rawLayout.Add<StructElement>("Test_Struct");

        rawStructLayout->Add<ScalarElement, DataType::UInt32>("Test_Struct.Uint32");

        SSBOLayout layout = SSBOLayout(rawLayout);

        layout.Get<StructElement>("Test_Struct")->Get<ScalarElement>("Test_Struct.Uint32")->Set(1u);
    };

};



constexpr std::size_t CalculateOffset(std::size_t rawOffset)
{
    const std::size_t correctedOffset = rawOffset + (16u - rawOffset % 16u) % 16u;

    return correctedOffset;
};

constexpr bool CrossesBoundary(std::size_t offset, std::size_t sizeInBytes)
{
    const std::size_t end = offset + sizeInBytes;

    const std::size_t pageStart = offset / 16u;
    const std::size_t pageEnd = end / 16u;

    return ((pageStart != pageEnd) && (end % 16 != 0u)) || (sizeInBytes > 16u);
};

constexpr std::size_t GetCorrectOffset(std::size_t offset, std::size_t sizeInBytes)
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
