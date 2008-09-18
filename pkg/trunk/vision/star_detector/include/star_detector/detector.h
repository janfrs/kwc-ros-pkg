#ifndef FEATURES_STAR_DETECTOR_H
#define FEATURES_STAR_DETECTOR_H

#include "star_detector/keypoint.h"
#include "star_detector/nonmax_suppress.h"
#include <cv.h>
#include <vector>
#include <cmath>

/*!
  This class offers a simple interface for keypoint detection.

  The detector searches for center-surround extrema, much like the
  the human low-level vision system. It uses a bilevel center-surround
  filter, approximating a circular shape by combining a box filter with
  another box filter rotated 45 degrees. This filter offers reasonable
  rotational invariance and can be calculated very efficiently using
  integral images. The filter is applied in several sizes (7 by default)
  to determine a keypoint's scale.

  The detector uses non-maximal suppression over the scale and spatial
  dimensions to pick out keypoints. Each keypoint must have a response
  with magnitude exceeding a user-specified threshold. Keypoints which
  are poorly localized along a line are also discarded using a line
  suppression check similar to SIFT's. Note that smaller line response
  thresholds are more stringent. Reasonable defaults for the thresholds
  are provided.
 */
class StarDetector
{
public:
    static const float DEFAULT_THRESHOLD = 30;
    static const float DEFAULT_LINE_THRESHOLD = 10;
    
    //! Constructor. The dimensions of the source images that will be used
    //! with the detector are required to pre-allocate memory.
    StarDetector(CvSize dims, int n = 7, float threshold = DEFAULT_THRESHOLD,
                 float line_threshold = DEFAULT_LINE_THRESHOLD);

    ~StarDetector();

    //! Returns a vector of keypoints in the source image.
    std::vector<Keypoint> DetectPoints(IplImage* source);

    //! Turn scale interpolation on or off. On by default.
    void interpolate(bool value);
    
    // TODO: expose integral images for SURF-type descriptor?

private:
    //! Scale/spatial dimensions
    int m_n, m_W, m_H;
    //! Normal integral image
    IplImage* m_upright;
    //! Tilted (by 45 degrees) integral image
    IplImage* m_tilted;
    //! Tilted integral image with a rounded two-pixel corner
    IplImage* m_flat;
    //! Array of filter response images over different scales
    IplImage** m_responses;
    //! Response magnitude threshold
    float m_threshold;
    //! Line response threshold
    float m_line_threshold;
    //! Filter size at each scale
    int* m_filter_sizes;
    //! Non-maximal suppression functor
    NonmaxSuppress3x3xN<float, LineSuppress> m_nonmax;
    //! Non-minimal suppression functor
    NonmaxSuppress3x3xN<float, LineSuppress, std::less_equal<float> > m_nonmin;
    //! Border size for non-max suppression
    int m_border;
    //! Scale interpolation flag
    bool m_interpolate;
    // TODO: Keep intermediate star sum images for reuse?
    
    static const float SCALE_RATIO = M_SQRT2;

    //! Calculate sum of all pixel values in the "star" shape.
    int StarAreaSum(CvPoint center, int radius, int offset);

    //! Count the number of pixels in the "star" shape. Pixels in the intersection
    //! are counted twice.
    int StarPixels(int radius, int offset);

    //! Calculate the center-surround filter response for some scale.
    void BilevelFilter(IplImage* dst, int scale);

    //! Calculate the filter responses over the range of desired scales.
    void FilterResponses();
    void FilterResponses2();
    void FilterResponses3();
    
    //! Return extrema which satisfy the strength and line response thresholds
    std::vector<Keypoint> FindExtrema();

    //! Fill in scales based in indices and interpolation
    void InterpolateScales(std::vector<Keypoint> &keypoints);
};

#endif
