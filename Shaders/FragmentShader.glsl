#version 460 core


in vec2 VertexShaderTextureCoordinateOutput;

uniform sampler2D Texutre;

out vec4 OutputColour;


void main()
{
    const vec4 pixel = texture(Texutre, VertexShaderTextureCoordinateOutput);

    // OutputColour = vec4(1.0f, 0.0f, 0.0f, 1.0f);

    OutputColour = pixel;
};