#ifdef _CH_
#pragma package <opencv>
#endif

#ifndef _EiC
#include "cv.h"
#include "highgui.h"
#include <stdio.h>
#include <time.h>
#endif

/* the script demostrates iterative construction of
   delaunay triangulation and voronoi tesselation */

//INITILIZATION CONVENIENCE FUNCTION FOR DELAYNAY SUBDIVISION
CvSubdiv2D* init_delaunay( CvMemStorage* storage, CvRect rect )
{
    CvSubdiv2D* subdiv;
    subdiv = cvCreateSubdiv2D( CV_SEQ_KIND_SUBDIV2D, sizeof(*subdiv),
                               sizeof(CvSubdiv2DPoint),
                               sizeof(CvQuadEdge2D),
                               storage );
    cvInitSubdivDelaunay2D( subdiv, rect ); //rect sets the bounds
    return subdiv;
}
void draw_subdiv_point( IplImage* img, CvPoint2D32f fp, CvScalar color )
{
    cvCircle( img, cvPoint(cvRound(fp.x), cvRound(fp.y)), 3, color, CV_FILLED, 8, 0 );
}

//Report is to turn on drawing of outer edges in different (pink) color
void draw_subdiv_edge( IplImage* img, CvSubdiv2DEdge edge, CvScalar color, int report )
{
    CvSubdiv2DPoint* org_pt;
    CvSubdiv2DPoint* dst_pt;
    CvPoint2D32f org;
    CvPoint2D32f dst;
    CvPoint iorg, idst;

    org_pt = cvSubdiv2DEdgeOrg(edge);
    dst_pt = cvSubdiv2DEdgeDst(edge);

    if( org_pt && dst_pt )
    {
        org = org_pt->pt;
        dst = dst_pt->pt;

        iorg = cvPoint( cvRound( org.x ), cvRound( org.y ));
        idst = cvPoint( cvRound( dst.x ), cvRound( dst.y ));
		if(report){ //Just showing that this is where you can detect outer points.
		if(org.x < 0 || org.y < 0 || dst.x < 0 || dst.y < 0 ||
				org.x > 599 || org.y > 599 || dst.x > 599 || dst.y > 599){
			printf("OUTER POINT org(%f, %f) dst(%f, %f)\n",org.x,org.y,dst.x,dst.y);
			color = CV_RGB(200,0,200);  //Draw outer edged in pretty colors
		}
		}

        cvLine( img, iorg, idst, color, 1, CV_AA, 0 );
    }
}


void draw_subdiv( IplImage* img, CvSubdiv2D* subdiv,
                  CvScalar delaunay_color, CvScalar voronoi_color )
{
    CvSeqReader  reader;
    int i, total = subdiv->edges->total;
    int elem_size = subdiv->edges->elem_size;

    cvStartReadSeq( (CvSeq*)(subdiv->edges), &reader, 0 );

    for( i = 0; i < total; i++ )
    {
        CvQuadEdge2D* edge = (CvQuadEdge2D*)(reader.ptr);

        if( CV_IS_SET_ELEM( edge ))
        {
            draw_subdiv_edge( img, (CvSubdiv2DEdge)edge + 1, voronoi_color,0 );
            draw_subdiv_edge( img, (CvSubdiv2DEdge)edge, delaunay_color,0 );
        }

        CV_NEXT_SEQ_ELEM( elem_size, reader );
    }
}


void locate_point( CvSubdiv2D* subdiv, CvPoint2D32f fp, IplImage* img,
                   CvScalar active_color )
{
    CvSubdiv2DEdge e;
    CvSubdiv2DEdge e0 = 0;
    CvSubdiv2DPoint* p = 0;

    cvSubdiv2DLocate( subdiv, fp, &e0, &p );
    if( e0 )
    {
        e = e0;
        do //Always 3 edges -- this is a triangulation after all.
        {
            draw_subdiv_edge( img, e, active_color,1 );
            e = cvSubdiv2DGetEdge(e,CV_NEXT_AROUND_LEFT);
        }
        while( e != e0 );
    }
    draw_subdiv_point( img, fp, active_color );
}


