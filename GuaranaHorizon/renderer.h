#ifndef RENDERER_H
#define RENDERER_H

#include "camera.h"
#include "aircraft.h"
#include <SDL3/SDL.h>

int renderer_init(void);
void shutdown(void);

void clear_renderer(void);
void renderer_present(void);

void ddraw(const Aircraft* aircraft);
int project_point(const Camera* camera, Vec3 point, float* screen_x, float* screen_y);
void draw_airplane(const Aircraft* aircraft, const Camera* camera);

#endif