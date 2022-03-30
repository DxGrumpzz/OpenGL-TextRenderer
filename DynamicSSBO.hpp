#pragma once

#include <cstddef>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include "WindowsUtilities.hpp"


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


static constexpr std::size_t DataTypeSizeInBytes(DataType type)
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

class ArrayElement;
class StructElement : public IElement
{
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
    requires std::derived_from<TElement, IElement>
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
    requires std::derived_from<TElement, IElement>
        const std::shared_ptr<TElement> Get(const std::string_view& name) const
    {
        return Get<TElement>(name);
    };



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
            const auto  elementResult = Add(DataType::Struct, name);
            return std::dynamic_pointer_cast<ArrayElement, IElement>(elementResult);
        }
        else
        {
            static_assert(false, "Incompatable (Scalar) element types");
        };
    };



private:

    std::shared_ptr<IElement> Add(DataType dataType, const std::string_view& name);

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
    requires std::derived_from<TElement, IElement>
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
    requires std::derived_from<TElement, IElement>
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

    constexpr DataType GetArrayElementType()
    {
        return _arrayElementType;
    };

    constexpr std::size_t GetElementCount()
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

    SSBOLayout(RawLayout& rawLayout)
    {
        const auto layout = CreateLayout(rawLayout);
        _layoutElements.insert(layout.begin(), layout.end());
    };


public:

    template<typename TElement>
    requires std::derived_from<TElement, IElement>
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
    requires std::derived_from<TElement, IElement>
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
                            rawStructLayout->Add(structElement->GetElementType(), structElementName);
                        };
                    };

                    const auto arrayStructLayout = CreateLayout(rawArrayStructLayout, currentOffset);


                    arrayElement->_arrayElements.clear();

                    for(auto& arrayStructElement : arrayStructLayout)
                    {
                        arrayElement->_arrayElements.emplace_back(arrayStructElement.second);
                    };

                    const auto arrayStructLayoutEnd = std::prev((arrayStructLayout.cend()));
                    currentOffset = arrayStructLayoutEnd->second->GetOffset() + arrayStructLayoutEnd->second->GetSizeInBytes();
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
                    currentOffset = arrayLayoutEnd->second->GetOffset() + arrayElementSizeInBytes;
                };

                layoutResult.emplace_back(std::make_pair(name, arrayElement));
            }
            else if(element->GetElementType() == DataType::Struct)
            {
                auto elementAsStruct = std::dynamic_pointer_cast<StructElement, IElement>(element);


                RawLayout rawStructLayout;

                for(auto& [structElementName, structElement] : elementAsStruct->_structElements)
                {
                    rawStructLayout.Add(structElement->GetElementType(), structElementName);
                };

                // TODO: Find a way to optimize struct size 

                // Calculate struct size
                const auto structLayoutInitial = CreateLayout(rawStructLayout, 0);

                auto structLayoutEnd = std::prev(structLayoutInitial.end());

                const std::size_t structSize = structLayoutEnd->second->GetOffset() + structLayoutEnd->second->GetSizeInBytes();


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
                currentOffset = structLayoutEnd->second->GetOffset() + structLayoutEnd->second->GetSizeInBytes();


                layoutResult.emplace_back(std::make_pair(name, elementAsStruct));
            }
            else
            {
                element->_offset = GetCorrectOffset(currentOffset, element->GetSizeInBytes());
                currentOffset = element->GetOffset() + element->GetSizeInBytes();

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



inline void SSBOTest()
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
};