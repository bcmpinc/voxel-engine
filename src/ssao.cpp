#include <random>
#include <glm/glm.hpp>

#include "surface.h"
#include "ssao.h"

ssao::ssao(int radius, double projection, int stride)
  : projection(projection)
  , radius(radius)
  , stride(stride)
{
    static const int PROBE_COUNT = sizeof(probes) / sizeof(probes[0]);
    std::mt19937_64 rand;
    std::uniform_real_distribution<> disc(-1, 1);
    for (int i=0; i<PROBE_COUNT; i++) {
        glm::dvec3 v;
        do {
            v = glm::dvec3(disc(rand),disc(rand),disc(rand));
        } while (glm::dot(v,v) > 1);
        probes[i].z = v.z*projection*(1<<30);
        probes[i].range = -sqrt(1 - v.x*v.x - v.y*v.y)*projection*(1<<30);
        probes[i].dx = v.x*radius + 0.5;
        probes[i].dy = v.y*radius + 0.5;
        probes[i].offset = probes[i].dx + probes[i].dy * stride;
    }
    
    for (int i=0; i<SSAO_PROBES_PER_PIXEL; i++) {
        double light = i/(double)SSAO_PROBES_PER_PIXEL;
        light *= light;
        light *= light;
        for (int j=0; j<256; j++) {
            lightmap[i][j] = j*light;
        }
    }
}


uint32_t ssao::modulate(uint32_t color, int light) {
    assert(light >= 0);
    assert(light < SSAO_PROBES_PER_PIXEL);
    int r = lightmap[light][(color>> 0) & 0xff];
    int g = lightmap[light][(color>> 8) & 0xff];
    int b = lightmap[light][(color>>16) & 0xff];
    return r | (g<<8) | (b<<16);
}

static int clamp(int val, int low, int high) {
    if (val <= low) return low;
    if (val >= high) return high;
    return val;
}

void ssao::apply(const surface& target) {
    int width = target.width;
    int height = target.height;
    int i=0;
    for (int y=0; y<height; y++) {
        for (int x=0; x<width; x++) {
            int probe_offset = (x%SSAO_SIZE + (y%SSAO_SIZE) * SSAO_SIZE) * SSAO_PROBES_PER_PIXEL;
            int count = 0;
            int64_t md = target.depth[i];
            for (int j=0; j<SSAO_PROBES_PER_PIXEL; j++) {
                const probe &p(probes[probe_offset+j]);
                int64_t pd1;
                int64_t pd2;
                if (x>=abs(p.dx) && y>=abs(p.dy) && x+abs(p.dx)<width && y+abs(p.dy)<height) {
                    pd1 = target.depth[i + p.offset];
                    pd2 = target.depth[i - p.offset];
                } else {
                    pd1 = target.depth[clamp(x+p.dx,0,width-1) + clamp(y+p.dy,0,height-1) * stride];
                    pd2 = target.depth[clamp(x-p.dx,0,width-1) + clamp(y-p.dy,0,height-1) * stride];
                }
                int64_t reld1 = (pd1-md)<<30;
                int64_t reld2 = (pd2-md)<<30;
                // Range check
                if (reld1 < p.range*md || reld2 < p.range*md) {
                    count++;
                } else {
                    // Occlusion check
                    if (reld1 >  p.z*md) count++;
                    if (reld2 > -p.z*md) count++;
                }
            }
            if (count < SSAO_PROBES_PER_PIXEL) {
                target.data[i] = modulate(target.data[i], count);
            }
            i++;
        }
    }
}
