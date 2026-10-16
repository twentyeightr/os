#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

typedef struct {
	double x, y, z;
} coords;

static double triangle(coords a, coords b, coords c) {
	coords ab = { b.x - a.x, b.y - a.y, b.z - a.z };
	coords ac = { c.x - a.x, c.y - a.y, c.z - a.z };
	coords ab_ac = { ab.y * ac.z - ab.z * ac.y, ab.z * ac.x - ab.x * ac.z, ab.x * ac.y - ab.y * ac.x };
	return 0.5 * sqrt(ab_ac.x * ab_ac.x + ab_ac.y * ab_ac.y + ab_ac.z * ab_ac.z);
}

static double get_time_ms(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

int main(int argc, char** argv) {
	if (argc != 2) {
		fprintf(stderr, "usage: %s points.txt\n", argv[0]);
		return 1;
	}

	FILE* file = fopen(argv[1], "r");
	if (!file) {
		fprintf(stderr, "fopen failed\n");
		return 1;
	}

	int n_points;
	if (fscanf(file, "%d", &n_points) != 1 || n_points < 3) {
		fprintf(stderr, "error: invalid number of points\n");
		fclose(file);
		return 1;
	}

	coords* points = malloc(n_points * sizeof(coords));
	if (!points) {
		fclose(file);
		return 1;
	}

	for (int i = 0; i < n_points; ++i) {
		if (fscanf(file, "%lf %lf %lf", &points[i].x, &points[i].y, &points[i].z) != 3) {
			fprintf(stderr, "error: invalid point #%d\n", i);
			free(points);
			fclose(file);
			return 1;
		}
	}
	fclose(file);

	double t0 = get_time_ms();

	double maxS = 0.0;
	int bi = 0, bj = 0, bl = 0;
	for (int i = 0; i < n_points - 2; ++i)
		for (int j = i + 1; j < n_points - 1; ++j)
			for (int l = j + 1; l < n_points; ++l) {
				double s = triangle(points[i], points[j], points[l]);
				if (s > maxS) {
					maxS = s;
					bi = i;
					bj = j;
					bl = l;
				}
			}

	double t = get_time_ms() - t0;

	printf("S:\t%.6f\n", maxS);
	printf("points:\t%d %d %d\n", bi, bj, bl);
	printf("time:\t%.3f\n", t);

	free(points);
	return 0;
}
