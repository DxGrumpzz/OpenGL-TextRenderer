#pragma once

#include <cstdint>
#include <glad/glad.h>
#include <string>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <unordered_map>

#include "../WindowsUtilities.hpp"


/// <summary>
/// A class that encapsulates the functionality of a Shader program
/// </summary>
class ShaderProgram
{
private:

    /// <summary>
    /// A list of known, cached, Uniform locations
    /// </summary>
    mutable std::unordered_map<std::string, std::uint32_t> _uniformLocations;

    /// <summary>
    /// An identifier used by the API
    /// </summary>
    std::uint32_t _programID { 0 };


public:


    ShaderProgram(const std::string& vertexShaderPath,
                  const std::string& fragmentShaderPath)
    {
        // Compile shaders
        const std::uint32_t vertexShaderID = CompileVertexShader(vertexShaderPath);
        const std::uint32_t fragmentShaderID = CompileFragmentShader(fragmentShaderPath);

        // Link and create the GL program
        _programID = CreateAndLinkShaderProgram(vertexShaderID, fragmentShaderID);

        glDeleteShader(fragmentShaderID);
        glDeleteShader(vertexShaderID);
    };

    ShaderProgram(const ShaderProgram&) = delete;

    ~ShaderProgram()
    {
        if(_programID != 0)
            glDeleteProgram(_programID);
    };

    void Bind() const
    {
        glUseProgram(_programID);
    };


    void SetVector3(const std::string& name, const float value1, const float value2, const float value3) const
    {
        const std::uint32_t colourUniformLocation = GetUniformLocation(name);

        glUniform3f(colourUniformLocation, value1, value2, value3);
    };

    void SetVector3(const std::string& name, const glm::vec3& vector) const
    {
        SetVector3(name, vector.x, vector.y, vector.z);
    };

    void SetFloat(const std::string& name, const float& value) const
    {
        const std::uint32_t uniformLocation = GetUniformLocation(name);
        glUniform1f(uniformLocation, value);
    };

    void SetMatrix4(const std::string& name, const glm::mat4& matrix) const
    {
        const std::uint32_t uniformLocation = GetUniformLocation(name);

        glUniformMatrix4fv(uniformLocation, 1, false, glm::value_ptr(matrix));
    };

    void SetInt(const std::string& name, const int value) const
    {
        const std::uint32_t uniformLocation = GetUniformLocation(name);

        glUniform1i(uniformLocation, value);
    };

    void SetBool(const std::string& name, const bool value) const
    {
        SetInt(name, value);
    };


public:

    std::uint32_t GetProgramID() const
    {
        return _programID;
    };


private:

    /// <summary>
    /// Compiles a vertex shader
    /// </summary>
    /// <param name="filename"> The path to the vertex shader </param>
    /// <returns></returns>
    std::uint32_t CompileVertexShader(const std::string& filename) const
    {
        const std::string vertexShaderSource = ReadAllText(filename);

        std::uint32_t vertexShaderID = 0;
        vertexShaderID = glCreateShader(GL_VERTEX_SHADER);

        const char* vertexShaderSourcePointer = vertexShaderSource.c_str();
        const int vertexShaderSourceLength = static_cast<int>(vertexShaderSource.length());

        glShaderSource(vertexShaderID, 1, &vertexShaderSourcePointer, &vertexShaderSourceLength);
        glCompileShader(vertexShaderID);


        // Ensure compilation is successful  
        int success = 0;
        glGetShaderiv(vertexShaderID, GL_COMPILE_STATUS, &success);

        if(!success)
        {
            int bufferLength = 0;
            glGetShaderiv(vertexShaderID, GL_INFO_LOG_LENGTH, &bufferLength);

            std::string error;
            error.resize(bufferLength);

            glGetShaderInfoLog(vertexShaderID, bufferLength, &bufferLength, error.data());


            std::cerr << "Vertex shader compilation error:\n" << error << "\n";

            __debugbreak();
        };

        return vertexShaderID;
    };


