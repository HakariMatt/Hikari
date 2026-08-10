#ifndef _SETTINGS_H
#define _SETTINGS_H

// all the settings

#define CAMERA_LOOKAT 	{  0.002459,  0.000000, 0.718907}
#define CAMERA_POS	 	{  0.735935, -0.487491, 0.776759}
#define CAMERA_FOV		20

#define WIDTH  			640		// dimensions of the image in pixels
#define HEIGHT			480
#define MAX_BOUNCES 	8		// how many times ray can bounce before dying off
#define N_SAMPLES		64		// number of samples per pixel

//advanced (dont touch)
#define BVH_MAX_DEPTH   24
#define BVH_LEAF_TRIS   4

#endif
