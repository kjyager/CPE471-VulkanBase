#version 450 core

layout(location = 0) in vec4 vertPos;
layout(location = 1) in vec4 vertNor;

layout(location = 0) out vec3 fragNor;
layout(location = 1) out vec4 W_fragPos;

layout(binding = 0) uniform WorldInfo {
    mat4 V;
    mat4 P;
    float time;
} uWorld;

layout(binding = 1) uniform Transform{
    mat4 Model;
} uModel;

void main(){

    W_fragPos = uModel.Model * vertPos; // Fragment world position
    fragNor = mat3(uModel.Model) * vertNor.xyz;

    gl_Position = uWorld.P * uWorld.V * W_fragPos;
}