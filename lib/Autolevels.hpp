#include<iostream>
#include <valarray>


using namespace std;



void build_histogram(valarray<float> &hist, const valarray<int> &src);
void compute_histogram_minmax(const valarray<float> &hist, const float threshold, float &minHist, float &maxHist);
void computeAutolevels(float *x,float *y,float *z, int width,int height,const float threshold, float &minHist, float &maxHist, float &gamma);
