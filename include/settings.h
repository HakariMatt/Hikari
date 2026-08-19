#ifndef _SETTINGS_H
#define _SETTINGS_H

// all the settings

#define CAMERA_LOOKAT 	{ 0.0, 0.0, 1 }
#define CAMERA_POS	 	{ 0, -5.67408 , 1}
#define CAMERA_FOV		24.1

#define WIDTH  			1920		// dimensions of the image in pixels
#define HEIGHT			1920
#define MAX_BOUNCES 	4		// how many times ray can bounce before dying off
#define N_SAMPLES		4096		// number of samples per pixel

//advanced (dont touch)
#define BVH_MAX_DEPTH   24
#define BVH_LEAF_TRIS   4

#endif
