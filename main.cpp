
#include <valarray>


#include <opencv2/opencv.hpp>
#include <opencv2/photo.hpp>
#include <sstream>
#include <fstream>
#include <string>
#include <iostream>
#include <vector>
#include <cmath>
#include "lib/compression_tmo.h"
#include "lib/progress.h"
#include "lib/Autolevels.hpp"
#include "lib/gamma_levels.h"
using namespace std;
using namespace cv;

#define WHITE_EFFICACY 179.0f

typedef unsigned char Trgbe;

//! \name RGB values and their exponent
struct Trgbe_pixel
{
    Trgbe r;
    Trgbe g;
    Trgbe b;
    Trgbe e;
};
#ifdef BRANCH_PREDICTION
#define likely(x)    __builtin_expect((x),1)
#define unlikely(x)    __builtin_expect((x),0)
#else
#define likely(x)    (x)
#define unlikely(x)    (x)
#endif
enum Colorspace {RGB, XYZ};

void readRadianceHeader(FILE *file, int &width, int &height, float &exposure, Colorspace &colorspace);
void RLERead(FILE* file, Trgbe* scanline, int size);
void rgbe2rgb(const Trgbe_pixel& rgbe, float exposure, float &r, float &g, float &b);
void readRadiance(FILE *file, int width, int height, float exposure,Mat &hdr_rgb);
inline float safelog10f( float x )
{
  if( unlikely(x < 1e-5f) )
    return -5.f;
  return log10f( x );
}

int main()
{

    for(int k=0;k<36;++k)
    {

        int width(0),height(0);
        float exposure(0.f);
        Colorspace colorspace;
        char name[2000] ;
        sprintf(name,"/home/peter/桌面/data/11/test/hdr_02/name_%02d.hdr",k);
        FILE* file = fopen(name, "rb");
        if(!file) {
        return false;
        }
        readRadianceHeader(file, width, height, exposure, colorspace);
        //cout<<"width:"<<width<<" "<<"height:"<<height<<" "<<endl;
        Mat hdr(height,width,CV_32FC3);
        readRadiance(file,width,height, exposure, hdr);
    float *x =new float[width*height];float *y =new float[width*height];float *z =new float[width*height];//float *l =new float[width*height];
    for(int i=0;i<hdr.rows;++i)
    for(int j=0;j<hdr.cols;++j)
    {
        float b=(hdr.at<Vec3f>(i, j)[0]);
        float g=(hdr.at<Vec3f>(i, j)[1]);
        float r=(hdr.at<Vec3f>(i, j)[2]);

       // l[hdr.cols*i+j]=(hdr1.at<float>(i, j));
        x[hdr.cols*i+j]=b;
        y[hdr.cols*i+j]=g;
        z[hdr.cols*i+j]=r;
    }
    CompressionTMO tmo;
    Progress ph;
    ph.setMaximum(100);
    //tonemap得到的是归一化的像素的值
    tmo.tonemap( x, y, z, width, height, x, y, z, y, ph);
    float minL, maxL, gammaL;
    computeAutolevels(x,y,z,width, height, 0.985f, minL, maxL, gammaL);
    gammaAndLevels(x,y,z,width,height,minL,maxL,0.f,1.f,gammaL);
    Mat ldr (height,width,CV_8UC3);
    for(int i=0;i<ldr.rows;++i)
    for(int j=0;j<ldr.cols;++j)
    {
        float b=x[ldr.cols*i+j];
        float g=y[ldr.cols*i+j];
        float r=z[ldr.cols*i+j];
        ldr.at<Vec3b>(i, j)[0]=(int) 255*b;
        ldr.at<Vec3b>(i, j)[1]=(int) 255*g;
        ldr.at<Vec3b>(i, j)[2]=(int) 255*r;
    }
    imshow("tone_mapping",ldr);
    waitKey(400);
    delete [] x;
    delete [] y;
    delete [] z;
    char nameldr[2000];
    sprintf(nameldr,"/home/peter/桌面/data/18/test1/ldr_%02d.jpg",k);
    imwrite(nameldr, ldr);

    }
    return 0;

}



