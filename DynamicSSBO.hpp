#pragma once

#include <cstddef>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glad/glad.h>

#include "WindowsUtilities.hpp"

/// <summary>
/// An enumeration of available data-types
/// </summary>
enum class DataType
{
    /// <summary>
    /// 4-byte unsigned integer
    /// </summary>
    UInt32,

    /// <summary>
    /// Two component 32-bit float vector
    /// </summary>
    Vec2f,

    /// <summary>
    /// Two component unsigned 32-bit integer vector
    /// </summary>
    Vec2ui,

    /// <summary>
    /// Four component 32-bit float vector
    /// </summary>
    Vec4f,

    /// <summary>
    /// Four component unsigned 32-bit integer vector
    /// </summary>
    Vec4ui,

    /// <summary>
    /// Four by four matrix of floats
    /// </summary>
    Mat4f,

    /// <summary>
    /// Abstract array, needs to be defined after declaration
    /// </summary>
    Array,

    /// <summary>
    /// Abstract structure, needs to be defined after declaration
    /// </summary>
    Struct,

    None,
};

/// <summary>
/// Convert a DataType to it's corresponding size in bytes
/// </summary>
/// <param name="type"> The DataType to convert </param>
/// <returns></returns>
inline constexpr std::size_t DataTypeSizeInBytes(DataType type)
{
    switch(type)
    {
        case DataType::UInt32:
            return sizeof(std::uint32_t);

        case DataType::Vec2f:
            return sizeof(glm::vec2);

        case DataType::Vec2ui:
            return sizeof(glm::u32vec2);

        case DataType::Vec4f:
            return sizeof(glm::vec4);

        case DataType::Vec4ui:
            return sizeof(glm::u32vec4);

        case DataType::Mat4f:
            return sizeof(glm::mat4);


        case DataType::Array:
        case DataType::Struct:
            return 0;

        default:
        {
            wt::Assert(false, "No such type");
            __debugbreak();
            return static_cast<std::size_t>(-1);
        };
    };
};



// Forward declarations
class SSBOLayout;
class ArrayElement;

/// <summary>
/// An *interface that defines an element.
/// </summary>
class IElement
{
    // I'm using friend here becase all these components/classes, should be thought of as semi-interconnected parts of a larger system
    friend class SSBOLayout;

protected:

    /// <summary>
    /// Offset of this element from the start of the declaration
    /// </summary>
    std::size_t _offset = 0;

    /// <summary>
    /// The data type associated with this element
    /// </summary>
    DataType _elementType = DataType::None;

    /// <summary>
    /// The element's size in bytes
    /// </summary>
    std::size_t _sizeInBytes = 0;


protected:

    IElement() = default;

    virtual ~IElement() = default;

public:

    std::size_t GetOffset() const
    {
        return _offset;
    };

    std::size_t GetSizeInBytes() const
    {
        return _sizeInBytes;
    };

    DataType GetElementType() const
    {
        return _elementType;
    };
};


/// <summary>
/// An element of a "simple" type, i.e. not an array, or a struct.
/// </summary>
class ScalarElement : public IElement
{
    // TODO: There has to be a better way to "feed" the buffer id to the elements
    friend class SSBOLayout;

private:

    /// <summary>
    /// The GL buffer ID associated with this layout
    /// </summary>
    std::uint32_t _bufferID = 0;

public:

    ScalarElement(DataType type)
    {
        _elementType = type;
        _sizeInBytes = DataTypeSizeInBytes(type);
    };

public:


