#version 450 core

layout(location = 0) in vec4 vertPos;
layout(location = 1) in vec4 vertNor;

layout(location = 0) out vec3 fragNor;

layout(binding = 0) uniform Transform{
    mat4 Model;
} uModel;

layout(binding = 1) uniform ViewTransforms {
    mat4 V;
    mat4 P;
    vec3 W_ViewDir; // World view direction. 
} uView;

void main(){
    gl_Position = uView.P * uView.V * uModel.Model * vertPos;
    fragNor = (uModel.Model * vertNor).xyz;
}