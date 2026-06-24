#version 450

layout (location = 0) in vec4 inColor;
layout (location = 1) in vec2 inUv;

layout (location = 0) out vec4 outFragColor;

layout (binding = 2) uniform sampler2D texSampler;

void main() 
{
	outFragColor = inColor * texture(texSampler, inUv);
}
