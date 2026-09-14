#ifndef RENDERER_H
#define RENDERER_H

#include "aircraft.h"
#include <SDL3/SDL.h>

int renderer_init(void);
void shutdown(void);

void clear_renderer(void);
void renderer_present(void);

void ddraw(const Aircraft* aircraft);

#endif