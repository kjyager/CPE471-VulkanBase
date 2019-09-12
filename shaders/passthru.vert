#version 450 core

layout(location = 0) in vec4 vertPos;

void main(){
    gl_Position = vertPos;
}