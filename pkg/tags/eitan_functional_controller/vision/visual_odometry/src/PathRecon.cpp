/*
 * CvPathRecon.cpp
 *
 *  Created on: Sep 2, 2008
 *      Author: jdchen
 */

#include <iostream>
#include <utility>
using namespace std;

// opencv
#include <opencv/cxcore.h>
#include <opencv/cv.h>
#include <opencv/highgui.h>

// boost
#include <boost/foreach.hpp>

#include "PathRecon.h"
using namespace cv::willow;

#include "CvMatUtils.h"
#include "PoseEstimate.h"

// timing
#include "CvTestTimer.h"

#define DISPLAY 0

#ifndef DEBUG
#define DEBUG  // to print debug message in release build
#endif
#undef DEBUG

// Please note that because the timing code is executed is called lots of lots of times
// they themselves have taken substantial timing as well
#define CHECKTIMING 1

#define SAVEDISPMAP 0

#if CHECKTIMING == 0
#define TIMERSTART(x)
#define TIMEREND(x)
#define TIMERSTART2(x)
#define TIMEREND2(x)
#else
#define TIMERSTART(x)  CvTestTimerStart(x)
#define TIMEREND(x)    CvTestTimerEnd(x)
#define TIMERSTART2(x) CvTestTimerStart2(x)
#define TIMEREND2(x)   CvTestTimerEnd2(x)
#endif

PathRecon::Stat::Stat(){}

PathRecon::PathRecon(const CvSize& imageSize):
	mPoseEstimator(imageSize.width, imageSize.height),
	mTransform(cvMat(4, 4, CV_64FC1, _transform)),
	mPathLength(0.),
	mVisualizer(NULL),
  mOutputDir(string("Output/indoor1/")),
  mRT(cvMat(4,4, CV_64FC1, _rt)),
  _mTempMat(cvMat(4,4,CV_64FC1, _tempMat))
{
	_init();
}

PathRecon::~PathRecon() {
	// TODO Auto-generated destructor stub
  delete mVisualizer;
}

void PathRecon::_init() {
	mMinNumInliersForGoodFrame  = defMinNumInliersForGoodFrame;
	mMinNumInliers  = defMinNumInliers;
	mMinInlierRatio = defMinInlierRatio;
	mMaxAngleAlpha  = defMaxAngleAlpha;
	mMaxAngleBeta   = defMaxAngleBeta;
	mMaxAngleGamma  = defMaxAngleGamma;
	mMaxShift       = defMaxShift;

	cvSetIdentity(&mTransform);

	// seting up the pose estimator to use harris corner for finding feature
	// points and keypoint ncc to find matching pairs
  mPoseEstimator.setInlierErrorThreshold(4.0);
//  mPoseEstimator.setKeyPointDector(PoseEstimateStereo::HarrisCorner);
//  mPoseEstimator.setKeyPointMatcher(KeyPointCrossCorrelation);
  mPoseEstimator.setKeyPointDector(PoseEstimateStereo::HarrisCorner);
  mPoseEstimator.setKeyPointMatcher(KeyPointSumOfAbsDiff);
  mMinNumInliers = 25;
  // end setting up for harris corner and keypoint ncc matcher

  delete mVisualizer;
#if DISPLAY
	mVisualizer = new Visualizer(mPoseEstimator);
#endif

  int maxDisp = (int)(mPoseEstimator.getD(400));// the closest point we care is at least 1000 mm away
  cout << "Max disparity is: " << maxDisp << endl;
  mStat.mErrMeas.setCameraParams((const CvStereoCamParams& )(mPoseEstimator));

  mFrameSeq.reset();
}

