#version 460 core

layout(location = 0) in vec2 VertexPosition;
layout(location = 1) in vec2 TextureCoordinate;


uniform mat4 Projection = mat4(1.0f);
uniform mat4 Transform = mat4(1.0f);

out vec2 VertexShaderTextureCoordinateOutput;


void main()
{
    VertexShaderTextureCoordinateOutput = TextureCoordinate;

    gl_Position = Projection * Transform * vec4(VertexPosition, 0.0f, 1.0f);
};