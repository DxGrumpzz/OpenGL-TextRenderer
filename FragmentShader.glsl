#version 460 core


in vec2 VertexShaderTextureCoordinateOutput;
in vec4 VertexShaderChromaKeyOutput;
in vec4 VertexShaderTextColourOutput;

uniform sampler2D Texutre;

out vec4 OutputColour;



void main()
{
    const vec4 pixel = texture(Texutre, VertexShaderTextureCoordinateOutput);

    // If the current pixel matches the chroma key colour..
    if(pixel.rgb == VertexShaderChromaKeyOutput.rgb)
        // Then throw pixel away
        discard;

    OutputColour = VertexShaderTextColourOutput;
};