KeyFramingDecision
PathRecon::keyFrameEval(
		int frameIndex,
		int numKeypoints,
		int numInliers,
		const CvMat& rot, const CvMat& shift
) {
	KeyFramingDecision kfd = KeyFrameKeep;
	bool keyFrameNeeded = false;

	//
	// check if the frame is good enough for checking
	//
	if (numInliers < defMinNumInliersForGoodFrame) {
		// Too few inliers to ensure good tracking
		keyFrameNeeded = true;
	} else if (cvNorm(&shift) > mMaxShift) {
		// the shift is large enough that a key frame is needed
		keyFrameNeeded = true;
	} else if (numInliers < mMinNumInliers) {
		keyFrameNeeded = true;
	} else if ((double)numInliers/(double)numKeypoints < mMinInlierRatio) {
		keyFrameNeeded = true;
	} else {
		CvPoint3D64f eulerAngles;
		CvMatUtils::eulerAngle(rot, eulerAngles);
		if (fabs(eulerAngles.x) > mMaxAngleAlpha ||
			fabs(eulerAngles.y) > mMaxAngleBeta  ||
			fabs(eulerAngles.z) > mMaxAngleGamma ){
			// at least one of the angle is large enough that a key frame is needed
			keyFrameNeeded = true;
		}
	}

	if (keyFrameNeeded == true) {
		if (mFrameSeq.mLastGoodFrame == NULL) {
			// nothing to back track to. What can I do except use it :(
			kfd = KeyFrameUse;
		} else {
			kfd = KeyFrameBackTrack;
		}
	} else {
		kfd = KeyFrameKeep;
	}

	return kfd;
}

/**
 * Keep track of the trajectory
 * store the current transformation from starting point to current position
 * in transform as transform = transform * rt
 */
bool PathRecon::appendTransform(const CvMat& rot, const CvMat& shift){
	PoseEstimate::constructTransform(rot, shift, mRT);
	cvCopy(&mTransform, &_mTempMat);
	cvMatMul(&_mTempMat, &mRT, &mTransform);
	return true;
}

/**
 * Store the current key frame to key frame transformation with w.r.t to starting frame
 */
bool PathRecon::storeTransform(const CvMat& rot, const CvMat& shift, int frameIndex){
	CvMat rotGlobal;
	CvMat shiftGlobal;
	cvGetSubRect(&mTransform, &rotGlobal,   cvRect(0, 0, 3, 3));
	cvGetSubRect(&mTransform, &shiftGlobal, cvRect(3, 0, 1, 3));
	FramePose* fp = new FramePose(frameIndex);
	CvMat rodGlobal2   = cvMat(1,3, CV_64FC1, &(fp->mRod));
	CvMat shiftGlobal2 = cvMat(1,3, CV_64FC1, &(fp->mShift));
	// make a copy
	cvRodrigues2(&rotGlobal, &rodGlobal2);
	cvTranspose(&shiftGlobal,  &shiftGlobal2);

	mFramePoses.push_back(*fp);

	// update the path length as well
	mPathLength += cvNorm((const CvMat *)&shift);
	return true;
}

void PathRecon::dispToGlobal(const CvMat& uvds, CvMat& xyzs){
  dispToGlobal(uvds, mTransform, xyzs);
}
void PathRecon::dispToGlobal(const CvMat& uvds, const CvMat& transform, CvMat& xyzs) {
  double _xyz[3*uvds.rows];
  CvMat localxyz = cvMat(uvds.rows, 3, CV_64FC1, _xyz);
  // Convert from disparity coordinates to Cartesian coordinates
  mPoseEstimator.reprojection(&uvds, &localxyz);
  double _inliers1t[3*uvds.rows];
  CvMat inliers1t = cvMat(uvds.rows, 1, CV_64FC3, _inliers1t);
  CvMat localxyzC3;
  CvMat xyzsC3;
  CvMat transform3x4;
  // Transform from local Cartesian coordinates to Global Cartesian.
  cvReshape(&localxyz, &localxyzC3, 3, 0);
  cvReshape(&xyzs, &xyzsC3, 3, 0);
  cvGetRows(&transform, &transform3x4, 0, 3);
  cvTransform(&localxyzC3, &xyzsC3, &transform3x4);
}

bool PathRecon::saveKeyPoints(const CvMat& keypoints, const string& filename){
	bool status = true;

	double _xyzs[3*keypoints.rows];
	CvMat xyzs = cvMat(keypoints.rows, 1, CV_64FC3, _xyzs);
	dispToGlobal(keypoints, xyzs);
	cvSave(filename.c_str(), &xyzs,
	    "inliers1", "inliers of current image in start frame");

	return status;
}