    /// <summary>
    /// Updates this element's value in the associated SSBO, in bufferID. 
    /// (I know, it's not very ideal atm)
    /// </summary>
    /// <typeparam name="T"></typeparam>
    /// <param name="bufferID"></param>
    /// <param name="value"></param>
    template<typename T>
    void Set(const T& value) const
    {
        // Ensure T matches this element's DataType
        switch(_elementType)
        {
            case DataType::UInt32:
            {
                wt::Assert(std::is_same <T, std::uint32_t>::value == true, "Invalid value type. Expected \"DataType::UInt32\"");
                break;
            };

            case DataType::Vec2f:
            {
                wt::Assert(std::is_same <T, glm::vec2>::value == true, "Invalid value type. Expected \"DataType::Vec2f\"");
                break;
            };

            case DataType::Vec2ui:
            {
                wt::Assert(std::is_same <T, glm::u32vec2>::value == true, "Invalid value type. Expected \"DataType::Vec2ui\"");
                break;
            };

            case DataType::Vec4f:
            {
                wt::Assert(std::is_same <T, glm::vec4>::value == true, "Invalid value type. Expected \"DataType::Vec4f\"");
                break;
            };

            case DataType::Vec4ui:
            {
                wt::Assert(std::is_same <T, glm::u32vec4>::value == true, "Invalid value type. Expected \"DataType::Vec4ui\"");
                break;
            };

            case DataType::Mat4f:
            {
                wt::Assert(std::is_same <T, glm::mat4>::value == true, "Invalid value type. Expected \"DataType::Mat4f\"");
                break;
            };

            default:
                wt::Assert(false, "Invalid element");
        };

        // Update value in GPU
        glNamedBufferSubData(_bufferID, _offset, _sizeInBytes, &value);
    };
};

/// <summary>
/// A struct element, can contain 'n' elements, must be defined after creation.
/// Can access elements by name
/// </summary>
class StructElement : public IElement
{
    friend class SSBOLayout;

private:

    /// <summary>
    /// The list of element defined in this struct.
    /// Unfortunately, this cannot be an "std::map" due to the way "std::map"s behave on insertions.
    /// To make this a map, will take some re-work, and re-structure
    /// </summary>
    std::vector<std::pair<std::string, std::shared_ptr<IElement>>> _structElements;


public:

    StructElement()
    {
        _elementType = DataType::Struct;
        _sizeInBytes = DataTypeSizeInBytes(_elementType);
    };

public:


    /// <summary>
    /// Retrieves an Element, returns "nullptr" if none found.
    /// If an element was found, automatically converts the element to TElement.
    /// As long as TElement is derived from IElement
    /// </summary>
    /// <typeparam name="TElement"> The element type to cast into, must be derived from IElement </typeparam>
    /// <param name="name"> The name of the new element </param>
    template<typename TElement>
    requires std::derived_from<TElement, IElement>
        std::shared_ptr<TElement> Get(const std::string_view& name)
    {
        // Search for a match element name in "_structElements"
        for(auto& [elementName, structElement] : _structElements)
        {
            // If an element was found..
            if(elementName == name)
            {
                // Cast it to TElement
                auto castResult = std::dynamic_pointer_cast<TElement, IElement>(structElement);

                // Ensure cast didn't fail
                wt::Assert(castResult != nullptr, "Invalid element cast");

                return castResult;
            };
        };

        // Assert that an element was found
        wt::Assert(false, [&]()
        {
            return std::string("No such element \"").append(name).append("\" was found");
        });

        return nullptr;
    };


    /// <summary>
    /// Const overload of StructElement::Get()
    /// </summary>
    /// <typeparam name="TElement"> The element type to cast into, must be derived from IElement </typeparam>
    /// <param name="name"> The name of the new element </param>
    /// <returns></returns>
    template<typename TElement>
    requires std::derived_from<TElement, IElement>
        const std::shared_ptr<TElement> Get(const std::string_view& name) const
    {
        return Get<TElement>(name);
    };


