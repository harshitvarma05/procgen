//
// Created by Harshit on 27-10-2025.
//
#include <cstdio>
#include <vector>
#include <string>
#include <chrono>
#include <cmath>
#include <functional>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../include/noise.hpp"
#include "../include/camera.hpp"

static std::string loadText(const char* path){
    FILE* f = fopen(path, "rb");
    if(!f) { fprintf(stderr,"Could not open %s\n", path); return {}; }
    fseek(f,0,SEEK_END); long sz = ftell(f); fseek(f,0,SEEK_SET);
    std::string s; s.resize(sz);
    fread(s.data(),1,sz,f); fclose(f);
    return s;
}
static GLuint makeShader(GLenum type, const char* src){
    GLuint sh = glCreateShader(type);
    glShaderSource(sh,1,&src,nullptr); glCompileShader(sh);
    GLint ok; glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if(!ok){ char log[4096]; glGetShaderInfoLog(sh,4096,nullptr,log); fprintf(stderr,"shader err: %s\n", log); }
    return sh;
}
static GLuint makeProgram(const std::string& vs, const std::string& fs){
    GLuint v=makeShader(GL_VERTEX_SHADER, vs.c_str());
    GLuint f=makeShader(GL_FRAGMENT_SHADER, fs.c_str());
    GLuint p=glCreateProgram(); glAttachShader(p,v); glAttachShader(p,f); glLinkProgram(p);
    GLint ok; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if(!ok){ char log[4096]; glGetProgramInfoLog(p,4096,nullptr,log); fprintf(stderr,"link err: %s\n", log); }
    glDeleteShader(v); glDeleteShader(f);
    return p;
}

static GLuint makeQuad(){
    float v[] = {

        -1,-1, 0,0,
         1,-1, 1,0,
         1, 1, 1,1,
        -1, 1, 0,1
    };
    unsigned idx[] = {0,1,2, 0,2,3};
    GLuint vao,vbo,ebo;
    glGenVertexArrays(1,&vao);
    glGenBuffers(1,&vbo);
    glGenBuffers(1,&ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(v),v,GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(idx),idx,GL_STATIC_DRAW);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)(2*sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    return vao;
}

struct Grid {
    int W, H;
    GLuint vao=0,vbo=0,nbo=0,ebo=0;
    std::vector<glm::vec3> pos, nor;
    std::vector<unsigned> idx;
    Grid(int w,int h): W(w),H(h){
        pos.resize(W*H);
        nor.resize(W*H);
        for(int z=0; z<H; ++z){
            for(int x=0; x<W; ++x){
                pos[z*W+x] = { (float)x, 0.0f, (float)z };
            }
        }
        for(int z=0; z<H-1; ++z){
            for(int x=0; x<W-1; ++x){
                unsigned i0 = z*W + x, i1 = i0+1, i2 = i0+W, i3 = i2+1;
                idx.push_back(i0); idx.push_back(i2); idx.push_back(i1);
                idx.push_back(i1); idx.push_back(i2); idx.push_back(i3);
            }
        }
        glGenVertexArrays(1,&vao);
        glGenBuffers(1,&vbo);
        glGenBuffers(1,&nbo);
        glGenBuffers(1,&ebo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER,vbo);
        glBufferData(GL_ARRAY_BUFFER,pos.size()*sizeof(glm::vec3),pos.data(),GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,(void*)0);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER,nbo);
        glBufferData(GL_ARRAY_BUFFER,nor.size()*sizeof(glm::vec3),nor.data(),GL_DYNAMIC_DRAW);
        glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,0,(void*)0);
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size()*sizeof(unsigned), idx.data(), GL_STATIC_DRAW);
        glBindVertexArray(0);
    }
    static glm::vec3 normalAt(const std::vector<glm::vec3>& P, int W, int H, int x, int z){
        auto at=[&](int ix,int iz){
            ix=std::clamp(ix,0,W-1); iz=std::clamp(iz,0,H-1);
            return P[iz*W+ix];
        };
        glm::vec3 L = at(x-1,z), R = at(x+1,z), D = at(x,z-1), U = at(x,z+1);
        glm::vec3 dx = R - L, dz = U - D;
        glm::vec3 n = glm::normalize(glm::cross(dz, dx));
        if(!std::isfinite(n.x)) n={0,1,0};
        return n;
    }
    void updateHeights(const std::function<float(int,int)>& hfn){
        for(int z=0; z<H; ++z){
            for(int x=0; x<W; ++x){
                pos[z*W+x].y = hfn(x,z);
            }
        }
        for(int z=0; z<H; ++z)
            for(int x=0; x<W; ++x)
                nor[z*W+x] = normalAt(pos,W,H,x,z);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, pos.size()*sizeof(glm::vec3), pos.data());
        glBindBuffer(GL_ARRAY_BUFFER, nbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, nor.size()*sizeof(glm::vec3), nor.data());
    }
    void draw(){
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, (GLsizei)idx.size(), GL_UNSIGNED_INT, 0);
    }
};