void PathRecon::measureErr(const CvMat* inliers0, const CvMat* inliers1){
  assert(mFrameSeq.mCurrentFrame);
	mStat.mErrMeas.setTransform(mFrameSeq.mCurrentFrame->mRot,
	    mFrameSeq.mCurrentFrame->mShift);

	// rot and shift transform inliers0 to inliers1
	mStat.mErrMeas.measure(*inliers0, *inliers1);
}

bool PathRecon::goodFeaturesToTrack(
    const WImage1_b& leftImage,
    const WImage1_16s* dispMap,
    Keypoints*& keypoints
) {
  bool status = true;
  if (keypoints == NULL)
    keypoints = new Keypoints();
  else
    keypoints->clear();
  status = mPoseEstimator.goodFeaturesToTrack(leftImage, dispMap, *keypoints);
  mStat.mHistoKeypoints.push_back(keypoints->size());
#ifdef DEBUG
  cout << "Found " << keypoints->size() << " good features in left  image" << endl;
  for (size_t i = 0; i < keypoints->size(); i++) {
    printf("(%f,%f,%f)\n", (*keypoints)[i].x, (*keypoints)[i].y, (*keypoints)[i].z);
  }
#endif
  return true;
}

void PathRecon::updateTrajectory() {
  if (mFrameSeq.mNumFrames>1) {
    // not the first frame
    // keep track of the trajectory
    appendTransform(mFrameSeq.mCurrentFrame->mRot, mFrameSeq.mCurrentFrame->mShift);
    int frameIndex = mFrameSeq.mCurrentFrame->mFrameIndex;
    mStat.mHistoKeyFrameInliers.push_back(make_pair(frameIndex, mFrameSeq.mCurrentFrame->mNumInliers));
    mStat.mHistoKeyFrameKeypoints.push_back(make_pair(frameIndex, mFrameSeq.mCurrentFrame->mKeypoints->size()));
    mStat.mHistoKeyFrameTrackablePairs.push_back(make_pair(frameIndex, mFrameSeq.mCurrentFrame->mNumTrackablePairs));

#if 0
    // save the inliers into a file
    char inliersFilename[256];
    sprintf(inliersFilename, "%s/inliers1_%04d.xml", mOutputDir.c_str(), mFrameSeq.mCurrentFrame->mFrameIndex);
    saveKeyPoints(*mFrameSeq.mCurrentFrame->mInliers1, string(inliersFilename));
#endif

    // stores rotation mat and shift vector in rods and shifts
    storeTransform(mFrameSeq.mCurrentFrame->mRot, mFrameSeq.mCurrentFrame->mShift,
        mFrameSeq.mCurrentFrame->mFrameIndex - mFrameSeq.mStartFrameIndex);

#ifdef DEBUG
    // measure the errors
    measureErr(mFrameSeq.mCurrentFrame->mInliers1, mFrameSeq.mCurrentFrame->mInliers0);
#endif
  }
  // getting ready for next iteration
  setLastKeyFrame(mFrameSeq.mCurrentFrame);
  mFrameSeq.mCurrentFrame = NULL;
}

void PathRecon::matchKeypoints(
    vector<pair<CvPoint3D64f, CvPoint3D64f> >* trackablePairs,
    vector<pair<int, int> >* trackableIndexPairs
) {
  TIMERSTART2(TrackablePair);
  PoseEstFrameEntry* lastKeyFrame = getLastKeyFrame();
  assert(lastKeyFrame != NULL);
  mPoseEstimator.getTrackablePairs(
      *mFrameSeq.mCurrentFrame->mImage,*lastKeyFrame->mImage,
      *mFrameSeq.mCurrentFrame->mDispMap,*lastKeyFrame->mDispMap,
      *mFrameSeq.mCurrentFrame->mKeypoints,*lastKeyFrame->mKeypoints,
      trackablePairs, trackableIndexPairs);
  // record the number of trackable pairs. At lease one of
  // trackablePairs of trackableIndexPairs is not NULL, if
  // there are trackable pairs of interest.
  if (trackablePairs) {
    mStat.mHistoTrackablePairs.push_back(trackablePairs->size());
    mFrameSeq.mCurrentFrame->mNumTrackablePairs = trackablePairs->size();
  } else if (trackableIndexPairs) {
    mStat.mHistoTrackablePairs.push_back(trackableIndexPairs->size());
    mFrameSeq.mCurrentFrame->mNumTrackablePairs = trackableIndexPairs->size();
  }
  TIMEREND2(TrackablePair);
}

