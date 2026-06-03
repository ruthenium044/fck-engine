#version 450

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 uv;

layout (location = 0) out vec4 outFragColor;

float sdRoundBox( in vec2 p ) 
{
	vec2 si = vec2(0.9, 0.6);
    vec4 r = vec4(0.5, 0.5, 0.5, 0.5);
    r = min(r,min(si.x,si.y));
    
    r.xy = (p.x>0.0)?r.xy : r.zw;
    r.x  = (p.y>0.0)?r.x  : r.y;
    vec2 q = abs(p)-1.0+r.x;
    return min(max(q.x,q.y),0.0) + length(max(q,0.0)) - r.x;
}


void main() 
{
    float d = sdRoundBox(uv);
    if (d > 0.0) {
        discard;
    }

	outFragColor = vec4(inColor, 1.0);
}
