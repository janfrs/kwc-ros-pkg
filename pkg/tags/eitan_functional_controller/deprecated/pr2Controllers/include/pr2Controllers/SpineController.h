#pragma once
/***************************************************/
/*! \class CONTROLLER::SpineController
    \brief A PR2 Spine controller
    
    This class implements controller loops for
    PR2 Spine Control

*/
/***************************************************/
//#include <newmat10/newmat.h>
//#include <libKinematics/ik.h>
//#include <sys/types.h>
//#include <stdint.h>
//#include <libKDL/kdl_kinematics.h> // for kinematics using KDL -- util/kinematics/libKDL

#include <iostream>

#include <pr2Core/pr2Core.h>
#include <genericControllers/Controller.h>

namespace controller
{
  class SpineController : Controller
  {
    public:
    
      /*!
        * \brief Constructor of SpineController class.
        *
        * \param 
        */
      SpineController();
      
      /*!
        * \brief Destructor of SpineController class.
        */       
      ~SpineController( );

      /*!
        * \brief Update controller
        */       
      void Update( );

      /*!
        * \brief Set height of the spine in the local spine frame.
        * 
        */       
      PR2::PR2_ERROR_CODE setPos(double z);


      /*!
        * \brief Set force of the Spine in the local spine frame.
        */       
      PR2::PR2_ERROR_CODE setForce(double fz);

      /*!
        * \brief Set parameters for this controller
        *
        * user can set maximum velocity
        * and maximum acceleration
        * constraints for this controller
        *
        * e.g. setParam('maxVel',10);
        *   or setParam('maxAcc',10);
        *   or setParam('maxLimit',10);
        *   or setParam('minLimit',-10);
        *
        */
      PR2::PR2_ERROR_CODE setParam(std::string label,double value);

    private:
      PR2::PR2_CONTROL_MODE controlMode;      /**< SpineController control mode >*/
  };
}

