#version 450 core

layout(location = 0) in vec4 fragVtxColor;
layout(location = 0) out vec4 fragColor;

void main(){
    fragColor = vec4(fragVtxColor.rgb, 1.0); 
}