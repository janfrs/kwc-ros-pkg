//UndirectedFeatureGraph *getFeatureGraph(IplImage *iplImage);

#include <boost/timer.hpp>

#include "util/ImageHistogram.hh"

#define COORD_HASH_N 200

#define HIST_DIM 5

#define HIST_BIN_THRESH 0.0

//#define MIN(X,Y) ((X) > (Y) ? (Y) : (X))
//#define MAX(X,Y) ((X) >= (Y) ? (X) : (Y))

struct pairHash : public unary_function<pair<int,int>, size_t> {
  size_t operator() (const pair<int,int> apair) const {
    return apair.first*COORD_HASH_N + apair.second;
  }
}; 
 
typedef __gnu_cxx::hash_set<pair<int,int>, pairHash> edgeHash;
//typedef set<pair<int,int> > edgeHash;

//typedef GeMatrix<FullStorage<double, ColMajor> > GEMatrix;

template <class FM>
FeatureGraphExtractor<FM>::
FeatureGraphExtractor(const IplImage *iplImage) :
  blobber(iplImage),
  nNodes(blobber.numBlobs()),
  contourImage(NULL),
  fgraph(NULL),
  nNodeFeat(H_BINS + S_BINS + V_BINS + THETA_BINS + MAG_BINS + 1)
  //  nNodeFeat(H_BINS + S_BINS + V_BINS + THETA_BINS + MAG_BINS + N_CHAN_FEAT + 1)
{
  
};

template <class FM>
FeatureGraphExtractor<FM>::
~FeatureGraphExtractor() {
  if (contourImage != NULL) 
    cvReleaseImage(&contourImage);
}

template <class FM>
UndirectedFeatureGraph<FM>* 
FeatureGraphExtractor<FM>::
getFeatureGraph() 
{
  boost::timer timer;

  if (fgraph != NULL)
    return fgraph;

  // create superpixel adjacency graph
  makeAdjacencyGraph();

  // calculate superpixel features, edge features
  //  FM* nodeFeat = getNodeFeaturesSeparateHistOpenCv(blobber);
  FM* nodeFeat = getNodeFeaturesSeparateHist(blobber);
  FM* edgeFeat = getEdgeFeatures(blobber, nodeFeat, edgeList);

  fgraph = new UndirectedFeatureGraph<FM>(nNodes, edgeList, nodeFeat, edgeFeat);

  if (getenv("oDebugOn")) {
    std::cout << "Feature graph took: " << timer.elapsed() << std::endl;
  }

  return fgraph;
}

