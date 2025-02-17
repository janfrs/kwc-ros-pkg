//
// void main( --> int main( 
//
//


#include "cxcore.h"
#include "highgui.h"

int main( int argc, char** argv )
{



    float priors[] = { 1.0, 10.0}; // Edible vs poisonos weights

    CvMat* var_type;
    CvMat* data; // jmh add
    data     = cvCreateMat( 20, 30, CV_8U ); // jmh add

    var_type = cvCreateMat( data->cols + 1, 1, CV_8U );
    cvSet( var_type, cvScalarAll(CV_VAR_CATEGORICAL) ); // all these vars 
                                                        // are categorical
    CvDTree* dtree;
    dtree = new CvDTree;
    dtree->train( data, CV_ROW_SAMPLE, responses, 0, 0, var_type, missing,
                  CvDTreeParams(  8, // max depth
                                 10, // min sample count
                                  0, // regression accuracy: N/A here
                               true, // compute surrogate split, 
                                     //   as we have missing data
                                 15, // max number of categories 
                                     //   (use sub-optimal algorithm for
                                     //   larger numbers)
                                 10, // cross-validations 
                               true, // use 1SE rule => smaller tree
                               true, // throw away the pruned tree branches
                              priors // the array of priors, the bigger 
                                     //   p_weight, the more attention
                                     //   to the poisonous mushrooms
                              )
                );



    dtree->save("tree.xml","MyTree");
    dtree->clear();
    dtree->load("tree.xml",”MyTree”);



    #define MAX_CLUSTERS 5
    CvScalar color_tab[MAX_CLUSTERS];
    IplImage* img = cvCreateImage( cvSize( 500, 500 ), 8, 3 );
    CvRNG rng = cvRNG(0xffffffff);
    
    color_tab[0] = CV_RGB(255,0,0);
    color_tab[1] = CV_RGB(0,255,0);
    color_tab[2] = CV_RGB(100,100,255);
    color_tab[3] = CV_RGB(255,0,255);
    color_tab[4] = CV_RGB(255,255,0);

    cvNamedWindow( "clusters", 1 );

    for(;;)
    {
        int k, cluster_count = cvRandInt(&rng)%MAX_CLUSTERS + 1;
        int i, sample_count = cvRandInt(&rng)%1000 + 1;
        CvMat* points = cvCreateMat( sample_count, 1, CV_32FC2 );
        CvMat* clusters = cvCreateMat( sample_count, 1, CV_32SC1 );

        /* generate random sample from multivariate 
           Gaussian distribution */
        for( k = 0; k < cluster_count; k++ )
        {
            CvPoint center;
            CvMat point_chunk;
            center.x = cvRandInt(&rng)%img->width;
            center.y = cvRandInt(&rng)%img->height;
            cvGetRows( points, &point_chunk, 
                       k*sample_count/cluster_count,
                       k == cluster_count - 1 ? sample_count :  
                       (k+1)*sample_count/cluster_count );
            cvRandArr( &rng, &point_chunk, CV_RAND_NORMAL,
                       cvScalar(center.x,center.y,0,0),
                       cvScalar(img->width/6, img->height/6,0,0) );
        }

        /* shuffle samples */
        for( i = 0; i < sample_count/2; i++ )
        {
            CvPoint2D32f* pt1 = (CvPoint2D32f*)points->data.fl +
                                 cvRandInt(&rng)%sample_count;
            CvPoint2D32f* pt2 = (CvPoint2D32f*)points->data.fl + 
                                 cvRandInt(&rng)%sample_count;
            CvPoint2D32f temp;
            CV_SWAP( *pt1, *pt2, temp );
        }

        cvKMeans2( points, cluster_count, clusters,
                   cvTermCriteria( CV_TERMCRIT_EPS+CV_TERMCRIT_ITER, 
                                   10, 1.0 ));
        cvZero( img );
        for( i = 0; i < sample_count; i++ )
        {
            CvPoint2D32f pt = ((CvPoint2D32f*)points->data.fl)[i];
            int cluster_idx = clusters->data.i[i];
            cvCircle( img, cvPointFrom32f(pt), 2, 
                      color_tab[cluster_idx], CV_FILLED );
        }

        cvReleaseMat( &points );
        cvReleaseMat( &clusters );

        cvShowImage( "clusters", img );

        int key = cvWaitKey(0);
        if( key == 27 ) // 'ESC'
            break;
    }
}
