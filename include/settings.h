#ifndef _SETTINGS_H
#define _SETTINGS_H

// all the settings
#define WIDTH  			1440		// dimensions of the image in pixels
#define HEIGHT			1080
#define MAX_BOUNCES 	8		// how many times ray can bounce before dying off
#define N_SAMPLES		64		// number of samples per pixel

//advanced (dont touch)
#define BVH_MAX_DEPTH   24
#define BVH_LEAF_TRIS   4

#endif
