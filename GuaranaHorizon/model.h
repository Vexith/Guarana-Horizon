#ifndef MODEL_H
#define MODEL_H

#include "vector.h"

typedef struct {
	Vec3* positions;
	int* indices;
	int vertex_count;
	int index_count;

	Vec3 center;
	float scale;
	float bound_radius;
} Model;

Model* load_obj(const char* path, float target_length);
Model* create_default(void);
void model_free(Model* m);

#endif