/// reconstruction w.r.t one additional frame
bool PathRecon::trackOneFrame(queue<StereoFrame>& inputImageQueue, FrameSeq& frameSeq) {
  bool insertNewKeyFrame = false;
  TIMERSTART(Total);

  // currFrame is a reference to frameSeq.mCurrentFrame;
  PoseEstFrameEntry*& currFrame = frameSeq.mCurrentFrame;
  bool stop = false;

  if (frameSeq.mNextFrame.get()){
    // do not need to load new images nor compute the key points again
    frameSeq.mCurrentFrame = frameSeq.mNextFrame.release();
  } else {
    // process the next stereo pair of images.
    StereoFrame stereoFrame = inputImageQueue.front();
    if (stereoFrame.mFrameIndex == -1) {
      // done
      stop = true;
    } else {
      currFrame = new PoseEstFrameEntry(stereoFrame.mFrameIndex);
      currFrame->mImage = stereoFrame.mImage;
      currFrame->mRightImage = stereoFrame.mRightImage;
      currFrame->mDispMap = stereoFrame.mDispMap;

      // counting how many frames we have processed
      frameSeq.mNumFrames++;

      TIMERSTART2(FeaturePoint);
      goodFeaturesToTrack(*currFrame->mImage, currFrame->mDispMap,
          currFrame->mKeypoints);
      TIMEREND2(FeaturePoint);

      // prepare the keypoint descriptors
      TIMERSTART2(KeyPointDescriptor);
      mPoseEstimator.constructKeypointDescriptors(*currFrame->mImage, *currFrame->mKeypoints);
      TIMEREND2(KeyPointDescriptor);
    }
    inputImageQueue.pop();
  }

  KeyFramingDecision kfd = KeyFrameBackTrack;

  if (stop == false) {
    if (frameSeq.mNumFrames==1) {
      // First frame ever, do not need to do anything more.
      frameSeq.mStartFrameIndex = currFrame->mFrameIndex;
#if 0 // TODO do it in action
      setLastKeyFrame(currFrame);
      insertNewKeyFrame = true;
      frameSeq.mCurrentFrame = NULL;
#endif
      kfd = KeyFrameUse;
    } else {
      //
      // match the good feature points between this iteration and last key frame
      //
      vector<pair<CvPoint3D64f, CvPoint3D64f> > trackablePairs;
      if (currFrame->mTrackableIndexPairs == NULL) {
        currFrame->mTrackableIndexPairs = new vector<pair<int, int> >();
      } else {
        currFrame->mTrackableIndexPairs->clear();
      }
      matchKeypoints(&trackablePairs, currFrame->mTrackableIndexPairs);

      assert(currFrame->mTrackableIndexPairs->size() == trackablePairs.size());
#ifdef DEBUG
      cout << "Num of trackable pairs for pose estimate: "<<trackablePairs.size() << endl;
      for (size_t i = 0; i < trackablePairs.size(); i++) {
        printf("(%f,%f,%f), (%f,%f,%f)\n",
          trackablePairs[i].first.x,
          trackablePairs[i].first.y,
          trackablePairs[i].first.z,
          trackablePairs[i].second.x,
          trackablePairs[i].second.y,
          trackablePairs[i].second.z);
      }
#endif

      // if applicable, pass a reference of the current frame for visualization
      if (mVisualizer) {
        mVisualizer->drawKeypoints(*getLastKeyFrame(), *currFrame, trackablePairs);
      }

      if (currFrame->mNumTrackablePairs< defMinNumTrackablePairs) {
#ifdef DEBUG
        cout << "Too few trackable pairs" <<endl;
#endif
        // shall backtrack
        kfd = KeyFrameBackTrack;
      } else {

        //  pose estimation given the feature point pairs
        //  note we do not do Levenberg-Marquardt here, as we are not sure if
        //  this is key frame yet.
        TIMERSTART2(PoseEstimateRANSAC);
        currFrame->mNumInliers =
          mPoseEstimator.estimate(*currFrame->mKeypoints, *getLastKeyFrame()->mKeypoints,
              *currFrame->mTrackableIndexPairs,
              currFrame->mRot, currFrame->mShift, false);
        TIMEREND2(PoseEstimateRANSAC);

        mStat.mHistoInliers.push_back(currFrame->mNumInliers);

        currFrame->mInliers0 = NULL;
        currFrame->mInliers1 = NULL;
        fetchInliers(currFrame->mInliers1, currFrame->mInliers0);
        currFrame->mInlierIndices = fetchInliers();
        currFrame->mLastKeyFrameIndex = getLastKeyFrame()->mFrameIndex;
#ifdef DEBUG
        cout << "num of inliers: "<< currFrame->mNumInliers <<endl;
#endif
        assert(getLastKeyFrame());
        if (mVisualizer) mVisualizer->drawTracking(*getLastKeyFrame(), *currFrame);

        // Decide if we need select a key frame by now
        kfd =
          keyFrameEval(currFrame->mFrameIndex, currFrame->mKeypoints->size(),
              currFrame->mNumInliers,
              currFrame->mRot, currFrame->mShift);

        // key frame action
//        insertNewKeyFrame = keyFrameAction(kfd, frameSeq);
      }
    } // not first frame;
  } // stop == false

  insertNewKeyFrame = keyFrameAction(kfd, frameSeq);

  TIMEREND(Total);
  return insertNewKeyFrame;
}

