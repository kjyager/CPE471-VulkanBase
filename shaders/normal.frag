#version 450 core

layout(location = 0) in vec3 fragNor;
layout(location = 0) out vec4 fragColor;

void main(){
    vec3 normal = normalize(fragNor);
    fragColor = vec4(normal, 1.0); 
}