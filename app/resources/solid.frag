#version 450

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 uv;

layout (location = 0) out vec4 outFragColor;

void main() 
{
  int  sides  = 5;
  vec3 colorA = inColor;
  vec3 colorB = vec3(1.0, 0.0, 1.0);

  float r = 6.28318530718 / float(Size);
  float a = atan(uv.x , uv.y) + 3.14159265359;
  float t = cos(floor(0.5 + a / r) * r - a) * length(uv);
                
  float3 col = 1 - step(0.4, t);
  col = mix(ColorA, ColorB, col);

  outFragColor = vec4(col, 1.0);
}