void readRadianceHeader(FILE *file, int &width, int &height, float &exposure, Colorspace &colorspace)
{
    // DEBUG_STR << "RGBE: reading header..." << endl;

    // read header information
    char head[255];
    float fval;
    int format = 0;
    exposure = 1.0f;

    while ( !feof(file) )
    {
        if ( fgets(head, 200, file) == NULL ) {
            throw ("RGBE: invalid header");
        }
        if ( strcmp(head, "\n")==0 ) break;
        if ( strcmp(head, "#?RADIANCE\n") == 0 ) {
            // format specifier found
            format = 1;
        }
        if ( strcmp(head, "#?RGBE\n") == 0 ) {
            // format specifier found
            format = 1;
        }
        if ( strcmp(head, "#?AUTOPANO\n") == 0 ) {
            // format specifier found
            format=1;
        }
        if ( head[0]=='#' ) {
            // comment found - skip
            continue;
        }
        if ( strcmp(head, "FORMAT=32-bit_rle_rgbe\n") == 0 ) {
            // header found
          //  cout<<"lk1"<<endl;
            colorspace = RGB;
            continue;
        }
        if ( strcmp(head, "FORMAT=32-bit_rle_xyze\n") == 0 ) {
            // header found
            //cout<<"lk2"<<endl;
            colorspace = XYZ;
            continue;
        }
        if ( sscanf(head, "EXPOSURE=%f", &fval) == 1 )
        {
            // exposure value
            exposure *= fval;
            cout<<exposure<<endl;
        }
    }

    // ignore wierd exposure adjustments
    if ( exposure > 1e12 || exposure < 1e-12 ) {
        exposure = 1.0f;
    }

    if ( !format )
    {
        throw ( "RGBE: no format specifier found" );
    }

    // image size
    char xbuf[4], ybuf[4];
    if ( fgets(head, sizeof(head)/sizeof(head[0]), file) == NULL ||
         sscanf(head, "%3s %d %3s %d", ybuf, &height, xbuf, &width) != 4 )
    {
        throw ( "RGBE: unknown image size" );
    }

    assert(height > 0);
    assert(width > 0);


    // DEBUG_STR << "RGBE: image size " << width << "x" << height << endl;
}
void RLERead(FILE* file, Trgbe* scanline, int size)
{
    int peek = 0;
    while ( peek < size )
    {
        Trgbe p[2];
        if ( fread(p, sizeof(p), 1, file) == 0) {
            throw("RGBE: Invalid data size");
        }
        if ( p[0]>128 )
        {
            // a run
            int run_len = p[0]-128;

            while ( run_len > 0 )
            {
                scanline[peek++] = p[1];
                run_len--;
            }
        }
        else
        {
            // a non-run
            scanline[peek++] = p[1];

            int nonrun_len = p[0]-1;
            if ( nonrun_len > 0 )
            {
                if ( fread(scanline+peek, sizeof(*scanline), nonrun_len, file) == 0) {
                    throw ("RGBE: Invalid data size");
                }
                else {
                    peek += nonrun_len;
                }
            }
        }
    }
    if ( peek != size )
    {
        throw( "RGBE: difference in size while reading RLE scanline");
    }

}
void rgbe2rgb(const Trgbe_pixel& rgbe, float exposure, float &r, float &g, float &b)
{
    if ( rgbe.e != 0 )        // a non-zero pixel
    {
        int e = rgbe.e - int(128+8);
        double f = ldexp( 1.0, e ) * WHITE_EFFICACY / exposure;
        //cout<<"rgbe.e: "<<(int) rgbe.e<<" e:"<<e<<" f:"<<f<<endl;
        r = (float)(rgbe.r * f);
        g = (float)(rgbe.g * f);
        b = (float)(rgbe.b * f);
       //cout<<" r:"<<r<<" g:"<<g<<" b:"<<b<<endl;
    }
    else
        r = g = b = 0.f;
}

void readRadiance(FILE *file, int width, int height, float exposure,Mat &hdr_rgb)
                  //pfs::Array2Df &X, pfs::Array2Df &Y, pfs::Array2Df &Z)
{
    // read image
    // depending on format read either rle or normal (note: only rle supported)
    std::vector<Trgbe> scanline(width*4);
    //Mat hdr_rgb(width,height,CV_32FC3);
    for (int y = 0; y < height; ++y)
    {
        // read rle header
        Trgbe header[4];
        if ( fread(header, sizeof(header), 1, file) == sizeof(header) ) {
            throw ("RGBE: invalid data size");
        }
        if ( header[0] != 2 || header[1] != 2 || (header[2]<<8) + header[3] != width )
        {
            //--- simple scanline (not rle)
            size_t rez = fread(scanline.data()+4, sizeof(Trgbe), 4*width-4, file);
            if ( rez != (size_t)4*width-4 )
            {
                //     DEBUG_STR << "RGBE: scanline " << y
                //           << "(" << (int)rez << "/" << width << ")" <<endl;
                throw ( "RGBE: not enough data to read "
                                      "in the simple format." );
            }
            //--- yes, we've read one pixel as a header
            scanline[0] = header[0];
            scanline[1] = header[1];
            scanline[2] = header[2];
            scanline[3] = header[3];

            //--- write scanline to the image
            for (int x=0 ; x<width ; ++x)
            {
                Trgbe_pixel rgbe;
                rgbe.r = scanline[4*x+0];
                rgbe.g = scanline[4*x+1];
                rgbe.b = scanline[4*x+2];
                rgbe.e = scanline[4*x+3];
                //cout<<(float) rgbe.r<<" "<<(float) rgbe.g<<" "<<(float) rgbe.b<<" "<<(float) rgbe.e<<endl;
                rgbe2rgb(rgbe, exposure, hdr_rgb.at<Vec3f>(y,x)[2], hdr_rgb.at<Vec3f>(y,x)[1], hdr_rgb.at<Vec3f>(y,x)[0]);
            }
        }
        else
               {
                   //--- rle scanline
                   //--- each channel is encoded separately
                   for (int ch = 0; ch < 4 ; ++ch) {
                       RLERead(file, scanline.data()+width*ch, width);
                   }

                   //--- write scanline to the image
                   for (int x = 0; x < width; ++x)
                   {
                       Trgbe_pixel rgbe;
                       rgbe.r = scanline[x+width*0];
                       rgbe.g = scanline[x+width*1];
                       rgbe.b = scanline[x+width*2];
                       rgbe.e = scanline[x+width*3];

                       rgbe2rgb(rgbe, exposure,hdr_rgb.at<Vec3f>(y,x)[2], hdr_rgb.at<Vec3f>(y,x)[1], hdr_rgb.at<Vec3f>(y,x)[0]);
                   }
               }

    }

}




