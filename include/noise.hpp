#pragma once
#include <array>
#include <cstdint>
#include <cmath>
#include <random>

struct RNG {
    uint64_t seed;
    explicit RNG(uint64_t s): seed(s) {}

    static inline uint64_t splitmix64(uint64_t x){
        x += 0x9e3779b97f4a7c15ull;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;
        return x ^ (x >> 31);
    }
    uint32_t nextU32(){
        seed = splitmix64(seed);
        return static_cast<uint32_t>(seed >> 32);
    }
    double next01(){ return (nextU32() / 4294967296.0); }
};

struct Perlin {
    std::array<int,512> perm{}; // permutation table * 2
    explicit Perlin(uint64_t seed=1337){
        std::array<int,256> p{};
        for(int i=0;i<256;++i) p[i]=i;
        RNG rng(seed);

        for(int i=255;i>0;--i){
            int j = rng.nextU32() % (i+1);
            std::swap(p[i],p[j]);
        }
        for(int i=0;i<512;++i) perm[i]=p[i&255];
    }
    static inline double fade(double t){ // quintic curve
        return t*t*t*(t*(t*6-15)+10);
    }
    static inline double lerp(double t,double a,double b){ return a + t*(b-a); }
    static inline double grad(int h, double x, double y, double z){

        int g = h & 15;
        double u = g<8 ? x : y;
        double v = g<4 ? y : (g==12||g==14 ? x : z);
        return ((g&1)?-u:u) + ((g&2)?-v:v);
    }

    double noise(double x,double y,double z) const {
        int X = (int)std::floor(x) & 255;
        int Y = (int)std::floor(y) & 255;
        int Z = (int)std::floor(z) & 255;
        x -= std::floor(x); y-=std::floor(y); z-=std::floor(z);
        double u=fade(x), v=fade(y), w=fade(z);

        int A = perm[X] + Y;
        int AA = perm[A] + Z;
        int AB = perm[A+1] + Z;
        int B = perm[X+1] + Y;
        int BA = perm[B] + Z;
        int BB = perm[B+1] + Z;

        double g000 = grad(perm[AA],   x,   y,   z);
        double g100 = grad(perm[BA], x-1,   y,   z);
        double g010 = grad(perm[AB],   x, y-1,   z);
        double g110 = grad(perm[BB], x-1, y-1,   z);
        double g001 = grad(perm[AA+1],   x,   y, z-1);
        double g101 = grad(perm[BA+1], x-1,   y, z-1);
        double g011 = grad(perm[AB+1],   x, y-1, z-1);
        double g111 = grad(perm[BB+1], x-1, y-1, z-1);

        double x1 = lerp(u, g000, g100);
        double x2 = lerp(u, g010, g110);
        double y1 = lerp(v, x1, x2);

        double x3 = lerp(u, g001, g101);
        double x4 = lerp(u, g011, g111);
        double y2 = lerp(v, x3, x4);

        return lerp(w, y1, y2); // [-1,1]
    }

    double noise2D(double x,double y) const { return noise(x,y,0.0); }

    double fbm3(double x,double y,double z,int oct,double lac=2.0,double pers=0.5) const {
        double amp=1.0, freq=1.0, sum=0.0, norm=0.0;
        for(int i=0;i<oct;++i){
            sum += amp * noise(x*freq, y*freq, z*freq);
            norm += amp;
            freq *= lac; amp *= pers;
        }
        return sum / norm; // normalize to [-1,1]
    }
    double fbm2(double x,double y,int oct,double lac=2.0,double pers=0.5) const {
        return fbm3(x,y,0.0,oct,lac,pers);
    }

    double ridged2(double x,double y,int oct,double lac=2.0,double gain=0.5) const {
        double sum=0.0, freq=1.0, amp=0.5;
        for(int i=0;i<oct;++i){
            double n = 1.0 - std::abs(noise2D(x*freq,y*freq)); // [0,2]→[0,1]
            n *= n;
            sum += n * amp;
            freq *= lac; amp *= gain;
        }
        return sum * 2.0 - 1.0; // map ~[0,1] to [-1,1]
    }
    double warp2(double x,double y,int oct,double strength) const {
        double qx = fbm2(x+5.2, y+1.3, oct) * strength;
        double qy = fbm2(x-3.7, y-7.1, oct) * strength;
        return fbm2(x+qx, y+qy, oct);
    }
};
