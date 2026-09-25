#define _CRT_SECURE_NO_WARNINGS

#include "model.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

typedef struct {
	Vec3* v; int v_count; int v_cap;
	int* idx; int i_count; int i_cap;
} Builder;

static bool builder_push_vertex(Builder* b, Vec3 p) {
	if (b->v_count >= b->v_cap) {
		int new_cap = b->v_cap ? b->v_cap * 2 : 256;
		Vec3* nv = (Vec3*)realloc(b->v, new_cap * sizeof(Vec3));
		if (!nv) return false;
		b->v = nv; b->v_cap = new_cap;
	}
	b->v[b->v_count++] = p;
	return true;
}

static bool builder_push_index(Builder* b, int i) {
	if (b->i_count >= b->i_cap) {
		int new_cap = b->i_cap ? b->i_cap * 2 : 512;
		int* ni = (int*)realloc(b->idx, new_cap * sizeof(int));
		if (!ni) return false;
		b->idx = ni; b->i_cap = new_cap;
	}
	b->idx[b->i_count++] = i;
	return true;
}
static int parse_face_line(const char* line, int vertex_total, int* out, int max_out) {
	int n = 0;
	const char* p = line + 1;
	while (*p && n < max_out) {
		while (*p == ' ' || *p == '\t') p++;
		if (*p == '\0' || *p == '\n' || *p == '\r') break;

		int idx = 0, sign = 1;
		if (*p == '-') { sign = -1; p++; }
		if (*p < '0' || *p > '9') break;

		while (*p >= '0' && *p <= '9') {
			idx = idx * 10 + (*p - '0');
			p++;
		}
		idx *= sign;

		if (idx < 0) idx = vertex_total + idx;
		else idx = idx - 1;

		if (idx < 0 || idx >= vertex_total) return -1;

		out[n++] = idx;
		while (*p && *p != ' ' && *p != '\t') p++;
	}
	return n;
}

Model* load_obj(const char* path, float target_length) {
	if (!path || !target_length) return NULL;
	printf("[model] trying to open: %s\n", path);

	FILE* f = NULL;
	errno_t err = fopen_s(&f, path, "r");
	if (err != 0 || f == NULL) {
		printf("[model] FATAL ERROR: %d", (int)err);
		return NULL;
	}
	printf("[model] file open\n");
	Builder b = { 0 };
	char line[512];

	int line_count = 0, v_count = 0, f_count = 0;
	while (fgets(line, sizeof(line), f)) {
		line_count++;
		if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
		if (line[0] == 'v' && (line[1] == ' ' || line[1] == '\t')) {
			Vec3 p;
			if (sscanf(line + 1, "%f %f %f", &p.x, &p.y, &p.z) == 3) {
				if (!builder_push_vertex(&b, p)) goto fail;
				v_count++;
			}
		}
		else if (line[0] == 'f' && (line[1] == ' ' || line[1] == '\t')) {
			int ids[64];
			int n = parse_face_line(line, b.v_count, ids, 64);
			if (n < 3) continue;

			for (int i = 1; i < n - 1; i++) {
				if (!builder_push_index(&b, ids[0])) goto fail;
				if (!builder_push_index(&b, ids[i])) goto fail;
				if (!builder_push_index(&b, ids[i + 1])) goto fail;
				f_count++;
			}
		}
	}
	fclose(f);
	printf("[model] lines read: %d, vertices: %d, faces: %d\n", line_count, v_count, f_count);
	if (b.v_count == 0 || b.i_count == 0) {
		printf("[model] error without vertices or indices"); 
		goto fail;
	}
	Vec3 mn = b.v[0], mx = b.v[0];
	for (int i = 1; i < b.v_count; i++) {
		if (b.v[i].x < mn.x) mn.x = b.v[i].x;
		if (b.v[i].x > mx.x) mn.x = b.v[i].x;
		if (b.v[i].y < mn.y) mn.y = b.v[i].y;
		if (b.v[i].y > mx.y) mn.y = b.v[i].y;
		if (b.v[i].z < mn.z) mn.z = b.v[i].z;
		if (b.v[i].z > mx.z) mn.z = b.v[i].z;
	}
	Vec3 center;
	center.x = (mn.x + mx.x) * 0.5f;
	center.y = (mn.y + mx.y) * 0.5f;
	center.z = (mn.z + mx.z) * 0.5f;

	for (int i = 0; i < b.v_count; i++) {
		b.v[i].x -= center.x;
		b.v[i].y -= center.y;
		b.v[i].z -= center.z;
	}

	Vec3 ext;
	ext.x = mx.x - mn.x;
	ext.y = mx.y - mn.y;
	ext.z = mx.z - mn.z;
	float longest = ext.x;
	if (ext.y > longest) longest = ext.y;
	if (ext.z > longest) longest = ext.z;
	if (longest < 1e-6f) longest = 1.0f;

	float scale = target_length / longest;

	float r2 = 0.0f;
	for (int i = 0; i < b.v_count; i++) {
		float d = b.v[i].x * b.v[i].x + b.v[i].y * b.v[i].y + b.v[i].z * b.v[i].z;
		if (d > r2) r2 = d;
	}
	float bound_radius = sqrtf(r2) * scale;

	Model* m = (Model*)malloc(sizeof(Model));
	if (!m) goto fail;

	m->positions = b.v;
	m->indices = b.idx;
	m->vertex_count = b.v_count;
	m->index_count = b.i_count;
	m->center = center;
	m->bound_radius = bound_radius;
	m->scale = scale;

	printf("[model] ok model: %d verts, %d indices, scale= %.4f\n", m->vertex_count, m->index_count, m->scale);
	return m;

fail:
	free(b.v);
	free(b.idx);
	return NULL;
}

Model* create_default(void) {
	static const Vec3 verts[] = {
		{0.0f , 0.0f, 25.0f},
		{0.0f, 5.0f, -15.0f},
		{0.0, -5.0f, -15.0f},
		{-22.0f, 0.0f, -3.0f},
		{22.0f, 0.0f, -3.0f},
	};
	static const int tris[] = {
	0,1,3,  0,4,1,   0,2,4,  0,3,2,  1,2,3,  1,4,2
	};
	int vc = (int)(sizeof(verts) / sizeof(verts[0]));
	int ic = (int)(sizeof(tris) / sizeof(tris[0]));

	Model* m = (Model*)malloc(sizeof(Model));
	if (!m) return NULL;

	m->positions = (Vec3*)malloc(sizeof(Vec3) * vc);
	m->indices = (int*)malloc(sizeof(int) * ic);
	if (!m->positions || !m->indices) {
		free(m->positions);
		free(m->indices);
		free(m);
		return NULL;
	}

	memcpy(m->positions, verts, sizeof(Vec3) * vc);
	memcpy(m->indices, tris, sizeof(int) * ic);

	m->vertex_count = vc;
	m->index_count = ic;
	m->center = (Vec3){ 0, 0, 0 };
	m->scale = 1.0f;
	m->bound_radius = 30.0f;
	return m;
}

void model_free(Model* m) {
	if (!m) return;
	free(m->positions);
	free(m->indices);
	free(m);
}