#ifndef WGSTEREOCAMPARAMS_H_
#define WGSTEREOCAMPARAMS_H_

class CvStereoCamParams
{
public:
    /**
     *  Fx  - focal length in x direction of the rectified image in pixels.
     *  Fy  - focal length in y direction of the rectified image in pixels.
     *  Tx  - Translatation in x direction from the left camera to the right camera.
     *  Clx - x coordinate of the optical center of the left  camera
     *  Crx - x coordinate of the optical center of the right camera
     *  Cy  - y coordinate of the optical center of both left and right cameras (they have to be the same)
     */
    CvStereoCamParams(double Fx, double Fy, double Tx, double Clx=DefaultClx, double Crx=DefaultCrx, double Cy=DefaultCy);
    CvStereoCamParams() {
    	*this = CvStereoCamParams(DefaultFx, DefaultFy, DefaultTx, DefaultClx, DefaultCrx, DefaultCy);
    }

    void setParams(double Fx, double Fy, double Tx, double Clx=DefaultClx, double Crx=DefaultCrx, double Cy=DefaultCy){
    	mFx = Fx;
    	mFy = Fy;
    	mTx = Tx;
    	mClx = Clx;
    	mCrx = Crx;
    	mCy  = Cy;
    }

	virtual ~CvStereoCamParams();
    static const double DefaultFx  =   7.290000e+02;
    static const double DefaultFy  =   7.290000e+02;
    static const double DefaultTx  =   4.381214e+004/7.290000e+02;
//    static const double DefaultTx  = -60.049333;
#if 1
    static const double DefaultClx = 3.239809e+002;
    static const double DefaultCrx = 3.239809e+002;
    static const double DefaultCy  = 2.478744e+002;
#else
    static const double DefaultClx = 0.0;
    static const double DefaultCrx = 0.0;
    static const double DefaultCy  = 0.0;
#endif

protected:
    double mFx, mFy;   // focal lengths of the rectified image (in pixels)
    double mTx;        // translation of right camera relative to left camera
    double mClx, mCrx, mCy;  // the optical centers in pixels left: (mClx, mCy) right: (mCrx, mCy)
};

#endif /*WGSTEREOCAMPARAMS_H_*/
