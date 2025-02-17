//
//  example 6-4
//
//
//
//   input to second cvLogPolar does not need "CV_WARP_FILL_OUTLIERS": "M, CV_INTER_LINEAR+CV_WARP_FILL_OUTLIERS+CV_WARP_INVERSE_MAP );" --> "M, CV_INTER_LINEAR+CV_WARP_INVERSE_MAP );"
//
//
//
//

// logPolar.cpp : Defines the entry point for the console application.
#include <cv.h>
#include <highgui.h>

int main(int argc, char** argv)
{
    IplImage* src;
   double M; 
    if( argc == 3 && ((src=cvLoadImage(argv[1],1)) != 0 ))
    {
      M = atof(argv[2]);
        IplImage* dst = cvCreateImage( cvGetSize(src), 8, 3 );
        IplImage* src2 = cvCreateImage( cvGetSize(src), 8, 3 );
        cvLogPolar( src,  dst, cvPoint2D32f(src->width/4,src->height/2),  
                    M, CV_INTER_LINEAR+CV_WARP_FILL_OUTLIERS );
        cvLogPolar( dst, src2, cvPoint2D32f(src->width/4,src->height/2),
                    M, CV_INTER_LINEAR+CV_WARP_INVERSE_MAP );
        cvNamedWindow( "log-polar", 1 );
        cvShowImage( "log-polar", dst );
        cvNamedWindow( "inverse log-polar", 1 );
        cvShowImage( "inverse log-polar", src2 );
        cvWaitKey();
    }
    return 0;
}
