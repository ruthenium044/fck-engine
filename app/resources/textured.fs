#version 450

layout (location = 0) in vec3 colour;
layout (location = 1) in vec2 uv;

layout (location = 0) out vec4 out_fragment_colour;

layout (binding = 3) uniform sampler2D texture_sampler;

void main() 
{
	out_fragment_colour = texture(texture_sampler, uv) * vec4(colour, 1.0);
	if(out_fragment_colour.a < 0.1f) 
	{
		discard;
	}
}