bool PathRecon::keyFrameAction(KeyFramingDecision kfd, FrameSeq& frameSeq) {
  bool insertNewKeyFrame = false;
  PoseEstFrameEntry *& currFrame = frameSeq.mCurrentFrame;
  switch (kfd) {
  case KeyFrameSkip:  {
    // skip this frame
    break;
  }
  case KeyFrameBackTrack:   {
    // go back to the last good frame
    // do smoothing
    if (frameSeq.backTrack()== true) {
      TIMERSTART2(PoseEstimateLevMarq);
      mPoseEstimator.estimateWithLevMarq(*(currFrame->mInliers1),
          *(currFrame->mInliers0), currFrame->mRot, currFrame->mShift);
      TIMEREND2(PoseEstimateLevMarq);
    }
    updateTrajectory();
    insertNewKeyFrame = true;
    break;
  }
  case KeyFrameKeep:   {
    // keep current frame as last good frame.
    frameSeq.keepCurrentAsGoodFrame();
    break;
  }
  case KeyFrameUse:  {
    // use currFrame as key frame
    // do smoothing
    if (mFrameSeq.mNumFrames>1) {
      TIMERSTART2(PoseEstimateLevMarq);
      mPoseEstimator.estimateWithLevMarq(*currFrame->mInliers1,
          *currFrame->mInliers0, currFrame->mRot, currFrame->mShift);
      TIMEREND2(PoseEstimateLevMarq);
    }
    updateTrajectory();
    insertNewKeyFrame = true;
    break;
  }
  default:
    break;
  }
  return insertNewKeyFrame;
}