template <class FM>
FM* FeatureGraphExtractor<FM>::
getNodeFeatures(SuperpixelBlobber &blobber) {
  int nNodeFeat = H_BINS*S_BINS*V_BINS+1;

  // make feature matrix
  FM *fmat = new FM(blobber.numBlobs(), nNodeFeat);
  
  // convert image to HSV
  const IplImage *sourceImage = blobber.getSourceImage();
  IplImage *hsv = cvCreateImage(cvGetSize(sourceImage), 8, 3);
  cvCvtColor(sourceImage, hsv, CV_BGR2HSV);
  
  // extract planes for histogram
  IplImage *hPlane = cvCreateImage(cvGetSize(sourceImage), 8, 1);
  IplImage *sPlane = cvCreateImage(cvGetSize(sourceImage), 8, 1);
  IplImage *vPlane = cvCreateImage(cvGetSize(sourceImage), 8, 1);
  //int nPlanes = 3;
  
  IplImage *planes[] = { hPlane, sPlane, vPlane };
  cvCvtPixToPlane(hsv, hPlane, sPlane, vPlane, 0);

  float h_ranges[] = { 0, 181 }; 
  float s_ranges[] = { 0, 256 };
  float v_ranges[] = { 0, 256 };
  float *ranges[] = { h_ranges, s_ranges, v_ranges };

  int numBins[] = { H_BINS, S_BINS, V_BINS };

  
  // calculate histogram for each blob, append to feature matrix
  //  SparseGeMatrix< CRS<double> > *fMatRaw = fmat->getMat();
  int blobNum = 0;
  std::vector<IplImage*>::const_iterator masksEnd = 
    blobber.getBlobMasks()->end();
  for (std::vector<IplImage*>::const_iterator it = 
	 blobber.getBlobMasks()->begin();
       it != masksEnd;
       it++) {
    IplImage* blobMask = *it;
    
    // create and calculate histogram
    CvHistogram *hist = 
      cvCreateHist(3, numBins, CV_HIST_ARRAY, (float**)ranges, 1);
        
    cvCalcHist(planes, hist, 0, blobMask);

    cvNormalizeHist(hist, 1.0);

    // FIXME: only works for 3-channel features for now 
    // lay out histogram into row of feature matrix
    int fnum = 0;
    for (int bin0 = 0; bin0 < numBins[0]; bin0++) {
      for (int bin1 = 0; bin1 < numBins[1]; bin1++) {
	for (int bin2 = 0; bin2 < numBins[2]; bin2++) {
	  double histVal = cvQueryHistValue_3D(hist, bin0, bin1, bin2);
	  if (histVal > HIST_BIN_THRESH) {
	    fmat->set(blobNum, fnum, histVal);
	  }
	  fnum++;
	}
      }
    }

    cvReleaseHist(&hist);

    /*
    cvNamedWindow("Current blob", 0);
    cvShowImage("Current blob", blobMask);

    cout << (*fMatRaw)(blobNum,_) << endl;
    cout << "Blob " << blobNum << ", area " << 
      cvCountNonZero(blobMask) << endl;
    int key = cvWaitKey();
    */

      
    blobNum++;
  }

  for (int row = 0; row < fmat->rows(); row++)
    fmat->set(row, nNodeFeat-1, 1);

  //  cout << *fMatRaw << endl;
  cvReleaseImage(&hPlane);
  cvReleaseImage(&sPlane);
  cvReleaseImage(&vPlane);
  cvReleaseImage(&hsv);

  blobber.releaseMasks();

  fmat->finalize();

  return fmat;
}

