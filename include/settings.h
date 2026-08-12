#ifndef _SETTINGS_H
#define _SETTINGS_H

// all the settings

#define CAMERA_LOOKAT 	{ 0.0, 0.03, .55 }
#define CAMERA_POS	 	{ -0.338136, -2.16438, 1.20508}
#define CAMERA_FOV		30

#define WIDTH  			480		// dimensions of the image in pixels
#define HEIGHT			720
#define MAX_BOUNCES 	8		// how many times ray can bounce before dying off
#define N_SAMPLES		256		// number of samples per pixel

//advanced (dont touch)
#define BVH_MAX_DEPTH   24
#define BVH_LEAF_TRIS   4

#endif