    /// <summary>
    /// Adds a new scalar element to this struct
    /// </summary>
    /// <typeparam name="TElement"> The element type to add, must be derived from IElement </typeparam>
    /// <typeparam name="dataType"> The data-type to associate with the new element, if we're adding a scalar type </typeparam>
    /// <param name="name"> The name of the new element </param>
    /// <returns></returns>
    template<typename TElement, DataType dataType>
    requires std::derived_from<TElement, IElement>
        std::shared_ptr<TElement> Add(const std::string_view& name)
    {
        // If TElement is a struct..
        if constexpr(std::is_same<TElement, StructElement>::value == true &&
                     // Make sure that the dataType also is a struct
                     dataType != DataType::Struct)
        {
            static_assert(false, "Incompatable (Struct) element types");
        }
        // If TElement is an array..
        else if constexpr(std::is_same<TElement, ArrayElement>::value == true &&
                          // Make sure that the dataType also is a array
                          dataType != DataType::Array)
        {
            static_assert(false, "Incompatable (Array) element types");
        }
        // If TElement is a scalar type..
        else if constexpr(std::is_same<TElement, ScalarElement>::value == true &&
                          // Make sure it's not "none", a struct, or an array
                          (dataType == DataType::None || dataType == DataType::Struct || dataType == DataType::Array))
        {
            static_assert(false, "Incompatable (Scalar) element types");
        };


        const auto elementResult = Add(dataType, name);
        return std::dynamic_pointer_cast<TElement, IElement>(elementResult);
    };


    /// <summary>
    /// Adds an array element, or a struct element to this struct.
    /// </summary>
    /// <typeparam name="TElement"> The element type to add, must be derived from IElement </typeparam>
    /// <param name="name"> The name of the new element </param>
    /// <returns></returns>
    template<typename TElement>
    requires std::derived_from<TElement, IElement>
        std::shared_ptr<TElement> Add(const std::string_view& name)
    {
        // Add the new element depending on if it's a struct, or an array.
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
        // Do not add if it's a scalar type
        else
        {
            static_assert(false, "Incompatable (Scalar) element types");
        };
    };


private:

    /// <summary>
    /// This function should only be ever called from SSBOLayout.
    /// The functions above are to stop us from shooting ourselves in the foot.
    /// 
    /// Adds an element, dynamicly, in runtime
    /// </summary>
    /// <param name="dataType"> The data-type of the new element </param>
    /// <param name="name"> The name of the new element </param>
    /// <returns></returns>
    std::shared_ptr<IElement> Add(DataType dataType, const std::string_view& name);

};

/// <summary>
/// A array element, can contain 'n' elements, must be defined after creation.
/// Can access elements by index
/// </summary>
class ArrayElement : public IElement
{
    friend class SSBOLayout;

private:

    /// <summary>
    /// A list of elements defined in this array
    /// </summary>
    std::vector<std::shared_ptr<IElement>> _arrayElements;

    /// <summary>
    /// The array's element type
    /// </summary>
    DataType _arrayElementType = DataType::None;

    /// <summary>
    /// The number of elements in thes array
    /// </summary>
    std::size_t _arrayElementCount = static_cast<std::size_t>(-1);


public:

    ArrayElement()
    {
        _elementType = DataType::Array;
        _sizeInBytes = DataTypeSizeInBytes(_elementType);
    };

public:

    /// <summary>
    /// Retrieves an element, by index
    /// </summary>
    /// <typeparam name="TElement"> The element type to cast into if successful, must be derived from IElement </typeparam>
    /// <param name="index"> The index at which the element exists </param>
    template<typename TElement>
    requires std::derived_from<TElement, IElement>
        std::shared_ptr<TElement> GetAtIndex(const std::size_t index)
    {
        wt::Assert(_elementType == DataType::Array, []()
        {
            return "Cannot index into non-array element";
        });

        // Ensure element list isn't empty
        wt::Assert(_arrayElements.empty() == false, []()
        {
            return "Element array is empty";
        });

        // Ensure we're not out of bounds
        wt::Assert(index >= 0 && index < _arrayElements.size(),
                   []()
        {
            return "Invalid index";
        });


        std::shared_ptr<IElement> element = _arrayElements[index];
        return std::dynamic_pointer_cast<TElement, IElement>(element);
    };

    /// <summary>
    /// Const overload of ArrayElement::GetAtIndex()
    /// </summary>
    template<typename TElement>
    requires std::derived_from<TElement, IElement>
        const std::shared_ptr<TElement> GetAtIndex(const std::size_t index) const
    {
        return GetAtIndex<TElement>(index);
    };


