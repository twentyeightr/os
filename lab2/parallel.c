#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <pthread.h>

typedef struct {
	double x, y, z;
} coords;

typedef struct {
	coords* point;
	int n;
	int start, end;
} ThreadArgs;

static double global_best = 0.0;
static int gbi = 0, gbj = 0, gbl = 0;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

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

static void* work(void* _args) {
	ThreadArgs* args = (ThreadArgs*)_args;

	double local_best = 0.0;
	int lbi = 0, lbj = 0, lbl = 0;

	for (int i = args->start; i < args->end; i++)
		for (int j = i + 1; j < args->n - 1; j++)
			for (int l = j + 1; l < args->n; l++) {
				double s = triangle(args->point[i], args->point[j], args->point[l]);
				if (s > local_best) {
					local_best = s;
					lbi = i;
					lbj = j;
					lbl = l;
				}
			}

	pthread_mutex_lock(&mutex);
	if (local_best > global_best) {
		global_best = local_best;
		gbi = lbi;
		gbj = lbj;
		gbl = lbl;
	}
	pthread_mutex_unlock(&mutex);

	return NULL;
}

int main(int argc, char** argv) {
	if (argc != 3) {
		fprintf(stderr, "usage: %s points.txt thread_count\n", argv[0]);
		return 1;
	}

	size_t n_threads = atoi(argv[2]);
	if (n_threads < 1 || n_threads > 4096) {
		fprintf(stderr, "error: invalid thread count\n");
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

	pthread_t* threads = malloc(n_threads * sizeof(pthread_t));
	ThreadArgs* thread_args = malloc(n_threads * sizeof(ThreadArgs));

	int start = 0;
	double t0 = get_time_ms();

	for (size_t i = 0; i < n_threads; i++) {
		int chunk = (n_points - 2) / (int)n_threads
		          + ((int)i < (n_points - 2) % (int)n_threads ? 1 : 0);
		thread_args[i] = (ThreadArgs){
			.point = points,
			.n = n_points,
			.start = start,
			.end = start + chunk
		};
		start += chunk;
		pthread_create(&threads[i], NULL, work, &thread_args[i]);
	}

	for (size_t i = 0; i < n_threads; i++)
		pthread_join(threads[i], NULL);

	double t = get_time_ms() - t0;

	printf("S:\t%.6f\n", global_best);
	printf("points:\t%d %d %d\n", gbi, gbj, gbl);
	printf("time:\t%.3f\n", t);

	free(thread_args);
	free(threads);
	free(points);
	return 0;
}