    /// <summary>
    /// Compiles a fragment shader
    /// </summary>
    /// <param name="filename"> The path to the vertex shader </param>
    /// <returns></returns>
    std::uint32_t CompileFragmentShader(const std::string& filename) const
    {
        const std::string fragmentShaderSource = ReadAllText(filename);

        std::uint32_t fragmentShaderID = 0;
        fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

        const char* fragmentShaderSourcePointer = fragmentShaderSource.c_str();
        const int fragmentShaderSourceLength = static_cast<int>(fragmentShaderSource.length());

        glShaderSource(fragmentShaderID, 1, &fragmentShaderSourcePointer, &fragmentShaderSourceLength);
        glCompileShader(fragmentShaderID);


        // Ensure compilation is successful  
        int success = 0;
        glGetShaderiv(fragmentShaderID, GL_COMPILE_STATUS, &success);


        if(!success)
        {
            int bufferLength = 0;
            glGetShaderiv(fragmentShaderID, GL_INFO_LOG_LENGTH, &bufferLength);

            std::string error;
            error.resize(bufferLength);

            glGetShaderInfoLog(fragmentShaderID, bufferLength, &bufferLength, error.data());

            std::cerr << "Fragment shader compilation error:\n" << error << "\n";

            __debugbreak();
        };

        return fragmentShaderID;
    };

    /// <summary>
    /// Create a shader program and link compiled shader 
    /// </summary>
    /// <param name="vertexShaderID"></param>
    /// <param name="fragmentShaderID"></param>
    /// <returns></returns>
    std::uint32_t CreateAndLinkShaderProgram(const std::uint32_t vertexShaderID, const std::uint32_t fragmentShaderID) const
    {
        const std::uint32_t programID = glCreateProgram();

        glAttachShader(programID, vertexShaderID);
        glAttachShader(programID, fragmentShaderID);
        glLinkProgram(programID);

        // Ensure linkage is successful
        int success = 0;
        glGetProgramiv(programID, GL_LINK_STATUS, &success);

        if(!success)
        {
            int bufferLength = 0;
            glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &bufferLength);

            std::string error;
            error.resize(bufferLength);

            int bytesWritten = 0;
            glGetProgramInfoLog(programID, bufferLength, &bytesWritten, error.data());

            std::cerr << "Program link error:\n" << error << "\n";

            __debugbreak();
        };

        return programID;
    };


    /// <summary>
    /// Find the location of a uniform
    /// </summary>
    /// <param name="name"></param>
    /// <returns></returns>
    std::uint32_t GetUniformLocation(const std::string& name) const
    {
        // Check if uniform exists in cache
        const auto result = _uniformLocations.find(name);

        int uniformLocation = -1;

        // If name uniform isn't cached...
        if(result == _uniformLocations.cend())
        {
            // Find the uniform
            uniformLocation = glGetUniformLocation(_programID, name.c_str());

            if(uniformLocation == -1)
            {
                std::cerr << "Uniform location error: Unable to find \"" << name << "\"\n";
                __debugbreak();
            }
            else
            {
                // Add to cache
                _uniformLocations.insert(std::make_pair(name, static_cast<std::uint32_t>(uniformLocation)));
            };
        }
        // If uniform was found...
        else
        {
            uniformLocation = result->second;
        };

        return uniformLocation;
    };


    /// <summary>
    /// Read all text inside a file
    /// </summary>
    /// <param name="filename"> Path to sid file </param>
    /// <returns></returns>
    std::string ReadAllText(const std::string& filename) const
    {
        // Open the file at the end so we can easily find its length
        std::ifstream fileStream = std::ifstream(filename, std::ios::ate);

        wt::Assert(fileStream.is_open() == true, std::string("Error occured while trying open the file \"").append(filename).append("\""));

        std::string fileContents;

        // Resize the buffer to fit content
        fileContents.resize(static_cast<std::size_t>(fileStream.tellg()));

        fileStream.seekg(std::ios::beg);

        // Read file contents into the buffer
        fileStream.read(fileContents.data(), static_cast<std::int64_t>(fileContents.size()));

        return fileContents;
    };


};