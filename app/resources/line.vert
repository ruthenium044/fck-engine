#version 450

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec2 outUv;
layout (location = 2) out int textureIndex;

layout (binding = 0) uniform Screen 
{
    float width;
    float height;
} screen;

struct LineTransform {
    float startX;   
    float startY;   
    float endX;    
    float endY;   
    float z;        
    float thickness;
    float scale;
    float unused;
};

layout(std430, binding = 1) readonly buffer TransformBuffer {
    LineTransform transforms[];
};

layout (binding = 2) uniform Config 
{
    int gradient;
    int is_sdf;
} configuration;

out gl_PerVertex 
{
    vec4 gl_Position;   
};

const vec3 POSITIONS[4] = vec3[](
    vec3(-1.0, -1.0, 0.0),
    vec3(-1.0,  1.0, 0.0),
    vec3( 1.0, -1.0, 0.0),
    vec3( 1.0,  1.0, 0.0)
);

const vec3 COLORS[4] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0),
    vec3(1.0, 1.0, 1.0)
);

const vec2 UVS[4] = vec2[](
    vec2(0.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 0.0),
    vec2(1.0, 1.0)
);

void main() 
{
    LineTransform transform = transforms[gl_InstanceIndex];
    
    vec2 startPt = vec2(transform.startX, transform.startY);
    vec2 endPt   = vec2(transform.endX, transform.endY);
    
    vec2 lineDir = endPt - startPt;
    vec2 lineNorm = normalize(vec2(-lineDir.y, lineDir.x)); // Perpendicular normal
    
    float isEnd = POSITIONS[gl_VertexIndex].x * 0.5 + 0.5; // Maps -1.0 -> 0.0, and 1.0 -> 1.0
    vec2 basePixelPos = mix(startPt, endPt, isEnd);
    
    float halfThickness = transform.thickness * 0.5;
    vec2 pixelPos = basePixelPos + (lineNorm * POSITIONS[gl_VertexIndex].y * halfThickness);

    vec2 ndcPos;
    ndcPos.x = pixelPos.x / (screen.width * 0.5);
    ndcPos.y = pixelPos.y / (screen.height * 0.5); 

    gl_Position = vec4(ndcPos, transform.z, 1.0);

    transform.scale = max(transform.scale, 0.001);

    if(configuration.gradient != 0) 
    {
        outColor = COLORS[gl_VertexIndex];
    }
    else 
    {
        outColor = COLORS[3];
    }

    if(configuration.is_sdf != 0) 
    {
        outUv = ((UVS[gl_VertexIndex] * 2.0) - 1.0) / transform.scale;
    } 
    else 
    {
        outUv = UVS[gl_VertexIndex] / transform.scale;
    }
}