#include <assert.h>
#include "common.h"
#include "point.h"
#include "math.h"

void
point_translate(struct point *p, double x, double y)
{
  point_set(p,point_X(p)+x,point_Y(p)+y);
  return;
}

double
point_distance(const struct point *p1, const struct point *p2)
{
  double distance = sqrt(pow(point_X(p1) - point_X(p2),2) + pow(point_Y(p1) - point_Y(p2),2));
  return distance;
}

int
point_compare(const struct point *p1, const struct point *p2)
{
  double d1= sqrt(pow(point_X(p1),2) + pow(point_Y(p1),2));
  double d2= sqrt(pow(point_X(p2),2) + pow(point_Y(p2),2));
	
  if(d1 < d2) return -1;
  else if(d1 == d2) return 0;
  else return 1;
}
