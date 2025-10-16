#version 320 es

//precision mediump float;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform Push{
    mat4 modelMatrix;
} push;

layout(set=0, binding=0) uniform CameraTransform {
    mat4 view;
    mat4 projection;
} cameraTransform;

const vec3 DIERCTION_TO_LIGHT = normalize(vec3(1.0f, -3.0f, -1.0f));

void main()
{
    vec4 positionWorlSpace = push.modelMatrix * vec4(position, 1.0f);

    mat3 normalMatrix = transpose(inverse(mat3(push.modelMatrix)));
    vec3 normalWorldSpace = normalize(normalMatrix * normal);

    float lightIntensity = max(dot(normalWorldSpace, DIERCTION_TO_LIGHT), 0.f);
    
    gl_Position = cameraTransform.projection * cameraTransform.view * positionWorlSpace;
    fragColor = lightIntensity * color;
}
 