#include "CvStereoCamParams.h"

CvStereoCamParams::CvStereoCamParams(double Fx, double Fy, double Tx, double Clx, double Crx, double Cy):
    mFx(Fx),
    mFy(Fy),
    mTx(Tx),
    mClx(Clx),
    mCrx(Crx),
    mCy(Cy)
{
}

CvStereoCamParams::~CvStereoCamParams()
{
}