// makes a histogram per channel, concatenates them in feature vector
// FIXME: make less fragile wrt. adding new features
template <class FM>
FM* FeatureGraphExtractor<FM>::
getNodeFeaturesSeparateHistOpenCv(SuperpixelBlobber &blobber) {
  boost::timer timer;

  int nNodeFeat = H_BINS + S_BINS + V_BINS + THETA_BINS + MAG_BINS + 1;

  // make feature matrix
  FM *fmat = new FM(blobber.numBlobs(), nNodeFeat);
  
  // convert image to HSV
  const IplImage *sourceImage = blobber.getSourceImage();
  IplImage *hsv = cvCreateImage(cvGetSize(sourceImage), 8, 3);
  cvCvtColor(sourceImage, hsv, CV_BGR2HSV);
  
  // extract planes for histogram
  IplImage *hPlane = cvCreateImage(cvGetSize(sourceImage), 8, 1);
  IplImage *sPlane = cvCreateImage(cvGetSize(sourceImage), 8, 1);
  IplImage *vPlane = cvCreateImage(cvGetSize(sourceImage), 8, 1);
  //int nPlanes = 3;
  
  //  IplImage *planes[] = { hPlane, sPlane, vPlane };
  cvCvtPixToPlane(hsv, hPlane, sPlane, vPlane, 0);

  // FIXME: use other channels for gradient?
  // calculate image gradient features
  IplImage *thPlane = 
    cvCreateImage(cvGetSize(sourceImage), IPL_DEPTH_8U, 1);
  IplImage *mgPlane = 
    cvCreateImage(cvGetSize(sourceImage), IPL_DEPTH_8U, 1);
  calcImGradientFeatures(vPlane, thPlane, mgPlane);

  IplImage *planes[] = { hPlane, sPlane, vPlane, thPlane, mgPlane };

  // create and calculate histogram
  //  static int numBins[] = { H_BINS, S_BINS, V_BINS };
  static int numBins[] = { H_BINS, S_BINS, V_BINS, THETA_BINS, MAG_BINS };
  static float h_ranges[] = { 0, 181 }; 
  static float s_ranges[] = { 0, 256 };
  static float v_ranges[] = { 0, 256 };
  static float th_ranges[] = { 0, 256 };
  //  static float mg_ranges[] = { 0, log2f(IMAGE_MAX_VAL + 1)+1 };
  static float mg_ranges[] = { 0, 256 };
  //  static float *ranges[] = { h_ranges, s_ranges, v_ranges };
  static float *ranges[] = 
    { h_ranges, s_ranges, v_ranges, th_ranges, mg_ranges };

  


  /*
  // TESTING
  double imin = 0, imax = 0;
  cvMinMaxLoc(hPlane, &imin, &imax);
  assert(imax < h_ranges[1]);
  std::cout << "H RANGE " << imin << " " << imax << std::endl;
  CvScalar iavg = cvAvg(hPlane);
  std::cout << "H AVG " << iavg.val[0] << std::endl;
  */



  // calculate histogram for each blob, append to feature matrix

  int blobNum = 0;
  std::vector<IplImage*>::const_iterator masksEnd = 
    blobber.getBlobMasks()->end();
  for (std::vector<IplImage*>::const_iterator it = 
	 blobber.getBlobMasks()->begin();
       it != masksEnd;
       it++) {
    IplImage* blobMask = *it;

    int featureNum = 0;
    for (int ci = 0; ci < HIST_DIM; ci++) {
      
      CvHistogram *hist = 
	cvCreateHist(1, &numBins[ci], CV_HIST_ARRAY, &(ranges[ci]), 1);
      
      cvCalcHist(&planes[ci], hist, 0, blobMask);
      
      cvNormalizeHist(hist, 1.0);

      
      // DEBUGGING!!!
      // DEBUGGING!!!
      //      if (ci < 3) continue;



      // lay out histogram into row of feature matrix
      for (int bin = 0; bin < numBins[ci]; bin++) {
	double histVal = cvQueryHistValue_1D(hist, bin);

	//	std::cerr << histVal << std::endl;

	if (histVal > HIST_BIN_THRESH) {
	  fmat->set(blobNum, featureNum, histVal);
	}

	featureNum++;
      }

      cvReleaseHist(&hist);
    }
    blobNum++;
  }

  for (int row = 0; row < fmat->rows(); row++) 
    fmat->set(row, nNodeFeat-1, 1);

  //  cout << *fMatRaw << endl;
  cvReleaseImage(&hPlane);
  cvReleaseImage(&sPlane);
  cvReleaseImage(&vPlane);
  cvReleaseImage(&hsv);
  cvReleaseImage(&thPlane);
  cvReleaseImage(&mgPlane);

  blobber.releaseMasks();

  fmat->finalize();

  if (getenv("oDebugOn"))
    std::cout << "Node features: " << timer.elapsed() << std::endl;

  //  cout << "NNZ " << fmat->nnz() << " SIZE " << fmat->rows() * fmat->cols() << endl;

  return fmat;
}

