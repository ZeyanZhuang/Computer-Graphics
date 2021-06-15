#include <cstdlib>
#include <cstring>
#include "./include/object.h"
#include "./include/_random.h"
#include "./src/_const.cpp"

class Canvas{
public:
    Canvas(int _n, Sphere* _objects, Line3 _camera, char* _path);
    Canvas();
    ~Canvas();
    Vec3 radiance(const Line3 &l, int depth, unsigned short * seed);
    inline bool get_intersect(const Line3 &l, double& t, int& id);
    void render(int samples, int w, int h, double d, double ratio);
// private:
    int n;
    Object **objects;
    const char* path;
    Line3 camera;
};

Canvas::Canvas():n(0), objects(nullptr){}
Canvas::Canvas(int _n, Sphere* _objects, Line3 _camera, char* _path):n(_n), camera(_camera){
    n = _n;
    objects = new Object* [n];
    for (int i = 0; i < n; ++i) {
        objects[i] = &_objects[i];
    }
    path = _path;
}
Canvas::~Canvas() {
    delete[] objects;
    delete[] path;
}

inline bool Canvas::get_intersect(const Line3 &l, double &t, int &id) {
    double d;
    t = inf_db;
    for(int i = 0; i < n; ++i) {
        d = objects[i]->intersect(l);
        if(d != 0 && d < t) {
            t = d, id = i;
        }
    }
    return t < inf_db;
}
Vec3 Canvas::radiance(const Line3 &l, int depth, unsigned short *seed) {
    depth++;
    int id = 0;
    double t;
    if(!get_intersect(l, t, id)) {
        return Vec3();
    }
    Object* obj = nullptr;
    obj = objects[id];
    Vec3 x = l.o + l.d * t, _n = obj->get_norm(x), nl = l.d.dot(_n) < 0 ? _n : _n * -1, clr = obj->c;
    double p = max3(clr.x, clr.y, clr.z);
    if(depth > 5) {
        if (rnd::erand48(seed) < p && depth < 10) {
            clr.x /= p, clr.y /= p, clr.z /= p;
        }
        else {
            return obj->e;
        }
    }
    if(obj->refl == DIFF) {
        double phi = PI * rnd::erand48(seed) / 2, theta = 2 * PI * rnd::erand48(seed), sin_phi = sin(phi);
        const Vec3& w = nl;
        Vec3 u = ((fabs(w.x) > .9 ? Vec3(0, 1, 0) : Vec3(1, 0, 0)).det(w)).normal(), v = w.det(u);
        Vec3 r = (u * sin_phi * cos(theta) + v * sin_phi * sin(theta) +  w * sqrt(1 - square(sin_phi))).normal();
        return obj->e + clr * radiance(Line3(x, r), depth, seed);
    }
    else if(obj->refl == SPEC){
        return obj->e + clr * radiance(Line3(x, l.d - nl * 2 * l.d.dot(nl)), depth, seed);
    }
    else if(obj->refl == REFR) {
        Line3  refL(x, l.d - nl * 2 * l.d.dot(nl));
        bool into = _n.dot(nl) > 0;
        double nc = 1.0, nt = 1.5, N = into? (nc / nt) : (nt / nc), cos_theta1 = -l.d.dot(nl);
        double cos2_theta2 = 1 - N * N * (1 - cos_theta1 * cos_theta1);
        if(cos2_theta2 <= 0) {
            return obj->e + clr * radiance(refL, depth, seed);
        }
        Vec3 d_fra = (l.d * N - nl * (-cos_theta1 * N  + sqrt(cos2_theta2))).normal();
        double F0 = square(nt - nc) / square(nt + nc), c = (1 - (into ? cos_theta1 : d_fra.dot(_n))); 
        double Re = F0 + (1 - F0) * c * c * c * c * c, Tr = 1 - Re; 
        if(depth > 2) {
            double P = .25 + .5 * Re, RP = Re / P, TP = Tr / (1 - P);
            return obj->e + clr * (rnd::erand48(seed) < P ? radiance(refL, depth, seed) * RP : radiance(Line3(x, d_fra), depth, seed) * TP);
        }
        else {
            return obj->e + clr * (radiance(refL, depth, seed) * Re + radiance(Line3(x, d_fra), depth, seed)) * Tr; 
        }
    }
    return Vec3();
}
void Canvas::render(int samples, int w, int h, double dis, double ratio) {
    Vec3 cx(w *ratio / h, 0, 0), cy = cx.det(this->camera.d).normal() * ratio; 
    Vec3 *c = new Vec3[h * w], r;
#pragma omp parallel for schedule(dynamic, 1) private(r)       // OpenMP
    for(int y = 0; y < h; ++y) {
        // fprintf(stderr, "\rRendering (%d spp) %.5f", samples * 4, 100.0 * y / (h - 1));
        fprintf(stderr,"\rRendering (%d spp) %5.2f%%",samples*4,100.* y/(h-1));
        // printf("Rendering (%d spp) %.5f", samples * 4, 100.0 * y / (h - 1));
        for(int x = 0; x < w; ++x) {
            unsigned short seedX[3] = {y, y * y, y * y * y};
            int idx = (h - 1 - y) * w + x;
            for(int sx = 0; sx < 2; ++ sx) {
                for(int sy = 0; sy < 2; ++sy, r=Vec3()) {
                    // Vec3 r(0, 0, 0);
                    for(int t = 0; t < samples; ++ t) {
                        double r1 = 2 * rnd::erand48(seedX), dx = (r1 < 1 ? sqrt(r1) - 1 : 1 - sqrt(2 - r1)); 
                        double r2 = 2 * rnd::erand48(seedX), dy = (r2 < 1 ? sqrt(r2) - 1 : 1 - sqrt(2 - r2));
                        Vec3 dir = cx * (((sx + 0.5 + dx) / 2 + x) / w - 0.5) + 
                                 cy * (((sy + 0.5 + dy) / 2 + y) / h - 0.5) +
                                 this->camera.d;
                        r = r + radiance(Line3(this->camera.o + dir * dis, dir.normal()), 0, seedX);
                    }
                    r = r * (1.0 / samples);
                    c[idx] = c[idx] + Vec3(clamp(r.x), clamp(r.y), clamp(r.z)) * 0.25;
                }
            }
            // c[idx] = c[idx] * 0.25;
        }
    }
    FILE* f = fopen(path, "w");
    fprintf(f, "P3\n%d %d\n%d\n", w, h, 255);
    for (int i = 0; i < w * h; i++) {
        fprintf(f,"%d %d %d ", toInt(c[i].x), toInt(c[i].y), toInt(c[i].z));
    }
}

