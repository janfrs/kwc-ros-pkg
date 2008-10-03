/*
 * CvVisOdemBundleAdj.h
 *
 *  Created on: Sep 17, 2008
 *      Author: jdchen
 */

#ifndef CVVISODOMBUNDLEADJ_H_
#define CVVISODOMBUNDLEADJ_H_

#include "PathRecon.h"

namespace cv {
namespace willow {
/**
 * (Under construction) Visual Odometry by sliding window bundle adjustment.
 * The input are a sequence of stereo video images.
 */
class VisOdomBundleAdj: public PathRecon {
public:
  /// Record the observation of a tracked point w.r.t.  a frame
  class TrackObserv {
  public:
    TrackObserv(const int fi, const CvPoint3D64f& coord, const int keypointIndex):
      mFrameIndex(fi), mDispCoord(coord), mKeypointIndex(keypointIndex){}
    /// the index of the image frame;
    int           mFrameIndex;
    /// disparity coordinates of the point in this frame
    CvPoint3D64f  mDispCoord;
    /// the index of this point in the keypoint list of the frame
    int           mKeypointIndex;
  };
  /// A sequence of observations of a 3D feature over a sequence of
  /// video images.
  class Track: public deque<TrackObserv> {
  public:
    typedef deque<TrackObserv> Parent;
    Track(const TrackObserv& obsv0, const TrackObserv& obsv1,
            const CvPoint3D64f& coord, int frameIndex):
          mCoordinates(coord)
        {
          push_back(obsv0);
          push_back(obsv1);
        }
    inline void extend(const TrackObserv& obsv){
      push_back(obsv);
    }
    /// purge all observation that is older than oldestFrameIndex
    void purge(int oldestFrameIndex) {
      while (front().mFrameIndex < oldestFrameIndex) {
        pop_front();
      }
    }
    inline size_type size() const { return Parent::size();}
    /// Index of the frame within the slide window with the lowest index in which
    /// this track is detected.
    inline int firstFrameIndex() const { return front().mFrameIndex;}
    /// Index of the frame within the slide window with the highest index in which
    /// this track is detected.
    inline int lastFrameIndex()  const { return back().mFrameIndex; }
    void print() const;

    /// estimated 3D Cartesian coordinates.
    CvPoint3D64f     mCoordinates;
  protected:
  };
  /// Book keeping of the tracks.
  class Tracks {
  public:
    Tracks():mCurrentFrameIndex(0){}
    Tracks(Track& track, int frameIndex):mCurrentFrameIndex(frameIndex){
      mTracks.push_back(track);
    }
    /// purge the tracks for tracks and track observations
    /// that are older than oldestFrameIndex
    void purge(int oldestFrameIndex);
    void print() const;
    /// collection stats of the tracks
    void stats(int *numTracks, int *maxLen, int* minLen, double *avgLen,
        /// histogram of the length of the tracks
        vector<int>* lenHisto) const;
    /// a container for all the tracks
    deque<Track> mTracks;
    /// The index of the last frame that tracks have been
    /// constructed against.
    int mCurrentFrameIndex;
  };
  VisOdomBundleAdj(
      /// Image size. Use for buffer allocation.
      const CvSize& imageSize);
  virtual ~VisOdomBundleAdj();

  /**
   * Given a sequence of stereo video, reconstruct the path of the
   * camera.
   * dirname  - directory of where the video sequence is stored
   * leftFileFmt  - format for generating the filename of an image from the
   *                left camera. e.g. left-%04d.ppm, for filenames like
   *                left-0500.ppm, etc.
   * rightFileFmt - format for generating the filename of an image from the
   *                right camera. Same convention as leftFileFmt.
   * start        - index of the first frame to be processed
   * end          - index of the first frame not to be process
   *              - namely, process the frame at most to frame number end-1
   * step         - step size of the increase of the index from one frame
   *                to next one.
   */
  bool recon(const string& dirname, const string& leftFileFmt,
      const string& rightFileFmt, int start, int end, int step);

  /// Slide down the end mark of the sliding window. Update the
  /// beginning end of applicable. Plus book keeping of the tracks
  /// and others.
  void updateSlideWindow();

  bool updateTracks(
      deque<PoseEstFrameEntry*>& frames,
      Tracks& tracks);
  void purgeTracks(int frameIndex);

  /// Default size of the sliding window
  static const int DefaultSlideWindowSize  = 10;
  /// Default number of iteration
  static const int DefaultNumIteration = 20;

  /// Visualizing the visual odometry process of bundle adjustment.
  class Visualizer: public PathRecon::Visualizer {
  public:
    typedef PathRecon::Visualizer Parent;
    Visualizer(PoseEstimateDisp& poseEstimator,
        const vector<FramePose>& framePoses,
        const Tracks& trcks):
      Parent(poseEstimator), framePoses(framePoses), tracks(trcks){}
    virtual ~Visualizer(){}
    /// Draw keypoints, tracks and disparity map on canvases for visualization
    virtual void drawTracking(
        const PoseEstFrameEntry& lastFrame,
        const PoseEstFrameEntry& frame);

    /// Draw the tracks. A track is drawn as a polyline connecting the observations
    /// of the same point in a sequence of frames. A line is drawn in yellow
    /// if one of the end point is outside of the slide window. Red otherwise.
    virtual void drawTrack(const PoseEstFrameEntry& frame);
    virtual void drawTrackTrajectories(const PoseEstFrameEntry& frame);
    virtual void drawTrackEstimatedLocations(const PoseEstFrameEntry& frame);


    /// a reference to the estimated pose of the frames
    const vector<FramePose>& framePoses;
    /// a reference to the tracks.
    const Tracks& tracks;
    int   slideWindowFront;
  };
  /// Statistics for bundle adjustment
  class Stat2 {
  public:
    void print();
    vector<int> numTracks;
    vector<int> maxTrackLens;
    vector<int> minTrackLens;
    vector<double> avgTrackLens;
    vector<int> trackLenHisto;
  };
  Stat2 mStat2;
  void updateStat2();

protected:
  /// If matched, extend an existing old track.
  /// @return true if a track is matched and extended. False otherwise.
  bool extendTrack(
      /// Reference to a collection of tracks.
      Tracks& tracks,
      /// The new frame, with new trackable pairs.
      PoseEstFrameEntry& frame,
      /// the index of the inlier,
      int inlierIndex
  );
  bool addTrack(
      /// Reference to a collection of tracks.
      Tracks& tracks,
      /// The new frame, with new trackable pairs.
      PoseEstFrameEntry& frame,
      /// the index of the inlier,
      int inlierIndex
  );
  Tracks mTracks;
  /// size of the sliding window
  int mSlideWindowSize;
  /// number of frozen cameras in bundle adjustment.  Frozen (or fixed)
  /// frame (cameras) are those fall out of the sliding window, but still share
  /// tracks with frames(cameras) inside the sliding window.
  /// At the beginning when there are not enough frames (cameras) for a full
  /// slide window, we always keep the first frame frozen.
  int mNumFrozenWindows;

  /// number of iteration for bundle adjustment.
  int mNumIteration;

};
}
}
#endif /* CVVISODOMBUNDLEADJ_H_ */
