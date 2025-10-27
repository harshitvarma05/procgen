#version 330 core
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D uTex;
uniform bool uFalseColor;
vec3 turbo(float x){
    x=clamp(x,0.0,1.0);
    return vec3(0.5+0.5*sin(6.2831*(x+0.0)),
    0.5+0.5*sin(6.2831*(x+0.33)),
    0.5+0.5*sin(6.2831*(x+0.66)));
}
void main(){
    float h = texture(uTex, vUV).r;
    if(uFalseColor) FragColor = vec4(turbo(h),1.0);
    else FragColor = vec4(h,h,h,1.0);
}
