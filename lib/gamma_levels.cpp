

#include <cmath>
#include <iostream>



template <typename T>
inline T clamp(const T& v, const T& lower_bound, const T& upper_bound)
{
    if ( v <= lower_bound ) return lower_bound;
    if ( v >= upper_bound ) return upper_bound;
    return v;
}

////! \note I assume that *in* contains only value between [0,1]
//void gamma_levels_array(const pfs::Array2D* in, pfs::Array2D* out,
//                        float black_in, float white_in,
//                        float black_out, float white_out, float gamma)
//{
//    // same formula used inside GammaAndLevels::refreshLUT()
//    //float value = powf( ( ((float)(i)/255.0f) - bin ) / (win-bin), expgamma);
//    //LUT[i] = clamp(blackout+value*(whiteout-blackout),0,255);

//    const float* in_vector = in->getRawData();
//    float* out_vector = out->getRawData();

//    const int ELEMS = in->getCols()*in->getRows();

//    if (gamma != 1.0f)
//    {
//#pragma omp parallel for
//        for (int idx = 0; idx < ELEMS; ++idx)
//        {
//            float tmp = (in_vector[idx] - black_in)/(white_in - black_in);
//            tmp = powf(tmp, gamma);

//            tmp = black_out + tmp*(white_out-black_out);

//            out_vector[idx] = clamp(tmp, 0.0f, 1.0f);
//        }
//    }
//    else
//    {
//#pragma omp parallel for
//        for (int idx = 0; idx < ELEMS; ++idx)
//        {
//            float tmp = (in_vector[idx] - black_in)/(white_in - black_in);
//            //tmp = powf(tmp, gamma);

//            tmp = black_out + tmp*(white_out-black_out);

//            out_vector[idx] = clamp(tmp, 0.0f, 1.0f);
//        }
//    }



void gammaAndLevels(float *x,float *y,float *z, int width,int heihgt,
		    float black_in, float white_in,
                    float black_out, float white_out,
                    float gamma)
{

    const int outWidth   = width;
    const int outHeight  = heihgt;

   /* float* R_i;float* G_i;float* B_i;
    R_i = x;
    G_i = y;
    B_i = z;*/
    // float exp_gamma = 1.f/gamma;
#pragma omp parallel for
    for (int idx = 0; idx < outWidth*outHeight; ++idx)
    {
        float red = x[idx];
        float green = y[idx];
        float blue = z[idx];

        float L = 0.2126f * red
                + 0.7152f * green
                + 0.0722f * blue; // number between [0..1]

        float c = powf(L, gamma - 1.0f);

        red = (red - black_in) / (white_in - black_in);
        red *= c;

        green = (green - black_in) / (white_in - black_in);
        green *= c;

        blue = (blue - black_in) / (white_in - black_in);
        blue *= c;

        x[idx] = clamp(black_out + red * (white_out - black_out), 0.f, 1.f);
        y[idx] = clamp(black_out + green * (white_out - black_out), 0.f, 1.f);
        z[idx] = clamp(black_out + blue * (white_out - black_out), 0.f, 1.f);
    }


}