PathRecon::Visualizer::Visualizer(PoseEstimateDisp& pe):
  poseEstWinName("Pose Estimated"),
  leftCamWinName("Left  Cam"),
  lastTrackedLeftCam(string("Last Tracked Left Cam")),
  dispWindowName(string("Disparity Map")),
  outputDirname("Output/indoor1/"),
  poseEstimator(pe),
  canvasKeypointRedrawn(false),
  canvasTrackingRedrawn(false),
  canvasDispMapRedrawn(false)
  {
  // create a list of windows to display results
  cvNamedWindow(poseEstWinName.c_str(), CV_WINDOW_AUTOSIZE);
  cvNamedWindow(leftCamWinName.c_str(), CV_WINDOW_AUTOSIZE);
//  cvNamedWindow(dispWindowName.c_str(), CV_WINDOW_AUTOSIZE);
//  cvNamedWindow(lastTrackedLeftCam.c_str(), CV_WINDOW_AUTOSIZE);

  cvMoveWindow(poseEstWinName.c_str(), 0, 0);
  cvMoveWindow(leftCamWinName.c_str(), 650, 0);
//  cvMoveWindow(dispWindowName.c_str(), 650, 530);
//  cvMoveWindow(lastTrackedLeftCam.c_str(), 0, 530);
}
void PathRecon::Visualizer::drawDisparityMap(WImageBuffer1_16s& dispMap) {
  double maxDisp = (int)poseEstimator.getD(400); // the closest point we care is at least 1000 mm away
  CvMatUtils::getVisualizableDisparityMap(dispMap, canvasDispMap, maxDisp);
}

void PathRecon::Visualizer::drawKeypoints(
    const PoseEstFrameEntry& lastFrame,
    const PoseEstFrameEntry& currentFrame,
    const vector<pair<CvPoint3D64f, CvPoint3D64f> >& pointPairsInDisp
) {
  int imgWidth  = currentFrame.mImage->Width();
  int imgHeight = currentFrame.mImage->Height();
  // make sure the image buffers is allocated to the right sizes
  canvasKeypoint.Allocate(imgWidth, imgHeight);

  assert(currentFrame.mImage);
  cvCvtColor(currentFrame.mImage->Ipl(), canvasKeypoint.Ipl(),  CV_GRAY2RGB);

  CvMatUtils::drawPoints(canvasKeypoint, *lastFrame.mKeypoints,*currentFrame.mKeypoints);
  // The following two line shall draw the same lines between point pairs.
  //  CvMatUtils::drawLines(canvasKeypoint, pointPairsInDisp);
  CvMatUtils::drawLines(canvasKeypoint, *currentFrame.mTrackableIndexPairs,*currentFrame.mKeypoints, *lastFrame.mKeypoints);
  sprintf(leftCamWithMarks, "%s/leftCamWithMarks-%04d.png", outputDirname.c_str(),
      currentFrame.mFrameIndex);
  canvasKeypointRedrawn = true;
}

void PathRecon::Visualizer::drawDispMap(const PoseEstFrameEntry& frame) {
  int imgWidth  = frame.mImage->Width();
  int imgHeight = frame.mImage->Height();
  // make sure the image buffers is allocated to the right sizes
  canvasDispMap.Allocate(imgWidth, imgHeight);

  assert(frame.mDispMap);
  drawDisparityMap(*frame.mDispMap);
  sprintf(dispMapFilename, "%s/dispMap-%04d.png", outputDirname.c_str(), frame.mFrameIndex);
  canvasDispMapRedrawn = true;
}

void PathRecon::Visualizer::drawTracking(
    const PoseEstFrameEntry& lastFrame,
    const PoseEstFrameEntry& frame
) {
  int imgWidth  = frame.mImage->Width();
  int imgHeight = frame.mImage->Height();
  // make sure the image buffers is allocated to the right sizes
  canvasTracking.Allocate(imgWidth, imgHeight);

  assert(frame.mImage);
  cvCvtColor(frame.mImage->Ipl(), canvasTracking.Ipl(),  CV_GRAY2RGB);

  bool reversed = true;
  if (frame.mInliers0) {
    assert(frame.mInliers1);
    CvMatUtils::drawMatchingPairs(*frame.mInliers0, *frame.mInliers1, canvasTracking,
        frame.mRot, frame.mShift,
        (PoseEstimateDisp&)poseEstimator, reversed);


    CvPoint3D64f euler;
    CvMatUtils::eulerAngle(frame.mRot, euler);
    char info[256];
    CvPoint org = cvPoint(0, 475);
    CvFont font;
    cvInitFont( &font, CV_FONT_HERSHEY_SIMPLEX, .5, .4);
    sprintf(info, "%04d, KyPt %d, TrckPir %d, Inlrs %d, eulr=(%4.2f,%4.2f,%4.2f), d=%4.1f",
        frame.mFrameIndex, frame.mKeypoints->size(),
        frame.mNumTrackablePairs, frame.mNumInliers,
        euler.x, euler.y, euler.z, cvNorm((const CvMat *)&frame.mShift));

    cvPutText(canvasTracking.Ipl(), info, org, &font, CvMatUtils::yellow);
    sprintf(poseEstFilename,  "%s/poseEst-%04d.png", outputDirname.c_str(), frame.mFrameIndex);
    canvasTrackingRedrawn = true;
  }
}

