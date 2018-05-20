#include "Autolevels.hpp"
#include<iostream>




void build_histogram(valarray<float> &hist, const valarray<int> &src)
{
    const int size = src.size();
    for (int i = 0; i < size; i++)
    {
        hist[src[i]] += 1.f;
    }

    //find max
    float hist_max = hist.max();

    //normalize in the range [0...1]
    hist /= hist_max;
}

void compute_histogram_minmax(const valarray<float> &hist, const float threshold, float &minHist, float &maxHist)
{
    //Scaling factor: range [0..1]
    const float scalingFactor = 1.f/255.f;

    const int hist_size = hist.size();
    float CUMUL = hist.sum();

    //Start from max hist
    float hist_max = 0.f;
    int xa = 0;
    for (int i = 0; i < hist_size; i++)
    {
        if ( hist[i] > hist_max )
        {
            hist_max = hist[i];
            xa = i;
        }
    }

    if (xa >= hist_size - 1)
    {
        xa = hist_size - 2;
    }

    int xb = xa + 1;

    const float Threshold = threshold*CUMUL;

    float count = 0.f;
    bool decrease_xa = true;
    bool increase_xb = true;
    while (true)
    {
        count = hist[slice(xa, xb-xa+1, 1)].sum();
        if ( count >= Threshold)
            break;

        if (decrease_xa)
            xa--;

        if (xa <= 0)
        {
            xa = 0;
            decrease_xa = false;
        }

        count = hist[slice(xa, xb-xa+1, 1)].sum();

        if (count >= Threshold)
            break;

        if (increase_xb)
            xb++;

        if (xb >= hist_size - 1)
        {
            xb = hist_size - 1;
            increase_xb = false;
        }
    }
    minHist = scalingFactor*xa;
    maxHist = scalingFactor*xb;
}

 void computeAutolevels(float *x,float *y,float *z, int width,int height,const float threshold, float &minHist, float &maxHist, float &gamma)
{

	const int ELEMENTS = width * height;
    const int COLOR_DEPTH=256;
    float minR, maxR;
    float minG, maxG;
    float minB, maxB;
	valarray<int> red(ELEMENTS);
    valarray<int> green(ELEMENTS);
    valarray<int> blue(ELEMENTS);
    for (int i = 0;  i < ELEMENTS; i++)
    {
        red[i] = 255*x[i];
        green[i] = 255*y[i];
        blue[i] = 255*z[i];
    }

    //Build histogram
    valarray<float> histR(0.f, COLOR_DEPTH);
    build_histogram(histR, red);
    compute_histogram_minmax(histR, threshold, minR, maxR);
    //Build histogram
    valarray<float> histG(0.f, COLOR_DEPTH);
    build_histogram(histG, green);
    compute_histogram_minmax(histG, threshold, minG, maxG);
    //Build histogram
    valarray<float> histB(0.f, COLOR_DEPTH);
    build_histogram(histB, blue);
    compute_histogram_minmax(histB, threshold, minB, maxB);
    minHist = std::min(std::min(minG,minR), std::min(minG,minB));
    maxHist = std::max(std::max(maxG,maxR), std::max(maxG,maxB));
	//const float meanL = accumulate(lightness, lightness + ELEMENTS, 0.f)/ELEMENTS;
    //float midrange = minHist + .5f*(maxHist - minHist);
    //gamma = log10(midrange*255.f)/log10(meanL);
    gamma = 1.f; //TODO Let's return gamma = 1

}

