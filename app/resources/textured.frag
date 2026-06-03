#version 450

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 uv;

layout (location = 0) out vec4 outFragColor;

layout (binding = 3) uniform sampler2D texSampler;

void main() 
{
	outFragColor = texture(texSampler, uv);

	//outFragColor = vec4(inColor, 1.0);
}
