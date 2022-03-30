#include "DynamicSSBO.hpp"


std::shared_ptr<IElement> StructElement::Add(DataType dataType, const std::string_view& name)
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
        // This one line is the reason this file exists...
        auto elementResult = _structElements.emplace_back(std::make_pair(name, std::make_shared<ArrayElement>()));
        return elementResult.second;
    };

    auto elementResult = _structElements.emplace_back(std::make_pair(name, std::make_shared<ScalarElement>(dataType)));
    return elementResult.second;
};