int main(){
    if(!glfwInit()){ fprintf(stderr,"glfw init fail\n"); return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* win = glfwCreateWindow(1280, 720, "Realtime Noise & Terrain", nullptr, nullptr);
    if(!win){ fprintf(stderr,"window fail\n"); glfwTerminate(); return -1; }
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        fprintf(stderr,"glad fail\n"); return -1;
    }


    GLuint progQuad = makeProgram(loadText("../shaders/noise_quad.vert"), loadText("../shaders/noise_quad.frag"));
    GLuint progTer  = makeProgram(loadText("../shaders/terrain.vert"),   loadText("../shaders/terrain.frag"));


    GLuint quadVAO = makeQuad();


    const int TEX=512;
    GLuint tex; glGenTextures(1,&tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D,0,GL_R8,TEX,TEX,0,GL_RED,GL_UNSIGNED_BYTE,nullptr);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);


    Grid grid(256,256);


    Camera cam;


    uint64_t seed = 1337;
    Perlin perlin(seed);
    int oct = 6; double lac=2.0, pers=0.5;
    double scale2D = 0.008;
    double scaleXZ = 0.03;
    double heightAmp = 40.0;
    bool falseColor=true, showTerrain=false, paused=false;
    bool ridged=false, warped=false, caves=false;
    double timeZ=0.0;

    auto tickStart = std::chrono::high_resolution_clock::now();

    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    while(!glfwWindowShouldClose(win)){
        glfwPollEvents();

        if(glfwGetKey(win, GLFW_KEY_ESCAPE)==GLFW_PRESS) glfwSetWindowShouldClose(win,1);
        if(glfwGetKey(win, GLFW_KEY_1)==GLFW_PRESS) showTerrain=false;
        if(glfwGetKey(win, GLFW_KEY_2)==GLFW_PRESS) showTerrain=true;
        if(glfwGetKey(win, GLFW_KEY_C)==GLFW_PRESS) caves=true;
        if(glfwGetKey(win, GLFW_KEY_V)==GLFW_PRESS) caves=false;
        if(glfwGetKey(win, GLFW_KEY_F)==GLFW_PRESS) falseColor=true;
        if(glfwGetKey(win, GLFW_KEY_G)==GLFW_PRESS) falseColor=false;
        if(glfwGetKey(win, GLFW_KEY_R)==GLFW_PRESS){ seed = RNG::splitmix64(seed+1); perlin = Perlin(seed); }
        if(glfwGetKey(win, GLFW_KEY_SPACE)==GLFW_PRESS) paused=true;
        if(glfwGetKey(win, GLFW_KEY_BACKSPACE)==GLFW_PRESS) paused=false;

        if(glfwGetKey(win, GLFW_KEY_O)==GLFW_PRESS) oct = std::max(1, oct-1);
        if(glfwGetKey(win, GLFW_KEY_P)==GLFW_PRESS) oct = std::min(12, oct+1);
        if(glfwGetKey(win, GLFW_KEY_K)==GLFW_PRESS) lac = std::max(1.1, lac-0.01);
        if(glfwGetKey(win, GLFW_KEY_L)==GLFW_PRESS) lac = std::min(4.0,  lac+0.01);
        if(glfwGetKey(win, GLFW_KEY_SEMICOLON)==GLFW_PRESS) pers = std::max(0.2, pers-0.005);
        if(glfwGetKey(win, GLFW_KEY_APOSTROPHE)==GLFW_PRESS) pers = std::min(0.9, pers+0.005);
        if(glfwGetKey(win, GLFW_KEY_LEFT_BRACKET)==GLFW_PRESS) scaleXZ = std::max(0.005, scaleXZ-0.0005);
        if(glfwGetKey(win, GLFW_KEY_RIGHT_BRACKET)==GLFW_PRESS) scaleXZ = std::min(0.08,  scaleXZ+0.0005);
        if(glfwGetKey(win, GLFW_KEY_N)==GLFW_PRESS) ridged = true;
        if(glfwGetKey(win, GLFW_KEY_M)==GLFW_PRESS) ridged = false;
        if(glfwGetKey(win, GLFW_KEY_W)==GLFW_PRESS) warped = true;
        if(glfwGetKey(win, GLFW_KEY_Q)==GLFW_PRESS) warped = false;


        if(!paused){
            auto now = std::chrono::high_resolution_clock::now();
            double t = std::chrono::duration<double>(now - tickStart).count();
            timeZ = t * 0.15;
        }


        float moveSpeed = 50.0f / 60.0f;
        if(glfwGetKey(win, GLFW_KEY_A)==GLFW_PRESS) cam.pos.x -= moveSpeed;
        if(glfwGetKey(win, GLFW_KEY_D)==GLFW_PRESS) cam.pos.x += moveSpeed;
        if(glfwGetKey(win, GLFW_KEY_W)==GLFW_PRESS) cam.pos.z -= moveSpeed;
        if(glfwGetKey(win, GLFW_KEY_S)==GLFW_PRESS) cam.pos.z += moveSpeed;
        if(glfwGetKey(win, GLFW_KEY_E)==GLFW_PRESS) cam.pos.y += moveSpeed;
        if(glfwGetKey(win, GLFW_KEY_Z)==GLFW_PRESS) cam.pos.y -= moveSpeed;


        static double lastX=0,lastY=0; static bool first=true;
        double mx,my; glfwGetCursorPos(win,&mx,&my);
        if(first){ lastX=mx; lastY=my; first=false; }
        float dx=(float)(mx-lastX), dy=(float)(my-lastY);
        lastX=mx; lastY=my;
        float sens=0.1f;
        cam.yaw   += dx*sens;
        cam.pitch -= dy*sens;
        cam.pitch = std::clamp(cam.pitch,-89.0f,89.0f);


        std::vector<uint8_t> texbuf(TEX*TEX);
        for(int y=0;y<TEX;++y){
            for(int x=0;x<TEX;++x){
                double u = x*scale2D, v = y*scale2D;
                double n;
                if(caves){

                    n = perlin.fbm3(u, v, timeZ, oct, lac, pers);
                } else {
                    if(ridged){
                        n = perlin.ridged2(u,v, oct, lac, 0.5);
                    } else if(warped){
                        n = perlin.warp2(u,v, oct, 1.0);
                    } else {
                        n = perlin.fbm2(u,v, oct, lac, pers);
                    }
                }
                n = 0.5*(n+1.0); // [-1,1]→[0,1]
                texbuf[y*TEX+x] = (uint8_t)std::clamp(int(n*255.0),0,255);
            }
        }
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexSubImage2D(GL_TEXTURE_2D,0,0,0,TEX,TEX,GL_RED,GL_UNSIGNED_BYTE,texbuf.data());


        if(showTerrain){
            grid.updateHeights([&](int x,int z)->float{
                double u = x*scaleXZ, v = z*scaleXZ;
                double base  = perlin.fbm2(u,v, oct, lac, pers);
                double rid   = perlin.ridged2(u*1.7, v*1.7, std::max(1,oct-2), 2.0, 0.5);
                double warp  = warped ? perlin.warp2(u*0.6, v*0.6, 4, 1.0) : 0.0;
                double height = 0.55*base + 0.45*rid + 0.25*warp;
                return (float)(heightAmp * (0.5*(height+1.0)));
            });
        }


        int w,h; glfwGetFramebufferSize(win,&w,&h);
        glViewport(0,0,w,h);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.07f,0.08f,0.1f,1);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        if(!showTerrain){
            glUseProgram(progQuad);
            glUniform1i(glGetUniformLocation(progQuad,"uTex"), 0);
            glUniform1i(glGetUniformLocation(progQuad,"uFalseColor"), falseColor?1:0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex);
            glBindVertexArray(quadVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        } else {
            glUseProgram(progTer);
            glm::mat4 proj = glm::perspective(glm::radians(cam.fov), w/(float)h, 0.1f, 1000.0f);
            glm::mat4 view = cam.view();
            glm::mat4 model = glm::translate(glm::mat4(1), glm::vec3(-128, 0, -128));
            glUniformMatrix4fv(glGetUniformLocation(progTer,"uProj"),1,GL_FALSE,&proj[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(progTer,"uView"),1,GL_FALSE,&view[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(progTer,"uModel"),1,GL_FALSE,&model[0][0]);
            glm::vec3 cp = cam.pos;
            glUniform3f(glGetUniformLocation(progTer,"uCamPos"), cp.x,cp.y,cp.z);
            grid.draw();
        }


        glfwSetWindowTitle(win, (std::string("Realtime Noise  [1]Noise [2]Terrain  [R]Reseed  O/P oct=")+std::to_string(oct)+
            "  L/K lac="+std::to_string(lac)+"  '/; pers="+std::to_string(pers)+
            (ridged?"  ridged":"")+(warped?"  warped":"")+(caves?"  caves":"")
        ).c_str());

        glfwSwapBuffers(win);
    }

    glfwTerminate();
    return 0;
}
