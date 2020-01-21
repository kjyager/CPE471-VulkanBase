#version 450 core

layout(location = 0) in vec4 vertPos;
layout(location = 1) in vec4 vertCol;

layout(location = 0) out vec4 fragVtxColor;

void main(){
    // Flip y-axis to conform to standard coordinate system
    gl_Position = vec4(vertPos.x, -vertPos.y, vertPos.z, vertPos.w);
    fragVtxColor = vertCol;
}