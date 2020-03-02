#version 450 core

layout(location = 0) in vec4 vertPos;
layout(location = 1) in vec4 vertCol;

layout(location = 0) out vec4 fragVtxColor;

layout(binding = 0) uniform WorldInfo {
    mat4 Ortho;
} uWorld;

void main(){
    gl_Position = uWorld.Ortho * vertPos;
    fragVtxColor = vertCol;
}