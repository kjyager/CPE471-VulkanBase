#version 450 core

// layout(location = 0) in vec4 vertPos;

const vec4 FakeVerts[3] = {
    vec4(0.0, -0.5, 0.0, 1.0),
    vec4(0.5, 0.5, 0.0, 1.0),
    vec4(-0.5, 0.5, 0.0, 1.0)
};

void main(){
    gl_Position = FakeVerts[gl_VertexIndex];
}