template <class FM>
FM* FeatureGraphExtractor<FM>::
getNodeFeaturesSeparateHist(SuperpixelBlobber &blobber) {
  boost::timer timer;

  // make feature matrix
  FM *fmat = new FM(blobber.numBlobs(), nNodeFeat);
  
  // convert image to HSV
  const IplImage *sourceImage = blobber.getSourceImage();
  IplImage *hsv = cvCreateImage(cvGetSize(sourceImage), 8, 3);
  cvCvtColor(sourceImage, hsv, CV_BGR2HSV);
  
  // extract planes for histogram
  IplImage *hPlane = cvCreateImage(cvGetSize(sourceImage), 8, 1);
  IplImage *sPlane = cvCreateImage(cvGetSize(sourceImage), 8, 1);
  IplImage *vPlane = cvCreateImage(cvGetSize(sourceImage), 8, 1);
  //int nPlanes = 3;
  
  //  IplImage *planes[] = { hPlane, sPlane, vPlane };
  cvCvtPixToPlane(hsv, hPlane, sPlane, vPlane, 0);

  // FIXME: use other channels for gradient?
  // calculate image gradient features
  IplImage *thPlane = 
    cvCreateImage(cvGetSize(sourceImage), IPL_DEPTH_8U, 1);
  IplImage *mgPlane = 
    cvCreateImage(cvGetSize(sourceImage), IPL_DEPTH_8U, 1);
  calcImGradientFeatures(vPlane, thPlane, mgPlane);

  IplImage *planes[] = { hPlane, sPlane, vPlane, thPlane, mgPlane };

  // create and calculate histogram
  //  static int numBins[] = { H_BINS, S_BINS, V_BINS };
  static int numBins[] = { H_BINS, S_BINS, V_BINS, THETA_BINS, MAG_BINS };
  static double h_ranges[] = { 0, 181 }; 
  static double s_ranges[] = { 0, 256 };
  static double v_ranges[] = { 0, 256 };
  static double th_ranges[] = { 0, 256 };
  static double mg_ranges[] = { 0, 256 };
  //  static double *ranges[] = { h_ranges, s_ranges, v_ranges };
  static double *ranges[] = 
    { h_ranges, s_ranges, v_ranges, th_ranges, mg_ranges };

  // calculate histogram for each blob, append to feature matrix

  int blobNum = 0;
  std::vector<coordList*>::const_iterator itEnd = 
    blobber.getBlobCoords()->end();
  for (std::vector<coordList*>::const_iterator it = 
	 blobber.getBlobCoords()->begin();
       it != itEnd;
       it++) {
    coordList* coords = *it;

    int featureNum = 0;
    for (int ci = 0; ci < HIST_DIM; ci++) {
      double bins[numBins[ci]];

      ImageHistogram<unsigned char>::
	histogram(planes[ci], 
		  pair<double,double>((ranges[ci])[0], (ranges[ci])[1]),
		  *coords, 
		  numBins[ci],
		  bins);

      // DEBUGGING!!!
      // DEBUGGING!!!
      //      if (ci < 3) continue;
      //      if (ci >= 3) continue;

      // lay out histogram into row of feature matrix
      for (int bin = 0; bin < numBins[ci]; bin++) {
	if (bins[bin] > HIST_BIN_THRESH) {
	  fmat->set(blobNum, featureNum, bins[bin]);
	}

	featureNum++;
      }

    }
    blobNum++;
  }

  /*
  // calculate whole-channel features
  CvScalar hMean = cvAvg(hPlane);
  CvScalar sMean = cvAvg(sPlane);
  CvScalar vMean = cvAvg(vPlane);
  
  for (int row = 0; row < fmat->rows(); row++) {
    fmat->set(row, nNodeFeat-2, vMean.val[0] / 255);
    fmat->set(row, nNodeFeat-3, sMean.val[0] / 255);
    fmat->set(row, nNodeFeat-4, hMean.val[0] / 255);
  }
  */

  for (int row = 0; row < fmat->rows(); row++) 
    fmat->set(row, nNodeFeat-1, 1);

  //  cout << *fMatRaw << endl;
  cvReleaseImage(&hPlane);
  cvReleaseImage(&sPlane);
  cvReleaseImage(&vPlane);
  cvReleaseImage(&hsv);
  cvReleaseImage(&thPlane);
  cvReleaseImage(&mgPlane);

  blobber.releaseMasks();

  fmat->finalize();

  if (getenv("oDebugOn"))
    std::cout << "Node features: " << timer.elapsed() << std::endl;

  //  cout << "NNZ " << fmat->nnz() << " SIZE " << fmat->rows() * fmat->cols() << endl;

  return fmat;
}