    /// <summary>
    /// Set the type of element this array will use, must be a scalar data-type
    /// </summary>
    /// <param name="arrayType"> The element type this array will use </param>
    /// <param name="elementCount"> Number of elements in this array </param>
    void SetArray(const DataType arrayType, const std::size_t elementCount)
    {
        wt::Assert(elementCount >= 1, []()
        {
            return "Invalid array size";
        });

        wt::Assert(arrayType != DataType::None, []()
        {
            return "Invalid data type";
        });

        wt::Assert(arrayType != DataType::Struct, []()
        {
            return "Invalid type, did you mean to call ArrayElement::SetCustomArrayType()?";
        });

        _arrayElementType = arrayType;
        _arrayElementCount = elementCount;
    };

    /// <summary>
    /// Set a custom array type, i.e. a struct.
    /// Returns a StructElement, which is used to define further the structure of the custom element
    /// </summary>
    /// <param name="elementCount"> Number of elements in this array </param>
    /// <returns></returns>
    std::shared_ptr<StructElement> SetCustomArrayType(const std::size_t elementCount)
    {
        // Set the array's element type to a struct
        _arrayElementType = DataType::Struct;
        _arrayElementCount = elementCount;

        // If we want to define an array of structs, we have to somehow store the strucutre of the struct.
        // We add StructElement as the first element of the array list, this element will be used to defined the strucutre 
        // of the rest of the array, and subsequent elements (Elements after this array)

        return std::dynamic_pointer_cast<StructElement, IElement>(_arrayElements.emplace_back(std::make_shared<StructElement>()));
    };


public:

    constexpr DataType GetArrayElementType()
    {
        return _arrayElementType;
    };

    constexpr std::size_t GetElementCount()
    {
        return _arrayElementCount;
    };
};


/// <summary>
/// A raw layout of elements. 
/// Used only to define an "abstract" layout of elements. 
/// </summary>
class RawLayout
{
    friend class SSBOLayout;

private:

    /// <summary>
    /// A layout of elements
    /// </summary>
    std::vector<std::pair<std::string, std::shared_ptr<IElement>>> _layoutElements;

public:

    /// <summary>
    /// Add a new element to the layout
    /// </summary>
    /// <typeparam name="TElement"> The element type to add, must be derived from IElement </typeparam>
    /// <typeparam name="dataType"> The data-type to associate with the new element, if we're adding a scalar type </typeparam>
    /// <param name="name"> The name of the new element </param>
    /// <returns></returns>
    template<typename TElement, DataType dataType>
    requires std::derived_from<TElement, IElement>
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

    /// <summary>
    /// Add a new struct, or array element to the layout
    /// </summary>
    /// <typeparam name="TElement"> The element type to add, must be derived from IElement, and not a scalar type </typeparam>
    /// <param name="name"> The name of the new element </param>
    /// <returns></returns>
    template<typename TElement>
    requires std::derived_from<TElement, IElement>
        std::shared_ptr<TElement> Add(const std::string_view& name)
    {
        if constexpr(std::is_same<TElement, StructElement>::value == true)
        {
            const auto elementResult = Add(DataType::Struct, name);
            return std::dynamic_pointer_cast<StructElement, IElement>(elementResult);
        }
        else if constexpr(std::is_same<TElement, ArrayElement>::value == true)
        {
            const auto  elementResult = Add(DataType::Array, name);
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


/// <summary>
/// The finalized layout of elements.
/// This contains the correct offsets, alignments, and padding
/// </summary>
class SSBOLayout
{
private:

    /// <summary>
    /// Total size in bytes of this layout
    /// </summary>
    std::size_t _sizeInBytes = 0;

    /// <summary>
    /// The list of elements associated with this layout
    /// </summary>
    std::unordered_map<std::string, std::shared_ptr<IElement>> _layoutElements;

    /// <summary>
    /// The GL buffer ID associated with this layout
    /// </summary>
    std::uint32_t _bufferID = 0;

    /// <summary>
    /// Which binding point the GL buffer is bound to
    /// </summary>
    std::uint32_t _bindingPoint = 0;


public:


    SSBOLayout(RawLayout& rawLayout, std::uint32_t bindingPoint = 0, std::uint32_t usage = GL_DYNAMIC_COPY) :
        _bindingPoint(bindingPoint)
    {

        // Create the GL buffer
        glCreateBuffers(1, &_bufferID);
        Bind();

        // Create the element layout
        const auto layout = CreateLayout(rawLayout);

        // Get the last element, so we can calculate the total size in bytes
        const auto& endElement = layout.back();
        _sizeInBytes = endElement.second->GetOffset() + endElement.second->GetSizeInBytes();

        // Add the completed list of elements to the layout list
        _layoutElements.insert(layout.begin(), layout.end());

        // Set the buffer's data size
        glNamedBufferData(_bufferID, _sizeInBytes, nullptr, usage);
    };


    SSBOLayout(SSBOLayout&& movedSSBOLayout) noexcept
    {
        std::swap(_sizeInBytes, movedSSBOLayout._sizeInBytes);
        std::swap(_bufferID, movedSSBOLayout._bufferID);
        std::swap(_bindingPoint, movedSSBOLayout._bindingPoint);

        std::swap(_layoutElements, movedSSBOLayout._layoutElements);
    };

    SSBOLayout(SSBOLayout&) = delete;



    ~SSBOLayout()
    {
        glDeleteBuffers(1, &_bufferID);
    };

public:

    void Bind() const
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, _bufferID);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, _bindingPoint, _bufferID);
    };


