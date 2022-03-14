
//#version 460 core
//
//layout(location = 0) in vec2 VertexPosition;
//
//
//layout(std430, binding = 0) readonly buffer Input
//{
//    uint GlyphWidth;
//    uint GlyphHeight;
//
//    uint TextureWidth;
//    uint TextureHeight;
//
//    vec4 ChromaKey;
//
//    vec4 TextColour;
//
//    // No 8-bit integers, so I'm using an int
//    uint Characters[];
//};
//
//
//
//uniform mat4 Projection = mat4(1.0f);
//
//uniform mat4 TextTransform = mat4(1.0f);
//
//
//out vec2 VertexShaderTextureCoordinateOutput;
//out vec4 VertexShaderChromaKeyOutput;
//out vec4 VertexShaderTextColourOutput;
//
//
//
//void main()
//{
//    // Subtract 32 (The space character) from the selected character to get the correct character index
//    const uint glyphIndex = Characters[gl_InstanceID] - 32;
//
//    const uint columns = TextureWidth / GlyphWidth;
//    const uint rows = TextureHeight / GlyphHeight;
//
//    // Convert 1D character to 2D.
//    const uint glpyhX = glyphIndex % columns;
//    const uint glpyhY = glyphIndex / columns;
//
//    // Calculate texutre sampling bounds
//    const float textureCoordinateLeft = float(glpyhX) / float(columns);
//    const float textureCoordinateBottom = float((2 - glpyhY)) / float(rows);
//
//    const float textureCoordinateRight = float(glpyhX + 1) / float(columns);
//    const float textureCoordinateTop = float((2 - glpyhY) + 1) / float(rows);
//
//
//    // Create a texture sampling point
//    switch(gl_VertexID)
//    {
//        // Top left
//        case 0:
//        {
//            VertexShaderTextureCoordinateOutput = vec2(textureCoordinateLeft, textureCoordinateTop);
//            break;
//        };
//
//        // Top right      
//        case 3:
//        case 1:
//        {
//            VertexShaderTextureCoordinateOutput = vec2(textureCoordinateRight, textureCoordinateTop);
//            break;
//        };
//
//        // Bottom left    
//        case 2:
//        case 5:
//        {
//            VertexShaderTextureCoordinateOutput = vec2(textureCoordinateLeft, textureCoordinateBottom);
//            break;
//        };
//
//        // Bottom right
//        case 4:
//        {
//            VertexShaderTextureCoordinateOutput = vec2(textureCoordinateRight, textureCoordinateBottom);
//            break;
//        };
//    };
//
//
//    VertexShaderTextColourOutput = TextColour;
//    VertexShaderChromaKeyOutput = ChromaKey;
//
//   gl_Position = Projection * TextTransform * vec4(VertexPosition.x + (gl_InstanceID * GlyphWidth), VertexPosition.y, 0.0f, 1.0f);
//};


#version 460 core

layout(location = 0) in vec2 VertexPosition;


struct Particle
{
    float TrajectoryA;
    float TrajectoryB;

    vec2 Trajectory;

    mat4 Transform;
    
    float Rate;

    float Opacity;

    float OpacityDecreaseRate;
};


struct Test_Struct
{
    vec2 Test_Vec2_1;
    vec2 Test_Vec2_2;
};

layout(std430, binding = 0) readonly buffer Input
{
    float[10] floats;
    Test_Struct[10] test_structs;
};



//layout(std430, binding = 0) readonly buffer Input
//{
//    float float_off_0;
//    float float_off_4;
//
//    vec4 vec4_off_16;
//    
//    float[10] float_off_32;
//    
//    vec2[2] vec2_off_72;
//
//    Test2[] Test2_off_88;
//};



uniform mat4 Projection = mat4(1.0f);

uniform mat4 TextTransform = mat4(1.0f);


out vec2 VertexShaderTextureCoordinateOutput;
out vec4 VertexShaderChromaKeyOutput;
out vec4 VertexShaderTextColourOutput;



void main()
{
    // VertexShaderTextureCoordinateOutput = Particles[0].Trajectory;
    VertexShaderTextureCoordinateOutput = test_structs[1].Test_Vec2_1 + test_structs[0].Test_Vec2_2;

    // gl_Position = Projection * TextTransform * VertexShaderTextColourOutput;
    gl_Position = Projection * TextTransform * (VertexShaderTextColourOutput * VertexShaderTextureCoordinateOutput.x);
};