// output argument IplImages should be preallocated
// maximum magnitude is log of maximum derivative + 1, min is 0
template <class FM> 
void FeatureGraphExtractor<FM>::
calcImGradientFeatures(const IplImage *src, 
		       IplImage *thetas,
		       IplImage *mags) {
  IplImage *dx = cvCreateImage(cvGetSize(src), IPL_DEPTH_16S, 1);
  IplImage *dy = cvCreateImage(cvGetSize(src), IPL_DEPTH_16S, 1);

  assert(dx != NULL);
  assert(dy != NULL);

  cvSobel(src, dx, 1, 0, 3);
  cvSobel(src, dy, 0, 1, 3);

  /*
  cvNamedWindow("original");
  cvShowImage("original", src);
  cvNamedWindow("gx", 0);
  cvNamedWindow("gy", 0);
  cvShowImage("gx", dx);
  cvShowImage("gy", dy);
  */

  assert(thetas->depth == IPL_DEPTH_8U);
  assert(mags->depth == IPL_DEPTH_8U);

  // FIXME: very fragile assumption, depends on kernel size, etc.
  double logMaxval = log2f(1024*sqrt(2) + 1);

  //  double minval = HUGE_VAL;
  //  double maxval = -HUGE_VAL;

  for (int yy = 0; yy < src->height; yy++) {
    for (int xx = 0; xx < src->width; xx++) {
      short idx = ((short*)(dx->imageData+yy*dx->widthStep))[xx];
      short idy = ((short*)(dy->imageData+yy*dy->widthStep))[xx];

      //      minval = MIN(minval, idx);
      //      minval = MIN(minval, idy);
      //      maxval = MAX(maxval, idx);
      //      maxval = MAX(maxval, idy);

      double pctLogMag = log2f(sqrtf(idx*idx + idy*idy)+1) / logMaxval;
      assert(pctLogMag <= 1);
      unsigned char theta = 255 * (atan2f(idy, idx) + M_PI) / (2*M_PI),
	mag = 255 * pctLogMag;
      ((uchar*)(thetas->imageData + yy*thetas->widthStep))[xx] = theta;
      ((uchar*)(mags->imageData + yy*mags->widthStep))[xx] = mag;
    }
  }

  //  std::cout << "MINVAL " << minval << std::endl;
  //  std::cout << "MAXVAL " << maxval << std::endl;

  //  sleep(1);

  cvReleaseImage(&dx);
  cvReleaseImage(&dy);
}

/*
// Returns features that guarantee separability (feature per node)
DenseFeatureMatrix<double>* FeatureGraphExtractor<FM>::
getNodeFeaturesTesting(const SuperpixelBlobber &blobber) {
  cout << "TESTING CODE! DISABLE ME! " << endl;

  // make feature matrix
  DenseFeatureMatrix<double> *fmat = 
    new DenseFeatureMatrix<double>(nNodes, nNodes);

  GeMatrix<FullStorage<double, ColMajor> > *fMatRaw = fmat->getMat();

  for (int ii = 0; ii < nNodes; ii++) {
    (*fMatRaw)(ii,ii) = 1;  
  }

  return fmat;
}
*/

