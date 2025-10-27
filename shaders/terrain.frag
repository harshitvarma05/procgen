#version 330 core
in vec3 vN;
in vec3 vWPos;
out vec4 FragColor;

uniform vec3 uLightDir = normalize(vec3(0.6,1.0,0.5));
uniform vec3 uCamPos;

void main(){
    vec3 n = normalize(vN);
    float diff = max(dot(n, normalize(uLightDir)), 0.0);
    float ao = 0.6;
    vec3 base = mix(vec3(0.35,0.55,0.2), vec3(0.5,0.45,0.35), smoothstep(0.0,1.0, vWPos.y/80.0));
    vec3 col = base * (0.2*ao + 0.8*diff);
    FragColor = vec4(col,1.0);
}