void PathRecon::Visualizer::show() {
  if (canvasKeypointRedrawn && canvasKeypoint.Ipl())
    cvShowImage(leftCamWinName.c_str(), canvasKeypoint.Ipl());
  if (canvasTrackingRedrawn && canvasTracking.Ipl())
    cvShowImage(poseEstWinName.c_str(), canvasTracking.Ipl());
#if 0 // do not draw disparity map
  if (canvasDispMapRedrawn && canvasDispMap.Ipl())
    cvShowImage(dispWindowName.c_str(),  canvasDispMap.Ipl());
#endif

  // wait for a while for opencv to draw stuff on screen
  cvWaitKey(25);  //  milliseconds
  //    cvWaitKey(0);  //  wait indefinitely
}

void PathRecon::Visualizer::save() {

  // save the marked images
  if (canvasKeypointRedrawn && canvasKeypoint.Ipl()) {
    cvSaveImage(leftCamWithMarks,  canvasKeypoint.Ipl());
  }
  if (canvasTrackingRedrawn && canvasTracking.Ipl()) {
    cvSaveImage(poseEstFilename,   canvasTracking.Ipl());
  }
  if (canvasDispMapRedrawn && canvasDispMap.Ipl()) {
    cvSaveImage(dispMapFilename,    canvasDispMap.Ipl());
  }
}

void PathRecon::Visualizer::reset() {
  // reset all the redrawn flags
  canvasDispMapRedrawn = false;
  canvasKeypointRedrawn = false;
  canvasTrackingRedrawn = false;
}

void PathRecon::visualize() {
  if (mVisualizer) {
    mVisualizer->show();
    mVisualizer->save();
    mVisualizer->reset();
  }
}

bool PathRecon::track(queue<StereoFrame>& inputImageQueue)
{
  bool status = false;

  while (inputImageQueue.size()>0) {
    cout << "---- " << "----" << endl;
    bool newKeyFrame = trackOneFrame(inputImageQueue, mFrameSeq);
    cout << "newKeyFrame " << newKeyFrame << " mFrameSeq.mCurrentFrame " << mFrameSeq.mCurrentFrame << endl;
    if (newKeyFrame == true) {
      // only keep the last frame in the queue
      reduceWindowSize(1);
    }
    visualize();
  }
  return status;
}

vector<FramePose>* PathRecon::getFramePoses() {
  return &mFramePoses;
}

void PathRecon::printStat(){
  mStat.mNumKeyPointsWithNoDisparity = mPoseEstimator.mNumKeyPointsWithNoDisparity;
  mStat.mPathLength = mPathLength;
  mStat.print();
}

