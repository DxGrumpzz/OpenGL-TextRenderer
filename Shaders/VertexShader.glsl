#version 460 core

layout(location = 0) in vec2 VertexPosition;


layout(std430, binding = 0) readonly buffer TextureCoordinateInput
{
    uint NumberOfVertices;
    uint GlyphWidth;
    vec2 TextureCoordinate[];
};



uniform mat4 Projection = mat4(1.0f);

uniform mat4 TextTransform = mat4(1.0f);


out vec2 VertexShaderTextureCoordinateOutput;


void main()
{
    VertexShaderTextureCoordinateOutput  = TextureCoordinate[gl_VertexID + (gl_InstanceID * NumberOfVertices)];

   gl_Position = Projection * TextTransform * vec4(VertexPosition.x + (gl_InstanceID * GlyphWidth), VertexPosition.y, 0.0f, 1.0f);
};