template <class FM>
FM *FeatureGraphExtractor<FM>::
getEdgeFeatures(const SuperpixelBlobber &blobber, 
		FM* snodeFeat,
		vector<pair<int,int> > &edgeList) {

  boost::timer timer;

  /*
  cout << "EDGE FEATURE TESTING " << endl;

  //  int nEdgeFeat = 1;

  // make feature matrix
  DenseFeatureMatrix<double> *edgeFeat = 
    new DenseFeatureMatrix<double>(edgeList.size(), 1);

  GEMatrix *rawEdgeMat = edgeFeat->getMat();

  (*rawEdgeMat) = 1;

  return edgeFeat;
  */

  // FIXME: don't hard-code this
  //  int nNodeFeat = H_BINS + S_BINS + V_BINS + THETA_BINS + MAG_BINS + 1;
  int nEdgeFeat = 2*nNodeFeat + 1;

  // make feature matrix
  FM *edgeFeat = new FM(edgeList.size(), nEdgeFeat);

  // make a dense copy to allow random access
  // FIXME: unnecessary if snodeFeat is already dense
  DenseFeatureMatrix<double> nodeFeat(*snodeFeat);

  int edgeNum = 0;
  //  SparseGeMatrix< CRS<double> > *rawEdgeMat = edgeFeat->getMat();
  for (vector<pair<int,int> >::iterator it = edgeList.begin();
       it != edgeList.end();
       it++) {
    pair<int,int> edge = *it;

    // splice together the two feature vectors from each node
    //    SparseGeMatrix< CRS<double> > *rawNodeMat = nodeFeat->getMat();
    for (int srcCol = 0; srcCol < nNodeFeat; srcCol++) {
      edgeFeat->set(edgeNum, srcCol, 
		    nodeFeat.get(edge.first, srcCol));
      edgeFeat->set(edgeNum, srcCol + nNodeFeat, 
		    nodeFeat.get(edge.second, srcCol));
    }


    //    SparseGeMatrix< CRS<double> >::VectorView n0Feat = (*rawNodeMat)(edge.first,_);
    //    SparseGeMatrix< CRS<double> >::VectorView n1Feat = (*rawNodeMat)(edge.second,_);
    
    //    (*rawEdgeMat)(edgeNum,_(0,nNodeFeat-1)) = n0Feat;
    //    (*rawEdgeMat)(edgeNum,_(nNodeFeat,2*nNodeFeat-1)) = n1Feat;

    //    cout << "Edge " << edgeNum << (*rawEdgeMat)(edgeNum,_) << endl;

    edgeNum++;
  }

  for (int row = 0; row < edgeFeat->rows(); row++)
    edgeFeat->set(row, 2*nNodeFeat, 1); 
  
  edgeFeat->finalize();

  if (getenv("oDebugOn")) 
    std::cout << "Edge features took " << timer.elapsed() << std::endl;

  return edgeFeat;
}

template <class FM>
const IplImage* FeatureGraphExtractor<FM>::
getContourImage() {
  if (contourImage == NULL)
    makeAdjacencyGraph();
  return contourImage;
}

template <class FM>
void FeatureGraphExtractor<FM>::
makeAdjacencyGraph() {
  contourImage = 
    cvCreateImage(cvGetSize(blobber.getSourceImage()), 8, 1);
  cvSetZero(contourImage);

  const IplImage *labeled = blobber.getLabeledImage();

  edgeHash edges;

  // connect blobs that are 8-neighbors
  const int nNeighbor = 3;
  const int iInc[] = { 1, 0, 1 };
  const int jInc[] = { 0, 1, 1 };

  cout << "Image " << labeled->width << "x" << labeled->height << endl;

  // FIXME: fragile code--this depends on labeled image being 
  // of depth IPL_DEPTH_32S
  assert(labeled->depth == (int)IPL_DEPTH_32S);
  
  for (int ii = 0; ii < labeled->width - 1; ii++) {
    for (int jj = 0; jj < labeled->height - 1; jj++) {
      int id0 = ((int*)(labeled->imageData + 
			labeled->widthStep*jj))[ii];

      //      cout << "Processing " << ii << ", " << jj << endl;
      
      // if any neighbor has different label, add edge to edge set
      for (int ni = 0; ni < nNeighbor; ni++) {
	int nii = ii + iInc[ni];
	int njj = jj + jInc[ni];

	int id1 = ((int*)(labeled->imageData + 
			  labeled->widthStep*njj))[nii];

	if (id0 != id1) {
	  pair<int,int> edge(MIN(id0,id1), MAX(id0,id1));
	  edges.insert(edge);	// FIXME: does this mask earlier bindings?

	  //	  cout << "Edge " << id0 << " " << id1 << endl;
  //	  cout << "Neighbor " << nii << ", " << njj << endl;

	  ((uchar*)(contourImage->imageData + 
		    contourImage->widthStep*jj))[ii] = 255;
	  /*
	  ((uchar*)(contourImage->imageData + 
		    contourImage->widthStep*njj))[nii] = 255;
	  */
	}
      }
    }
  }

  blobber.releaseLabeledImage();

  //  cvSaveImage("contours.ppm", contourImage);		// FIXME: take this out

  edgeHash::iterator itEnd = edges.end();
  for (edgeHash::iterator it = edges.begin();
       it != itEnd;
       it++) {
    pair<int,int> edge = *it;
    edgeList.push_back(edge);
  }

  if (getenv("oShowFGraph"))
    displayGraph();
}

