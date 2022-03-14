#pragma once

#include <cstddef>
#include <cstdint>
#include <glad/glad.h>
#include <vector>
#include <unordered_map>
#include <string_view>

#include "WindowsUtilities.hpp"
#include "ShaderProgram.hpp"


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


/// <summary>
/// A wrapper class for SSBOs
/// </summary>
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

    template<typename T>
    void SetValue(const std::size_t offset, const T& value) const
    {
        glNamedBufferSubData(_bufferID, offset, sizeof(T), &value);
    };


    void SetValue(const std::size_t offset, const std::size_t sizeInBytes, const void* value) const
    {
        glNamedBufferSubData(_bufferID, offset, sizeInBytes, &value);
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
        // TODO: Arrays and struct are somewhat problematic, fix in sometime in the future

        // Get SSBO index
        GLint ssboIndex = glGetProgramResourceIndex(shaderProgram.GetProgramID(), GL_SHADER_STORAGE_BLOCK, ssboName.data());


        const bool assertResult = wt::Assert(ssboIndex != -1, [&]()
        {
            return std::string("SSBO \"").append(ssboName).append("\" was not found in prorgam \"").append(std::to_string(shaderProgram.GetProgramID()).append("\""));
        });

        if(assertResult == false)
            return false;


        // Get number of active SSBO variables
        static constexpr GLenum numberOfActiveVariablesProperty = GL_NUM_ACTIVE_VARIABLES;
        GLint numberOfVariables = 0;

        glGetProgramResourceiv(shaderProgram.GetProgramID(), GL_SHADER_STORAGE_BLOCK, ssboIndex, 1, &numberOfActiveVariablesProperty, 1, nullptr, &numberOfVariables);

        _ssboElements.reserve(numberOfVariables);


        // Get SSBO variable indices
        static constexpr  GLenum activeVariablesProperty = GL_ACTIVE_VARIABLES;
        std::vector<GLint> variableIndices = std::vector<GLint> (numberOfVariables);

        glGetProgramResourceiv(shaderProgram.GetProgramID(), GL_SHADER_STORAGE_BLOCK, ssboIndex, 1, &activeVariablesProperty, numberOfVariables, nullptr, variableIndices.data());


        // Get SSBO variable offsets
        for(std::size_t i = 0; i < variableIndices.size(); ++i)
        {
            static constexpr GLenum topLevelArrayPropety = GL_TOP_LEVEL_ARRAY_SIZE;

            int topLevelArraySize = 0;
            glGetProgramResourceiv(shaderProgram.GetProgramID(), GL_BUFFER_VARIABLE, variableIndices[i], 1, &topLevelArrayPropety, sizeof(topLevelArraySize), nullptr, &topLevelArraySize);


            // Get SSBO variable array size
            int arraySize = 0;

            static constexpr GLenum arraySizeProperty = GL_ARRAY_SIZE;
            glGetProgramResourceiv(shaderProgram.GetProgramID(), GL_BUFFER_VARIABLE, variableIndices[i], 1, &arraySizeProperty, sizeof(arraySize), nullptr, &arraySize);

            // Length of variable name 
            int nameBufferLength = 0;

            static constexpr GLenum nameLengthProperty = GL_NAME_LENGTH;
            glGetProgramResourceiv(shaderProgram.GetProgramID(), GL_BUFFER_VARIABLE, variableIndices[i], 1, &nameLengthProperty, sizeof(nameBufferLength), nullptr, &nameBufferLength);

            // SSBO variable name
            std::string name;
            name.resize(static_cast<std::size_t>(nameBufferLength) - 1);

            glGetProgramResourceName(shaderProgram.GetProgramID(), GL_BUFFER_VARIABLE, variableIndices[i], nameBufferLength, nullptr, name.data());


            GLint offset = 0;

            // SSBO variable offset
            static constexpr GLenum offsetProperty = GL_OFFSET;
            glGetProgramResourceiv(shaderProgram.GetProgramID(), GL_BUFFER_VARIABLE, variableIndices[i], 1, &offsetProperty, sizeof(GLint), nullptr, &offset);


            _ssboElements.insert(std::make_pair(name, SSBOElement(offset, arraySize)));
        };

        return true;
    };

};