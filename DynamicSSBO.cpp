#include "DynamicSSBO.hpp"


/// <summary>
/// This function should only be ever called from SSBOLayout.
/// The functions above are to stop us from shooting ourselves in the foot.
/// 
/// Adds an element, dynamicly, in runtime
/// </summary>
/// <param name="dataType"> The data-type of the new element </param>
/// <param name="name"> The name of the new element </param>
/// <returns></returns>
std::shared_ptr<IElement> StructElement::Add(DataType dataType, const std::string_view& name)
{
    // Ensure that the element's name isn't empty
    wt::Assert(name.empty() == false, []()
    {
        return "Invalid member name";
    });

    wt::Assert(_elementType == DataType::Struct, []()
    {
        return "Trying to add struct member to non-struct element";
    });

    // Ensure that element's data-type is defined
    wt::Assert(dataType != DataType::None, []()
    {
        return "Invalid data type";
    });

    // Ensure that there are no duplicates
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


    // Add the element depending on if it's a struct, array, or a scalar
    if(dataType == DataType::Struct)
    {
        auto& elementResult = _structElements.emplace_back(std::make_pair(name, std::make_shared<StructElement>()));
        return elementResult.second;
    }
    else if(dataType == DataType::Array)
    {
        // This one line is the reason this file exists...
        auto& elementResult = _structElements.emplace_back(std::make_pair(name, std::make_shared<ArrayElement>()));

        return elementResult.second;
    };

    auto& elementResult = _structElements.emplace_back(std::make_pair(name, std::make_shared<ScalarElement>(dataType)));
    return elementResult.second;
};
