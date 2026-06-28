#version 450

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec2 outUv;
layout (location = 2) out int textureIndex;

layout (binding = 0) uniform Screen 
{
    float width;
    float height;
} screen;

struct QuadTransform {
    float x;      
    float y;      
    float z;      
    float rotation;
    float width;  
    float height;
    float scale;
    int horizontal_index;
    int vertical_index;
};


layout(std430, binding = 1) readonly buffer TransformBuffer {
    QuadTransform transforms[];
};

layout (binding = 3) uniform sampler2D texSampler;

out gl_PerVertex 
{
    vec4 gl_Position;   
};

const vec3 POSITIONS[4] = vec3[](
    vec3(-1.0, -1.0, 0.0),
    vec3(-1.0, 1.0, 0.0),
    vec3(1.0, -1.0, 0.0),
    vec3(1.0, 1.0, 0.0)
);

const vec3 COLORS[4] = vec3[](
    vec3(1.0, 0.0, 0.0), // Red
    vec3(0.0, 1.0, 0.0), // Green
    vec3(0.0, 0.0, 1.0),  // Blue
    vec3(1.0, 1.0, 1.0)  // White
);

const vec2 UVS[4] = vec2[](
    vec2(0.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 0.0),
    vec2(1.0, 1.0)
);

void main() 
{
    vec2 spriteSize = vec2(32.0f, 32.0f);
    ivec2 texSize = textureSize(texSampler, 0);

    QuadTransform transform = transforms[gl_InstanceIndex];

	vec3 pos = POSITIONS[gl_VertexIndex];
    pos.x *= (transform.width * 0.5);
    pos.y *= (transform.height * 0.5);

    float rad = transform.rotation;
    float cosR = cos(rad);
    float sinR = sin(rad);
    
    vec2 rotatedPos;
    rotatedPos.x = pos.x * cosR - pos.y * sinR;
    rotatedPos.y = pos.x * sinR + pos.y * cosR;

    vec2 pixelPos = rotatedPos + vec2(transform.x, transform.y);

    vec2 ndcPos;
    ndcPos.x = pixelPos.x / (screen.width * 0.5);
    ndcPos.y = pixelPos.y / (screen.height * 0.5); 

    gl_Position = vec4(ndcPos, transform.z + pos.z, 1.0);

    transform.scale = max(transform.scale, 0.001);

    vec2 maxIndices = vec2(texSize) / spriteSize;
    float clampedXIndex = clamp(float(transform.horizontal_index), 0.0, maxIndices.x - 1.0);
    float clampedYIndex = clamp(float(transform.vertical_index), 0.0, maxIndices.y - 1.0);
    vec2 spriteOffsetPixels = vec2(clampedXIndex, clampedYIndex) * spriteSize;
    vec2 localPixelUV = UVS[gl_VertexIndex] * spriteSize;
    outUv = (spriteOffsetPixels + localPixelUV) / vec2(texSize);

    outColor = COLORS[3];
}
