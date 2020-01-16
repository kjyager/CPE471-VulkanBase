#version 450 core

layout(location = 0) in vec4 vertPos;
layout(location = 1) in vec4 vertCol;

layout(location = 0) out vec4 fragVtxColor;

layout(binding = 0) uniform Transforms {
    mat4 Model;
    mat4 View;
    mat4 Perspective;
} uTransforms;

void main(){
    gl_Position = vertPos;
    fragVtxColor = vertCol;
}