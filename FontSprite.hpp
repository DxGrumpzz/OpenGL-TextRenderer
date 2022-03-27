#pragma once
#define WIN32_LEAN_AND_MEN

#include <Windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <GLFW/glfw3.h>
#include <cstdint>
#include <memory>
#include <string_view>
#include <glad/glad.h>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>

#pragma comment(lib, "gdiplus.lib")

#include "ShaderProgram.hpp"
#include "ShaderStorageBuffer.hpp"


struct Input
{
    std::uint32_t GlyphWidth;
    std::uint32_t GlyphHeight;

    std::uint32_t TextureWidth;
    std::uint32_t TextureHeight;

    glm::vec4 ChromaKey;

    glm::vec4 TextColour;
};


/// <summary>
/// Draw text from a textue
/// </summary>
class FontSprite
{

private:

    /// <summary>
    /// The ID of the loaded texture
    /// </summary>
    std::uint32_t _textureID = 0;


    /// <summary>
    /// The total height of the font sprite
    /// </summary>
    std::uint32_t _fontSpriteWidth = 0;

    /// <summary>
    /// The total height of the font sprite
    /// </summary>
    std::uint32_t _fontSpriteHeight = 0;


    /// <summary>
    /// The width of a single glyph 
    /// </summary>
    std::uint32_t _glyphWidth = 0;

    /// <summary>
    /// The height of a single glyph 
    /// </summary>
    std::uint32_t _glyphHeight = 0;


    /// <summary>
    /// The number of character columns present in the font sprite
    /// </summary>
    std::uint32_t _columns = 0;

    /// <summary>
    /// The number of character rows present in the font sprite
    /// </summary>
    std::uint32_t _rows = 0;



    std::uint32_t _vao = 0;

    std::uint32_t _glyphVertexPositionsVBO = 0;

    std::reference_wrapper<const ShaderProgram> _shaderProgram;

    // TODO: Try an std::optional
    ShaderStorageBuffer _inputSSBO;


    /// <summary>
    /// Character capacity, how many character we can write before we need to reallocate the buffer
    /// </summary>
    mutable std::size_t _capacity = 0;

public:

    glm::mat4 Transform = glm::mat4(1.0f);

    glm::mat4 ScreenSpaceProjection = glm::mat4(1.0f);


public:

    FontSprite(const std::uint32_t glyphWidth,
               const std::uint32_t glyphHeight,
               const ShaderProgram& shaderProgram,
               const std::wstring_view& texturePath,
               const std::uint32_t capacity = 32) :
        _glyphWidth (glyphWidth),
        _glyphHeight(glyphHeight),
        _shaderProgram(shaderProgram),
        _capacity(capacity)
    {
        _textureID = LoadTexture(texturePath);

        _columns = _fontSpriteWidth / glyphWidth;
        _rows = _fontSpriteHeight / glyphHeight;


        const float glyphWidthAsFloat = static_cast<float>(glyphWidth);
        const float glyphHeightAsFloat = static_cast<float>(glyphHeight);

        std::array<float, 12> glyphVertices =
        {
            // Top left
            0.0f,  0.0f,
            // Top right                           
            glyphWidthAsFloat, 0.0f,
            // Bottom left                         
            0.0f, glyphHeightAsFloat,

            // Top right                           
            glyphWidthAsFloat, 0.0f,
            // Bottom right
            glyphWidthAsFloat, glyphHeightAsFloat,
            // Bottom left
            0.0f, glyphHeightAsFloat,
        };



        glCreateVertexArrays(1, &_vao);
        glBindVertexArray(_vao);

        // Glyph vertex positions
        glCreateBuffers(1, &_glyphVertexPositionsVBO);
        glNamedBufferData(_glyphVertexPositionsVBO, sizeof(glyphVertices), glyphVertices.data(), GL_STATIC_DRAW);
        glVertexArrayVertexBuffer(_vao, 0, _glyphVertexPositionsVBO, 0, sizeof(float) * 2);

        glVertexArrayAttribFormat(_vao, 0, 2, GL_FLOAT, false, 0);
        glVertexArrayAttribBinding(_vao, 0, 0);
        glEnableVertexArrayAttrib(_vao, 0);


        // TODO: Find a way to compute SSBO size automatically
        const std::size_t inputSize = sizeof(Input) + (sizeof(std::uint32_t) * (_capacity));
        _inputSSBO = ShaderStorageBuffer("Input", shaderProgram, inputSize);

        Input input = { };

        input.GlyphWidth = glyphWidth;
        input.GlyphHeight = glyphHeight;

        input.TextureWidth = _fontSpriteWidth;
        input.TextureHeight = _fontSpriteHeight;

        input.ChromaKey = { 1.0f, 1.0f, 1.0f, 1.0f };

        input.TextColour = { 0.0f, 0.0f, 0.0f, 1.0f };

        _inputSSBO.SetValue(0, input);

        /*
        _inputSSBO.SetValue("GlyphWidth", glyphWidth);
        _inputSSBO.SetValue("GlyphHeight", glyphHeight);

        _inputSSBO.SetValue("TextureWidth", _fontSpriteWidth);
        _inputSSBO.SetValue("TextureHeight", _fontSpriteHeight);

        constexpr glm::vec4 chromaKey = { 1.0f, 1.0f, 1.0f, 1.0f };
        _inputSSBO.SetValue("ChromaKey", chromaKey);

        constexpr glm::vec4 textColour = { 0.0f, 0.0f, 0.0f, 1.0f };
        _inputSSBO.SetValue("TextColour", textColour);
        */

        // TODO: Refactor
    };


