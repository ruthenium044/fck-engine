#version 450

layout (location = 0) out vec3 outColor;

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
};

layout(std430, binding = 1) readonly buffer TransformBuffer {
    QuadTransform transforms[];
};

layout (binding = 2) uniform Config 
{
    int gradient;
} configuration;


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

void main() 
{
    QuadTransform transform = transforms[gl_InstanceIndex];

	vec3 pos = POSITIONS[gl_VertexIndex];
    if(configuration.gradient != 0) {
        outColor = COLORS[gl_VertexIndex];
    }
    else {
        outColor = COLORS[3];
    }
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
}
