#version 460

layout(location = 0) in vec2 VertexCoordinate;


uniform mat4 Projection = mat4(1.0f);
uniform mat4 Transform = mat4(1.0f);


void main()
{
    gl_Position = Projection * Transform * vec4(VertexCoordinate, 0.0f, 1.0f);
};