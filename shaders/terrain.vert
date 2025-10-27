#version 330 core
layout (location=0) in vec3 aPos;
layout (location=1) in vec3 aNormal;

uniform mat4 uProj, uView, uModel;

out vec3 vN;
out vec3 vWPos;

void main(){
    vec4 wpos = uModel * vec4(aPos,1.0);
    vWPos = wpos.xyz;
    vN = mat3(uModel) * aNormal;
    gl_Position = uProj * uView * wpos;
}
