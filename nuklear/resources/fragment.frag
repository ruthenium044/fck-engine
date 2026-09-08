#version 450

layout (location = 0) in vec4 colour;
layout (location = 1) in vec2 uv;

layout (location = 0) out vec4 out_fragment_colour;

layout (binding = 2) uniform sampler2D texture_sampler;

void main() 
{
	out_fragment_colour = colour * texture(texture_sampler, uv);
}