    ~FontSprite()
    {
        glDeleteBuffers(1, &_glyphVertexPositionsVBO);

        glDeleteTextures(1, &_textureID);

        glDeleteVertexArrays(1, &_vao);
    };


public:

    void Bind(const std::uint32_t textureUnit = 0) const
    {
        _shaderProgram.get().Bind();

        glActiveTexture(GL_TEXTURE0 + textureUnit);
        glBindTexture(GL_TEXTURE_2D, _textureID);

        glBindVertexArray(_vao);

        glBindBuffer(GL_ARRAY_BUFFER, _glyphVertexPositionsVBO);

        _inputSSBO.Bind();
    };


    void Draw(const std::string& text, const glm::vec4& textColour = { 0.0f, 0.0f, 0.0f, 1.0f }) const
    {
        if(text.empty() == true)
            return;

        // Allocate buffer memory if necessary
        if(text.size() > _capacity)
        {
            const std::size_t inputSize = (sizeof(std::uint32_t) * 4) + (sizeof(glm::vec4) * 2) + (sizeof(std::uint32_t) * (text.size() + (_capacity / 2)));
            _inputSSBO.Reallocate(inputSize);

            _capacity = (text.size() + (_capacity / 2));
        };


        // Update uniforms
        _shaderProgram.get().SetMatrix4("Projection", ScreenSpaceProjection);
        _shaderProgram.get().SetMatrix4("TextTransform", Transform);


        // Set text foreground colour
        _inputSSBO.SetValue(offsetof(Input, Input::TextColour), sizeof(textColour), &textColour);


        // Copy texts' characters to the SSBO
        for(std::size_t index = 0;
            const auto & character : text)
        {
            const std::uint32_t& characterAsInt = character;

            _inputSSBO.SetValue(sizeof(Input) + (sizeof(std::int32_t) * index), characterAsInt);
            index++;
        };


        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, static_cast<std::int32_t>(text.size()));
    };


private:

    std::uint32_t LoadTexture(const std::wstring_view& texturePath)
    {
        // TODO: Make this texture loader more generic, in-case we may want to move to STBI or something

        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        ULONG_PTR gdiplusToken = 0;
        Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

        Gdiplus::Bitmap* image = new Gdiplus::Bitmap(texturePath.data());


        _fontSpriteWidth = image->GetWidth();
        _fontSpriteHeight = image->GetHeight();

        Gdiplus::Rect rc(0, 0, static_cast<int>(_fontSpriteWidth), static_cast<int>(_fontSpriteHeight));

        Gdiplus::BitmapData* bitmapData = new Gdiplus::BitmapData();

        image->RotateFlip(Gdiplus::RotateFlipType::Rotate180FlipX);

        image->LockBits(&rc, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, bitmapData);


        std::vector<std::uint32_t> pixels = std::vector<std::uint32_t>(static_cast<std::size_t>(_fontSpriteWidth) * _fontSpriteHeight);

        for(std::size_t y = 0; y < _fontSpriteHeight; y++)
        {
            memcpy(&pixels[y * _fontSpriteWidth], reinterpret_cast<char*>(bitmapData->Scan0) + static_cast<std::size_t>(bitmapData->Stride) * y, static_cast<std::size_t>(_fontSpriteWidth) * 4);

            for(std::size_t x = 0; x < _fontSpriteWidth; x++)
            {
                std::uint32_t& pixel = pixels[y * _fontSpriteWidth + x];
                pixel = (pixel & 0xff00ff00) | ((pixel & 0xff) << 16) | ((pixel & 0xff0000) >> 16);
            };
        };

        image->UnlockBits(bitmapData);

        delete bitmapData;
        delete image;

        Gdiplus::GdiplusShutdown(gdiplusToken);

        std::uint32_t textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<int>(_fontSpriteWidth), static_cast<int>(_fontSpriteHeight), 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

        return textureID;
    };

};

