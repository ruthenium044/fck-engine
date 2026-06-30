#version 450

layout (location = 0) out vec4 out_colour;
layout (location = 1) out vec2 out_uv;

out gl_PerVertex 
{
    vec4 gl_Position;   
};

layout (binding = 0) uniform fck_nk_screen 
{
    float width;
    float height;
} screen;

struct app_vertex {
    float x;      
    float y;      
    float u;      
    float v;
    float r;
    float g;
    float b;
    float a;
};

layout(std430, binding = 1) readonly buffer app_vertex_buffer {
    app_vertex vertices[];
};

void main() 
{
    app_vertex v = vertices[gl_VertexIndex];
	vec2 pos = vec2(v.x, v.y);

    vec2 normalices_position;
    normalices_position.x = pos.x / (screen.width * 0.5);
    normalices_position.y = pos.y / (screen.height * 0.5); 
    normalices_position.x = normalices_position.x - 1.0;
    normalices_position.y = normalices_position.y - 1.0; 

    gl_Position = vec4(normalices_position, 0.0, 1.0);

    out_colour = vec4(v.r, v.g, v.b, v.a);
    out_uv = vec2(v.u, v.v);
}