/*
void FeatureGraphExtractor<FM>::
viewGraph() {
  //  assert(contourImage != NULL);
  if (contourImage == NULL)
    makeAdjacencyGraph();

  cvNamedWindow("contours",0);
  cvShowImage("contours", contourImage);

  IplImage *imdisp = 
    cvCreateImage(cvGetSize(contourImage), IPL_DEPTH_8U, 3);
  cvCopy(blobber.getSourceImage(), imdisp);  

  IplImage *temp = 
    cvCreateImage(cvGetSize(contourImage), IPL_DEPTH_8U, 3);
  cvMerge(contourImage, contourImage, contourImage, NULL, temp);    

  cvAdd(temp, imdisp, imdisp);

  cvNamedWindow("graph",0);
  int bnum = 0;
  vector<blobStat*> *blobStats = blobber.getBlobStats();

  cout << "Total " << blobStats->size() << " blobs" << endl;

  for (vector<blobStat*>::iterator it = blobStats->begin();
       it != blobStats->end();
       it++) {
    blobStat* bstat = *it;
    cvCircle(imdisp, cvPoint(bstat->mx, bstat->my), 10, 
	     CV_RGB(255,0,0), 2);
    bnum++;
  }

  for (vector<pair<int,int> >::iterator it = edgeList.begin();
       it != edgeList.end();
       it++) {
    pair<int,int> edge = *it;
    CvPoint p0 = cvPoint((*blobStats)[edge.first]->mx, 
			 (*blobStats)[edge.first]->my);
    CvPoint p1 = cvPoint((*blobStats)[edge.second]->mx, 
			 (*blobStats)[edge.second]->my);
    cvLine(imdisp, p0, p1, CV_RGB(0,255,0), 1);
  }

  cvShowImage("graph", imdisp);

  cvReleaseImage(&imdisp);
  cvReleaseImage(&temp);
}
*/

template <class FM>
void FeatureGraphExtractor<FM>::
getBlobWeights(vector<double> &weights) {
  const Blobber *blobber = getBlobber();
  const vector<coordList*> *coords = blobber->getBlobCoords();

  for (vector<coordList*>::const_iterator it = coords->begin();
       it != coords->end();
       it++) {
    weights.push_back((double)(*it)->coords.size());


    //    weights.push_back((double)1);
  }
}

template <class FM>
void FeatureGraphExtractor<FM>::
testBlobWeights() {
  const vector<IplImage*> *masks = getBlobber()->getBlobMasks();

  vector<double> weights;
  getBlobWeights(weights);

  cvNamedWindow("mask");
  int bnum = 0;
  for (vector<IplImage*>::const_iterator it = masks->begin();
       it != masks->end();
       it++) {
    IplImage *mask = *it;
    cvShowImage("mask", mask);
    cout << "Blob: " << weights[bnum] << " pix" << endl;
    bnum++;
    cvWaitKey();
  }

  getBlobber()->releaseMasks();
}