void PathRecon::Stat::print(){
  // Please note that because of backtracking, the following two
  // assertion in general will not be true
  //assert(mHistoKeypoints.size() == mHistoTrackablePairs.size());
  //assert(mHistoKeypoints.size() == mHistoInliers.size());
  assert(mHistoKeyFrameKeypoints.size() == mHistoKeyFrameTrackablePairs.size());
  assert(mHistoKeyFrameKeypoints.size() == mHistoKeyFrameInliers.size());
  int numFrames    = mHistoKeypoints.size();
  int numKeyFrames = mHistoKeyFrameKeypoints.size();
  int numTotalKeypoints = 0;
  BOOST_FOREACH(int &numKeypoints, mHistoKeypoints) {
    numTotalKeypoints += numKeypoints;
  }
  int numTotalInliers = 0;
  BOOST_FOREACH(int &numInliers, mHistoInliers) {
    numTotalInliers += numInliers;
  }
  int numTotalTrackablePairs = 0;
  BOOST_FOREACH(int &numTotalTrackablePairs, mHistoTrackablePairs) {
    numTotalTrackablePairs += numTotalTrackablePairs;
  }
  int numTotalKeyFrameKeypoints = 0;
  typedef std::pair<int, int> Pair;
  BOOST_FOREACH( Pair& p, mHistoKeyFrameKeypoints ) {
    numTotalKeyFrameKeypoints += p.second;
  }
  int numTotalKeyFrameInliers = 0;
  BOOST_FOREACH(Pair& p, mHistoKeyFrameInliers) {
    numTotalKeyFrameInliers += p.second;
  }
  int numTotalKeyFrameTrackablePairs = 0;
  BOOST_FOREACH(Pair& p, mHistoKeyFrameTrackablePairs) {
    numTotalKeyFrameTrackablePairs += p.second;
  }
  double scale   = 1. / (double)(numFrames);
  double kfScale = 1. / (double)(numKeyFrames);
  fprintf(stdout, "Num of frames skipped:    %d\n", numFrames-numKeyFrames);
  fprintf(stdout, "Total distance covered:   %05.2f mm\n",mPathLength);
  fprintf(stdout, "Total/Average keypoints:           %d,   %05.2f\n", numTotalKeypoints, (double)(numTotalKeypoints) * scale);
  fprintf(stdout, "Total/Average trackable pairs:     %d,   %05.2f\n", numTotalTrackablePairs, (double)(numTotalTrackablePairs) * scale);
  fprintf(stdout, "Total/Average inliers:             %d,   %05.2f\n", numTotalInliers, (double)(numTotalInliers) * scale);
  fprintf(stdout, "Total/Average keypoints w/o disp:  %d,   %05.2f\n", mNumKeyPointsWithNoDisparity,
      (double)(mNumKeyPointsWithNoDisparity) * kfScale);
  fprintf(stdout, "In Key Frames:\n");
  fprintf(stdout, "Total/Average keypoints:           %d,   %05.2f\n", numTotalKeyFrameKeypoints,      (double)(numTotalKeyFrameKeypoints) * kfScale);
  fprintf(stdout, "Total/Average trackable pairs:     %d,   %05.2f\n", numTotalKeyFrameTrackablePairs, (double)(numTotalKeyFrameTrackablePairs) * kfScale);
  fprintf(stdout, "Total/Average inliers:             %d,   %05.2f\n", numTotalKeyFrameInliers,        (double)(numTotalKeyFrameInliers) *kfScale);

}

// See CvPathRecon.h for documentation
PoseEstFrameEntry::PoseEstFrameEntry(WImageBuffer1_b* image,
    WImageBuffer1_16s* dispMap,
    Keypoints* keypoints, CvMat& rot, CvMat& shift,
    int numTrackablePair,
    int numInliers, int frameIndex,
    WImageBuffer3_b* imageC3a,
    int *inlierIndices,
    vector<pair<int, int> >* inlierIndexPairs,
    CvMat* inliers0, CvMat* inliers1){
  mRot   = cvMat(3, 3, CV_64FC1, _mRot);
  mShift = cvMat(3, 1, CV_64FC1, _mShift);
  mImage = image;
  mDispMap = dispMap;
  mKeypoints  = keypoints;
  cvCopy(&rot,   &mRot);
  cvCopy(&shift, &mShift);
  mNumTrackablePairs = numTrackablePair;
  mNumInliers = numInliers;
  mFrameIndex = frameIndex;

  mImageC3a = imageC3a;
  mInliers0 = inliers0;
  mInliers1 = inliers1;
}