void draw_subdiv_facet( IplImage* img, CvSubdiv2DEdge edge )
{
    CvSubdiv2DEdge t = edge;
    int i, count = 0;
    CvPoint* buf = 0;

    // count number of edges in facet
    do
    {
        count++;
        t = cvSubdiv2DGetEdge( t, CV_NEXT_AROUND_LEFT );
    } while (t != edge );

    buf = (CvPoint*)malloc( count * sizeof(buf[0]));

    // gather points
    t = edge;
    for( i = 0; i < count; i++ )
    {
        CvSubdiv2DPoint* pt = cvSubdiv2DEdgeOrg( t );
        if( !pt ) break;
        buf[i] = cvPoint( cvRound(pt->pt.x), cvRound(pt->pt.y));
        t = cvSubdiv2DGetEdge( t, CV_NEXT_AROUND_LEFT );
    }

    if( i == count )
    {
        CvSubdiv2DPoint* pt = cvSubdiv2DEdgeDst( cvSubdiv2DRotateEdge( edge, 1 ));
        cvFillConvexPoly( img, buf, count, CV_RGB(rand()&255,rand()&255,rand()&255), CV_AA, 0 );
        cvPolyLine( img, &buf, &count, 1, 1, CV_RGB(0,0,0), 1, CV_AA, 0);
        draw_subdiv_point( img, pt->pt, CV_RGB(0,0,0));
    }
    free( buf );
}

void paint_voronoi( CvSubdiv2D* subdiv, IplImage* img )
{
    CvSeqReader  reader;
    int i, total = subdiv->edges->total;
    int elem_size = subdiv->edges->elem_size;

    cvCalcSubdivVoronoi2D( subdiv );

    cvStartReadSeq( (CvSeq*)(subdiv->edges), &reader, 0 );

    for( i = 0; i < total; i++ )
    {
        CvQuadEdge2D* edge = (CvQuadEdge2D*)(reader.ptr);

        if( CV_IS_SET_ELEM( edge ))
        {
            CvSubdiv2DEdge e = (CvSubdiv2DEdge)edge;
            // left
            draw_subdiv_facet( img, cvSubdiv2DRotateEdge( e, 1 ));

            // right
            draw_subdiv_facet( img, cvSubdiv2DRotateEdge( e, 3 ));
        }

        CV_NEXT_SEQ_ELEM( elem_size, reader );
    }
}


void run(void)
{
    char win[] = "source";
    int i;
    CvRect rect = { 0, 0, 600, 600 };
    CvMemStorage* storage;
    CvSubdiv2D* subdiv;
    IplImage* img;
    CvScalar active_facet_color, delaunay_color, voronoi_color, bkgnd_color;

    active_facet_color = CV_RGB( 255, 0, 0 );
    delaunay_color  = CV_RGB( 0,0,0);
    voronoi_color = CV_RGB(0, 180, 0);
    bkgnd_color = CV_RGB(255,255,255);

    img = cvCreateImage( cvSize(rect.width,rect.height), 8, 3 );
    cvSet( img, bkgnd_color, 0 );

    cvNamedWindow( win, 1 );

    storage = cvCreateMemStorage(0);
    subdiv = init_delaunay( storage, rect );

    printf("\n\nDelaunay triangulation will be built now interactively.\n"
           "'q' or ESC quits, otherwise any other key to keep locating and inserting points\n\n");
	char c;
	srand( time(NULL)); //Init random number generation
    for( i = 0; i < 100; i++ )
    {
        CvPoint2D32f fp = cvPoint2D32f( (float)(rand()%(rect.width-10)+5),
                                        (float)(rand()%(rect.height-10)+5));
		printf("Locate point (%f,%f)\n",fp.x,fp.y);
		//This function is just to teach localization -- you don't have to locate to insert below.
        locate_point( subdiv, fp, img, active_facet_color );
        cvShowImage( win, img );

    //    if( cvWaitKey( 100 ) >= 0 )
    //        break;
	    c = cvWaitKey(0);
		if (c == 27 || c == 'q' || c == 'Q')
			break;
		printf("Insert point\n");
    cvSubdivDelaunay2DInsert( subdiv, fp );

cvCalcSubdivVoronoi2D( subdiv ); //Fill out Voronoi data in subdiv

        cvSet( img, bkgnd_color, 0 );
        draw_subdiv( img, subdiv, delaunay_color, voronoi_color );
        cvShowImage( win, img );

//        if( cvWaitKey( 100 ) >= 0 )
//            break;
	    c = cvWaitKey(0);
		if (c == 27 || c == 'q' || c == 'Q')
			break;
	}

    cvSet( img, bkgnd_color, 0 );
	printf("Paint Voronoi\n");
    paint_voronoi( subdiv, img );
    cvShowImage( win, img );

    cvWaitKey(0);

    cvReleaseMemStorage( &storage );
    cvReleaseImage(&img);
    cvDestroyWindow( win );
}

int main( int argc, char** argv )
{
    argc, argv;
    run();
    return 0;
}

#ifdef _EiC
main( 1, "delaunay.c" );
#endif
