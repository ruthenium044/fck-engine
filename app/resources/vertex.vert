#version 450

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec2 outUv;

out gl_PerVertex 
{
    vec4 gl_Position;   
};

layout (binding = 0) uniform Screen 
{
    float width;
    float height;
} screen;

struct Vertex {
    float x;      
    float y;      
    float u;      
    float v;
    float r;
    float g;
    float b;
    float a;
};

layout(std430, binding = 1) readonly buffer VertexBuffer {
    Vertex vertices[];
};

void main() 
{
    Vertex v = vertices[gl_VertexIndex];
	vec2 pos = vec2(v.x, v.y);

    vec2 ndcPos;
    ndcPos.x = pos.x / (screen.width * 0.5);
    ndcPos.y = pos.y / (screen.height * 0.5); 
    ndcPos.x = ndcPos.x - 1.0;
    ndcPos.y = ndcPos.y - 1.0; 

    gl_Position = vec4(ndcPos, 0.0, 1.0);

    outColor = vec4(v.r, v.g, v.b, v.a);
    outUv = vec2(v.u, v.v);
}
