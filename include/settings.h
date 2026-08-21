#ifndef _SETTINGS_H
#define _SETTINGS_H

// all the settings

#define CAMERA_LOOKAT 	{ 0.0, 0.0, 1 }
#define CAMERA_POS	 	{ 0, -5.67408 , 1}
// #define CAMERA_LOOKAT 	{ 0.0, 0.0, 4.52425 }
// #define CAMERA_POS	 	{ 9.52398, 0 , 1.5754}
#define CAMERA_FOV		24

#define WIDTH  			480		// dimensions of the image in pixels
#define HEIGHT			480
#define MAX_BOUNCES 	8		// how many times ray can bounce before dying off
#define N_SAMPLES		256		// number of samples per pixel

#define LAMBDA_MIN		380.0		// lower bound of spectral range (nm). λ_min < λ_max
#define LAMBDA_MAX		780.0		// upper bound of spectral range (nm).

//advanced (dont touch)
#define BVH_MAX_DEPTH   24
#define BVH_LEAF_TRIS   4
#define LAMBDA_BAR (LAMBDA_MAX - LAMBDA_MIN)
#define CMF_NORM_K (1 / 106.856) // precomputed. ≈∫ȳ(λ)dλ

#endif