    /// <summary>
    /// Copy GL buffer data from "readLayout" into this buffer
    /// </summary>
    /// <param name="readLayout"> The buffer at which to copy from </param>
    void CopyBufferData(const SSBOLayout& readLayout) const
    {
        // Ensure that the current buffer is big enough to fit the data inside 'readLayout'
        wt::Assert(_sizeInBytes > readLayout.GetBufferID(), "Copy-Write buffer is too small");

        glCopyNamedBufferSubData(readLayout.GetBufferID(), _bufferID, 0, 0, readLayout.GetSizeInBytes());
    };


    /// <summary>
    /// Retrieve an element 
    /// </summary>
    /// <param name="name"> The name of the element to search for </param>
    template<typename TElement>
    requires std::derived_from<TElement, IElement>
        std::shared_ptr<TElement> Get(const std::string_view& name)
    {
        // Ensure that layout list isn't empty
        wt::Assert(_layoutElements.empty() == false, [&]()
        {
            return std::string("Layout is empty");
        });

        // Search the for the requested element
        auto findResult = _layoutElements.find(name.data());

        // Ensure element was found
        wt::Assert(findResult != _layoutElements.end(), [&]()
        {
            return std::string("No such element \"").append(name).append("\" was found");
        });


        auto element = std::dynamic_pointer_cast<TElement, IElement>(findResult->second);

        wt::Assert(element != nullptr, "Invalid element cast");

        return element;
    };


    /// <summary>
    /// const overload of SSBOLayout::Get()
    /// </summary>
    template<typename TElement>
    requires std::derived_from<TElement, IElement>
        const std::shared_ptr<TElement> Get(const std::string_view& name) const
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

        const auto element = std::dynamic_pointer_cast<TElement, IElement>(findResult->second);

        wt::Assert(element != nullptr, "Invalid element cast");

        return element;
    };



public:

    constexpr std::size_t GetSizeInBytes() const
    {
        return _sizeInBytes;
    };

    constexpr std::uint32_t GetBufferID() const
    {
        return _bufferID;
    };

public:

    SSBOLayout& operator = (SSBOLayout&) = delete;

    SSBOLayout& operator = (SSBOLayout&& movedSSBOLayout) noexcept
    {
        std::swap(_sizeInBytes, movedSSBOLayout._sizeInBytes);
        std::swap(_bufferID, movedSSBOLayout._bufferID);
        std::swap(_bindingPoint, movedSSBOLayout._bindingPoint);

        std::swap(_layoutElements, movedSSBOLayout._layoutElements);

        return *this;
    };


