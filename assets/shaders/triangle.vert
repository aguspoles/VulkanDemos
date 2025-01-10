#version 320 es

layout(location = 0) in vec3 position;

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform Push{
    mat4 transform; // projection * view * model (later we will use uniform buffers)
    mat4 modelMatrix;
} push;

void main()
{
    vec4 positionWorlSpace = push.modelMatrix * vec4(position, 1.0f);
    
    gl_Position = push.transform * vec4(position, 1.0f);
    fragColor = vec3(0.5f);
}
 