template <class FM>
void FeatureGraphExtractor<FM>::
displayGraph(const Int2IntMap* blob2NodeInds) {
  displayGraph(blobber.getSourceImage(), 
	       getContourImage(),
	       *blobber.getBlobStats(),
	       edgeList,
	       blob2NodeInds);
}

template <class FM>
void FeatureGraphExtractor<FM>::
displayGraph(const vector<pair<int,int> >& edgeList,
	     const Int2IntMap* blob2NodeInds) {
  displayGraph(blobber.getSourceImage(), 
	       getContourImage(),
	       *blobber.getBlobStats(),
	       edgeList,
	       blob2NodeInds);
}

// displays the given graph 
// blob2NodeInds maps indices of blobs in image to node indices
template <class FM>
void FeatureGraphExtractor<FM>::
displayGraph(const IplImage* sourceImage,
	     const IplImage* contourImage, 
	     const vector<blobStat*>& blobStats,
	     const vector<pair<int,int> >& edgeList,
	     const Int2IntMap* blob2NodeInds) {
  
  cvNamedWindow("contours",0);
  cvShowImage("contours", contourImage);
  
  IplImage *imdisp = 
    cvCreateImage(cvGetSize(contourImage), IPL_DEPTH_8U, 3);
  cvCopy(sourceImage, imdisp);  

  IplImage *temp = 
    cvCreateImage(cvGetSize(contourImage), IPL_DEPTH_8U, 3);
  cvMerge(contourImage, contourImage, contourImage, NULL, temp);    

  cvAdd(temp, imdisp, imdisp);

  cvNamedWindow("graph",0);
  int bnum = 0;

  cout << "Total " << blobStats.size() << " blobs" << endl;

  for (int ii = 0; ii < (int)blobStats.size(); ii++) {
    if (blob2NodeInds != NULL && 
	blob2NodeInds->find(ii) == blob2NodeInds->end()) 
      continue;

    blobStat* bstat = blobStats[ii];
    cvCircle(imdisp, cvPoint(bstat->mx, bstat->my), 10, 
	     CV_RGB(255,0,0), 2);
    bnum++;
  }

  // invert mapping
  Int2IntMap* node2BlobInds = NULL;
  if (blob2NodeInds != NULL) {
    node2BlobInds = new Int2IntMap();
    for (Int2IntMap::const_iterator it = blob2NodeInds->begin();
	 it != blob2NodeInds->end();
	 it++) {
      pair<int,int> nodeBlob = *it;
      (*node2BlobInds)[nodeBlob.second] = nodeBlob.first;
    }
  }

  for (vector<pair<int,int> >::const_iterator it = edgeList.begin();
       it != edgeList.end();
       it++) {
    pair<int,int> edge = *it;

    int blob0 = -1, blob1 = -1;

    if (node2BlobInds != NULL) {
      blob0 = (*node2BlobInds)[edge.first];
      blob1 = (*node2BlobInds)[edge.second];
    } else {
      blob0 = edge.first;
      blob1 = edge.second;
    }

    assert(blob0 <= (int)blobStats.size());
    assert(blob1 <= (int)blobStats.size());

    CvPoint p0 = cvPoint(blobStats[blob0]->mx, 
			 blobStats[blob0]->my);
    CvPoint p1 = cvPoint(blobStats[blob1]->mx, 
			 blobStats[blob1]->my);
    cvLine(imdisp, p0, p1, CV_RGB(0,255,0), 1);
  }

  if (node2BlobInds != NULL)
    delete node2BlobInds;

  cvShowImage("graph", imdisp);

  cvReleaseImage(&imdisp);
  cvReleaseImage(&temp);
}