Sphere spheres[] = {
        Sphere(1e5, Vec3( 1e5+1,40.8,81.6), Vec3(),Vec3(.75,.25,.25),DIFF),//Left
        // Sphere(1e5, Vec3(-1e5+99,40.8,81.6),Vec3(),Vec3(.25,.25,.75),DIFF),//Rght
        // Sphere(1e5, Vec3(50,40.8, 1e5),     Vec3(),Vec3(.75,.75,.75),DIFF),//Back
        Sphere(1e5, Vec3(50,40.8,-1e5+170), Vec3(),Vec3(),           DIFF),//Frnt
        Sphere(1e5, Vec3(50, 1e5, 81.6),    Vec3(),Vec3(.75,.75,.75),DIFF),//Botm
        Sphere(1e5, Vec3(50,-1e5+81.6,81.6),Vec3(),Vec3(.75,.75,.75),DIFF),//Top
        Sphere(16.5,Vec3(27,16.5,47),       Vec3(),Vec3(1,1,1) * .99, SPEC),
        Sphere(16.5,Vec3(73,16.5,78),       Vec3(),Vec3(1,1,1) * .88, REFR),
        Sphere(7 ,Vec3(50,25,78),       Vec3(),Vec3(1,0,1) * .50, DIFF),
        Sphere(600, Vec3(50,681.6-.27,81.6),Vec3(12,12,12),  Vec3(), DIFF)
};

//Canvas(int _n, Sphere* _objects, Line3 _camera, char* _path);
//render(int samples, int w, int h, double dis, double ratio)

int main(int argc, char *argv[]) {
    // Line3 camera = Line3(Vec3(50,52,295.6), Vec3(0,-0.042612,-1).normal());
    // int w = 1024, h = 768, samps = (argc == 2 ? atoi(argv[1]) / 4 : 1);
    // double d = 140;
    // char path[] = "image.ppm";
    // Canvas can(8, spheres, camera, path);
    // can.render(samps, w, h, d, 0.5135);


    int w=1024, h=768, samps = (argc == 2 ? atoi(argv[1]) / 4 : 1);
    Line3 cam(Vec3(50,52,295.6), Vec3(0,-0.042612,-1).normal()); // cam pos, dir
    char path[] = "image.ppm";
    Canvas can(8, spheres, cam, path);
    Vec3 cx=Vec3(w*.5135 / h), cy=((cx.det(can.camera.d)).normal())*.5135, r, *c=new Vec3[w * h];
    Line3  rr;
    
#pragma omp parallel for schedule(dynamic, 1) private(r)       // OpenMP
    for (unsigned short y = 0; y < h; y++){                       // Loop over image rows
        unsigned short oo = y * y * y, Xi[3] = {0, 0, oo};
        fprintf(stderr,"\rRendering (%d spp) %5.2f%%", samps * 4, 100.* y/(h-1));
        for (unsigned short x = 0; x < w; x++)   // Loop cols
            for (int sy = 0, i = (h-y-1) * w + x; sy < 2; sy++)     // 2x2 subpixel rows
                for (int sx = 0; sx < 2; sx++, r = Vec3()){        // 2x2 subpixel cols
                    for (int s = 0; s < samps; s++){
                        double r1 = 2 * rnd::erand48(Xi), dx = r1<1 ? sqrt(r1) - 1 : 1 - sqrt(2 - r1);
                        double r2 = 2 * rnd::erand48(Xi), dy = r2<1 ? sqrt(r2) - 1 : 1 - sqrt(2 - r2);
                        // double r1 = 2 * rnd::erand48(Xi), dx = r1 - 1;
                        // double r2 = 2 * rnd::erand48(Xi), dy = r2 - 1;
                        Vec3 d = cx*( ( (sx+.5 + dx)/2 + x) / w - .5) +
                                cy*( ( (sy+.5 + dy)/2 + y) / h - .5) + cam.d;
                        r = r + can.radiance(Line3(cam.o+d * 140,d.normal()),0,Xi)*(1./samps);
                    } // Camera rays are pushed ^^^^^ forward to start in interior
                    c[i] = c[i] + Vec3(clamp(r.x),clamp(r.y),clamp(r.z)) *.25;
                }
    }
    FILE *f = fopen("image.ppm", "w");         // Write image to PPM file.
    fprintf(f, "P3\n%d %d\n%d\n", w, h, 255);
    for (int i=0; i<w*h; i++)
        fprintf(f,"%d %d %d ", toInt(c[i].x), toInt(c[i].y), toInt(c[i].z));
}