/*


/// <summary>
/// Draw text from a textue
/// </summary>
class FontSprite
{

private:

    /// <summary>
    /// The ID of the loaded texture
    /// </summary>
    std::uint32_t _textureID = 0;


    /// <summary>
    /// The total height of the font sprite
    /// </summary>
    std::uint32_t _fontSpriteWidth = 0;

    /// <summary>
    /// The total height of the font sprite
    /// </summary>
    std::uint32_t _fontSpriteHeight = 0;


    /// <summary>
    /// The width of a single glyph
    /// </summary>
    std::uint32_t _glyphWidth = 0;

    /// <summary>
    /// The height of a single glyph
    /// </summary>
    std::uint32_t _glyphHeight = 0;


    /// <summary>
    /// The number of character columns present in the font sprite
    /// </summary>
    std::uint32_t _columns = 0;

    /// <summary>
    /// The number of character rows present in the font sprite
    /// </summary>
    std::uint32_t _rows = 0;



    std::uint32_t _vao = 0;

    std::uint32_t _glyphVertexPositionsVBO = 0;

    std::reference_wrapper<const ShaderProgram> _shaderProgram;

    ShaderStorageBuffer _inputSSBO;


    /// <summary>
    /// Character capacity, how many character we can write before we need to reallocate the buffer
    /// </summary>
    mutable std::size_t _capacity = 0;

public:

    glm::mat4 Transform = glm::mat4(1.0f);

    glm::mat4 ScreenSpaceProjection = glm::mat4(1.0f);


public:

    FontSprite(const std::uint32_t glyphWidth,
               const std::uint32_t glyphHeight,
               const ShaderProgram& shaderProgram,
               const std::wstring_view& texturePath,
               const std::uint32_t capacity = 32) :
        _glyphWidth (glyphWidth),
        _glyphHeight(glyphHeight),
        _shaderProgram(shaderProgram),
        _capacity(capacity)
    {
        _textureID = LoadTexture(texturePath);

        _columns = _fontSpriteWidth / glyphWidth;
        _rows = _fontSpriteHeight / glyphHeight;


        const float glyphWidthAsFloat = static_cast<float>(glyphWidth);
        const float glyphHeightAsFloat = static_cast<float>(glyphHeight);

        std::array<float, 12> glyphVertices =
        {
            // Top left
            0.0f,  0.0f,
            // Top right
            glyphWidthAsFloat, 0.0f,
            // Bottom left
            0.0f, glyphHeightAsFloat,

            // Top right
            glyphWidthAsFloat, 0.0f,
            // Bottom right
            glyphWidthAsFloat, glyphHeightAsFloat,
            // Bottom left
            0.0f, glyphHeightAsFloat,
        };



        glCreateVertexArrays(1, &_vao);
        glBindVertexArray(_vao);

        // Glyph vertex positions
        glCreateBuffers(1, &_glyphVertexPositionsVBO);
        glNamedBufferData(_glyphVertexPositionsVBO, sizeof(glyphVertices), glyphVertices.data(), GL_STATIC_DRAW);
        glVertexArrayVertexBuffer(_vao, 0, _glyphVertexPositionsVBO, 0, sizeof(float) * 2);

        glVertexArrayAttribFormat(_vao, 0, 2, GL_FLOAT, false, 0);
        glVertexArrayAttribBinding(_vao, 0, 0);
        glEnableVertexArrayAttrib(_vao, 0);


        // TODO: Find a way to compute SSBO size automatically
        const std::size_t inputSize = (sizeof(std::uint32_t) * 4) + (sizeof(glm::vec4) * 2) + (sizeof(std::uint32_t) * (_capacity));
        _inputSSBO = ShaderStorageBuffer("Input", shaderProgram, inputSize);

        _inputSSBO.SetValue("GlyphWidth", glyphWidth);
        _inputSSBO.SetValue("GlyphHeight", glyphHeight);

        _inputSSBO.SetValue("TextureWidth", _fontSpriteWidth);
        _inputSSBO.SetValue("TextureHeight", _fontSpriteHeight);

        constexpr glm::vec4 chromaKey = { 1.0f, 1.0f, 1.0f, 1.0f };
        _inputSSBO.SetValue("ChromaKey", chromaKey);

        constexpr glm::vec4 textColour = { 0.0f, 0.0f, 0.0f, 1.0f };
        _inputSSBO.SetValue("TextColour", textColour);

        // TODO: Refactor
    };


    ~FontSprite()
    {
        glDeleteBuffers(1, &_glyphVertexPositionsVBO);

        glDeleteTextures(1, &_textureID);

        glDeleteVertexArrays(1, &_vao);
    };


public:

    void Bind(const std::uint32_t textureUnit = 0) const
    {
        _shaderProgram.get().Bind();

        glActiveTexture(GL_TEXTURE0 + textureUnit);
        glBindTexture(GL_TEXTURE_2D, _textureID);

        glBindVertexArray(_vao);

        glBindBuffer(GL_ARRAY_BUFFER, _glyphVertexPositionsVBO);

        _inputSSBO.Bind();
    };


    void Draw(const std::string& text, const glm::vec4& textColour = { 0.0f, 0.0f, 0.0f, 1.0f }) const
    {
        if(text.empty() == true)
            return;

        // Allocate buffer memory if necessary
        if(text.size() > _capacity)
        {
            const std::size_t inputSize = (sizeof(std::uint32_t) * 4) + (sizeof(glm::vec4) * 2) + (sizeof(std::uint32_t) * (text.size() + (_capacity / 2)));
            _inputSSBO.Reallocate(inputSize);

            _capacity = (text.size() + (_capacity / 2));
        };


        // Update uniforms
        _shaderProgram.get().SetMatrix4("Projection", ScreenSpaceProjection);
        _shaderProgram.get().SetMatrix4("TextTransform", Transform);


        // Set text colour
        _inputSSBO.SetValue("TextColour", textColour);


        // Copy texts' characters to the SSBO
        for(std::size_t index = 0;
            const auto & character : text)
        {
            const std::uint32_t& characterAsInt = character;

            glNamedBufferSubData(_inputSSBO.GetBufferID(), (sizeof(std::int32_t) * 4) + (sizeof(glm::vec4) * 2) + (sizeof(std::int32_t) * index), sizeof(characterAsInt), &characterAsInt);

            index++;
        };


        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, static_cast<std::int32_t>(text.size()));
    };


private:

    std::uint32_t LoadTexture(const std::wstring_view& texturePath)
    {
        // TODO: Make this texture loader more generic, in-case we may want to move to STBI or something

        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        ULONG_PTR gdiplusToken = 0;
        Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

        Gdiplus::Bitmap* image = new Gdiplus::Bitmap(texturePath.data());


        _fontSpriteWidth = image->GetWidth();
        _fontSpriteHeight = image->GetHeight();

        Gdiplus::Rect rc(0, 0, static_cast<int>(_fontSpriteWidth), static_cast<int>(_fontSpriteHeight));

        Gdiplus::BitmapData* bitmapData = new Gdiplus::BitmapData();

        image->RotateFlip(Gdiplus::RotateFlipType::Rotate180FlipX);

        image->LockBits(&rc, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, bitmapData);


        std::vector<std::uint32_t> pixels = std::vector<std::uint32_t>(static_cast<std::size_t>(_fontSpriteWidth) * _fontSpriteHeight);

        for(std::size_t y = 0; y < _fontSpriteHeight; y++)
        {
            memcpy(&pixels[y * _fontSpriteWidth], reinterpret_cast<char*>(bitmapData->Scan0) + static_cast<std::size_t>(bitmapData->Stride) * y, static_cast<std::size_t>(_fontSpriteWidth) * 4);

            for(std::size_t x = 0; x < _fontSpriteWidth; x++)
            {
                std::uint32_t& pixel = pixels[y * _fontSpriteWidth + x];
                pixel = (pixel & 0xff00ff00) | ((pixel & 0xff) << 16) | ((pixel & 0xff0000) >> 16);
            };
        };

        image->UnlockBits(bitmapData);

        delete bitmapData;
        delete image;

        Gdiplus::GdiplusShutdown(gdiplusToken);

        std::uint32_t textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<int>(_fontSpriteWidth), static_cast<int>(_fontSpriteHeight), 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

        return textureID;
    };

};
*/