private:

    /// <summary>
    /// Creates a complete layout from a RawLayout
    /// </summary>
    /// <param name="rawLayout"> The initial layout </param>
    /// <param name="offset"> Initial (or relative) offset at which this function calculates other offsets </param>
    /// <returns></returns>
    std::vector<std::pair<std::string, std::shared_ptr<IElement>>> CreateLayout(RawLayout& rawLayout, std::size_t offset = 0) const
    {
        std::vector<std::pair<std::string, std::shared_ptr<IElement>>> layoutResult;

        std::size_t currentOffset = offset;

        // For every element in the rawlayout
        for(const auto& [name, element] : rawLayout._layoutElements)
        {
            // If the element is a array..
            if(element->GetElementType() == DataType::Array)
            {
                auto arrayElement = std::dynamic_pointer_cast<ArrayElement, IElement>(element);

                // When defining an array, we need to consider two (Three, soon... maybe) cases.
                // An array of a scalar element, and an Array of structs.


                // If the array is an array of structs..
                if(arrayElement->GetArrayElementType() == DataType::Struct)
                {
                    RawLayout rawArrayStructLayout;

                    // The structure of defintion of the array's struct 
                    const auto structLayout = std::dynamic_pointer_cast<StructElement, IElement>(arrayElement->_arrayElements[0]);

                    // For every array element "to-be"
                    for(std::size_t i = 0; i < arrayElement->GetElementCount(); ++i)
                    {
                        // Create a "yet-undefined" array element struct 
                        auto rawStructLayout = std::dynamic_pointer_cast<StructElement, IElement>(rawArrayStructLayout.Add(DataType::Struct, std::string("[").append(std::to_string(i)).append("]")));

                        // Add the struct's members from array, to the "yet-undefined" struct layout
                        for(auto& [structElementName, structElement] : structLayout->_structElements)
                        {
                            rawStructLayout->Add(structElement->GetElementType(), structElementName);
                        };
                    };


                    // Finalize the array layout
                    const auto arrayStructLayout = CreateLayout(rawArrayStructLayout, currentOffset);

                    // Clear the array, since the first element is no longer needed
                    arrayElement->_arrayElements.clear();

                    // Copy the correct elements into the actual array
                    for(auto& arrayStructElement : arrayStructLayout)
                    {
                        arrayElement->_arrayElements.emplace_back(arrayStructElement.second);
                    };

                    const auto arrayStructLayoutEnd = std::prev((arrayStructLayout.cend()));
                    currentOffset = arrayStructLayoutEnd->second->GetOffset() + arrayStructLayoutEnd->second->GetSizeInBytes();
                }
                // If the array is an array of scalars..
                else
                {
                    // The size of the array's type in bytes
                    const std::size_t arrayElementSizeInBytes = DataTypeSizeInBytes(arrayElement->_arrayElementType);

                    // Calculate array offset
                    arrayElement->_offset = GetCorrectOffset(currentOffset, arrayElementSizeInBytes);

                    // We can simply find the array's size in bytes by multiplying the array's type size in byte by the number of elements.
                    arrayElement->_sizeInBytes = arrayElementSizeInBytes * arrayElement->_arrayElementCount;


                    // Because we alreay take care of scalar elements, in a different part of this function,
                    // we can use some recursion to help us to finalize the structure of the array
                    RawLayout rawArrayLayout;

                    for(std::size_t i = 0; i < arrayElement->_arrayElementCount; ++i)
                    {
                        rawArrayLayout.Add(arrayElement->_arrayElementType, std::string("[").append(std::to_string(i)).append("]"));
                    };

                    // Finalize array layout
                    const auto arrayLayout = CreateLayout(rawArrayLayout, currentOffset);

                    for(auto& arrayLayoutElement : arrayLayout)
                    {
                        arrayElement->_arrayElements.emplace_back(arrayLayoutElement.second);
                    };

                    const auto arrayLayoutEnd = std::prev((arrayLayout.cend()));
                    currentOffset = arrayLayoutEnd->second->GetOffset() + arrayElementSizeInBytes;
                };

                layoutResult.emplace_back(std::make_pair(name, arrayElement));
            }
            // If the element is a struct..
            else if(element->GetElementType() == DataType::Struct)
            {
                auto elementAsStruct = std::dynamic_pointer_cast<StructElement, IElement>(element);

                // To get a correct offset for the struct's children, we first need to figure out it's size.
                // Luckily enough, most of the figuring out part can be done by some recursion, since all elements end-up being just scalar elements

                // Create a RawLayout with the struct's childrens as the element layout
                RawLayout rawStructLayout;
                for(auto& [structElementName, structElement] : elementAsStruct->_structElements)
                {
                    rawStructLayout.Add(structElement->GetElementType(), structElementName);
                };

                // Finalized (initial) struct layout, this doesn't yet use the right offsets, only the size
                const auto structLayoutInitial = CreateLayout(rawStructLayout, 0);

                // A pointer to the end of the list of finalized struct elements
                auto structLayoutEndPointer = std::prev(structLayoutInitial.end());
                const std::size_t structSize = structLayoutEndPointer->second->GetOffset() + structLayoutEndPointer->second->GetSizeInBytes();


                elementAsStruct->_sizeInBytes = structSize;
                elementAsStruct->_offset = GetCorrectOffset(currentOffset, structSize);

                // Update currect offset
                currentOffset = elementAsStruct->_offset;

                // (Actually) finalize the struct layout, this will result in the correct struct layout
                const auto structLayoutFinal = CreateLayout(rawStructLayout, currentOffset);

                // Because when we first added element to the struct, when we were defining it.
                // They are not longer needed, so we can clear them, and add the correct elements
                elementAsStruct->_structElements.clear();

                for(auto& structLayoutElement : structLayoutFinal)
                {
                    elementAsStruct->_structElements.emplace_back(structLayoutElement);
                };

                // Update current offset
                structLayoutEndPointer = std::prev(structLayoutFinal.cend());
                currentOffset = structLayoutEndPointer->second->GetOffset() + structLayoutEndPointer->second->GetSizeInBytes();

                layoutResult.emplace_back(std::make_pair(name, elementAsStruct));
            }
            // If the element is a scalar element..
            else
            {
                auto elementAsScalar = std::dynamic_pointer_cast<ScalarElement, IElement>(element);

                elementAsScalar->_bufferID = _bufferID;

                // Simply, calaculate the offset, and we're done
                element->_offset = GetCorrectOffset(currentOffset, element->GetSizeInBytes());
                currentOffset = element->GetOffset() + element->GetSizeInBytes();

                layoutResult.emplace_back(std::make_pair(name, element));
            };
        };

        return layoutResult;
    };

    /// <summary>
    /// Calculates an offset depending on if an element crosses a 16-byte boundary or not
    /// </summary>
    /// <param name="offset"> The offset at which the element starts from </param>
    /// <param name="size"> The element's size in bytes </param>
    /// <returns></returns>
    constexpr std::size_t GetCorrectOffset(std::size_t offset, std::size_t sizeInBytes) const
    {
        const bool crossesBoundary = CrossesBoundary(offset, sizeInBytes);

        if(crossesBoundary == true)
        {
            const std::size_t rawOffset = offset;

            const std::size_t actualOffset = CalculateBoundaryOffset(rawOffset);

            const std::size_t padding = actualOffset - offset;

            return actualOffset;
        }
        else
        {
            return offset;
        };
    };

    /// <summary>
    /// Calculates an offset-boundary given an initial offset
    /// </summary>
    /// <returns></returns>
    constexpr std::size_t CalculateBoundaryOffset(std::size_t offset) const
    {
        // I have no idea why, or how this works
        const std::size_t boundaryOffset = offset + (16u - offset % 16u) % 16u;

        return boundaryOffset;
    };

    /// <summary>
    /// Check if an element crosses a 16-byte boundary
    /// </summary>
    /// <param name="offset"> The offset at which the element starts from </param>
    /// <param name="size"> The element's size in bytes </param>
    /// <returns></returns>
    constexpr bool CrossesBoundary(std::size_t offset, std::size_t sizeInBytes) const
    {
        // Calculate the boundary-offset from "offset"
        const std::size_t boundaryOffset = CalculateBoundaryOffset(offset);

        // Check if the total size of the element crosses that boundary
        const bool crossesBoundary = ((offset + sizeInBytes) > boundaryOffset);

        return crossesBoundary;
    };

};
