#version 450

layout (location = 0) out vec3 out_colour;
layout (location = 1) out vec2 out_uv;

layout (binding = 0) uniform app_screen 
{
    float width;
    float height;
    float sprite_width;
    float sprite_height;
} screen;

struct app_sprite_transform {
    float x;      
    float y;      
    float z;      
    float rotation;
    float scale;
    int horizontal_index;
    int vertical_index;
};

layout(std430, binding = 1) readonly buffer transform_buffer {
    app_sprite_transform transforms[];
};

layout (binding = 3) uniform sampler2D texture_sampler;

out gl_PerVertex 
{
    vec4 gl_Position;   
};

const vec3 positions[4] = vec3[](
    vec3(-1.0, -1.0, 0.0),
    vec3(-1.0, 1.0, 0.0),
    vec3(1.0, -1.0, 0.0),
    vec3(1.0, 1.0, 0.0)
);

const vec3 colours[4] = vec3[](
    vec3(1.0, 0.0, 0.0), // Red
    vec3(0.0, 1.0, 0.0), // Green
    vec3(0.0, 0.0, 1.0),  // Blue
    vec3(1.0, 1.0, 1.0)  // White
);

const vec2 uvs[4] = vec2[](
    vec2(0.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 0.0),
    vec2(1.0, 1.0)
);

void main() 
{
    vec2 spriteSize = vec2(screen.sprite_width, screen.sprite_height);
    ivec2 texture_size = textureSize(texture_sampler, 0);

    app_sprite_transform transform = transforms[gl_InstanceIndex];

	vec3 pos = positions[gl_VertexIndex];
    pos.x *= (screen.sprite_width * transform.scale * 0.5);
    pos.y *= (screen.sprite_height * transform.scale * 0.5);

    float rad = transform.rotation;
    float cos_radius = cos(rad);
    float sin_radius = sin(rad);
    
    vec2 rotated_position;
    rotated_position.x = pos.x * cos_radius - pos.y * sin_radius;
    rotated_position.y = pos.x * sin_radius + pos.y * cos_radius;

    vec2 pixel_position = rotated_position + vec2(transform.x, transform.y);

    vec2 normalized_pixel_position;
    normalized_pixel_position.x = pixel_position.x / (screen.width * 0.5);
    normalized_pixel_position.y = pixel_position.y / (screen.height * 0.5); 

    gl_Position = vec4(normalized_pixel_position, transform.z + pos.z, 1.0);

    transform.scale = max(transform.scale, 0.001);

    vec2 max_indices = vec2(texture_size) / spriteSize;
    float clamped_index_x = clamp(float(transform.horizontal_index), 0.0, max_indices.x - 1.0);
    float clamped_index_y = clamp(float(transform.vertical_index), 0.0, max_indices.y - 1.0);
    vec2 sprite_offset_pixels = vec2(clamped_index_x, clamped_index_y) * spriteSize;

    vec2 local_pixel_uv = uvs[gl_VertexIndex] * spriteSize;
    out_uv = (sprite_offset_pixels + local_pixel_uv) / vec2(texture_size);

    out_colour = colours[3];
}
