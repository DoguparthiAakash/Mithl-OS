#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "types.h"
#include "graphics.h"

/* Triangle rendering functions */

// Fill a solid-color triangle
void draw_triangle_filled(point_t p1, point_t p2, point_t p3, uint32_t color);

// Draw triangle with gradient (interpolated colors at vertices)
void draw_triangle_gradient(point_t p1, point_t p2, point_t p3,
                           uint32_t c1, uint32_t c2, uint32_t c3);

// Draw triangle outline
void draw_triangle_outline(point_t p1, point_t p2, point_t p3, uint32_t color);

// Helper: Sort points by Y coordinate
void sort_points_by_y(point_t* p1, point_t* p2, point_t* p3);

#endif /* TRIANGLE_H */
