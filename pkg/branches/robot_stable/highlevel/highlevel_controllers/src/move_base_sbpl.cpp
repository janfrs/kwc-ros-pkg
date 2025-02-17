/*********************************************************************
 * Software License Agreement (BSD License)
 * 
 *  Copyright (c) 2008, Willow Garage, Inc.
 *  All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 * 
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the Willow Garage nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 * 
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/

/**
 * @mainpage
 *
 * @htmlinclude manifest.html
 *
 * @b move_base is...
 *
 * <hr>
 *
 *  @section usage Usage
 *  @verbatim
 *  $ move_base
 *  @endverbatim
 *
 * <hr>
 *
 * @section topic ROS topics
 *
 * Subscribes to (name/type):
 * - @b 
 *
 * Publishes to (name / type):
 * - @b 
 *
 *  <hr>
 *
 * @section parameters ROS parameters
 *
 * - None
 **/

#include <MoveBase.hh>
#include <sbpl_util.hh>
#include <environment_wrap.h>
#include <plan_wrap.h>

#include <sbpl/headers.h>
#include <err.h>


namespace {
  
  /** Set an angle to the range -pi < angle <= +pi */
  double mod2pi(double x)
  {
    x = fmod(x, 2 * M_PI);
    if (x > M_PI)
      x -= 2 * M_PI;
    else if (x <= - M_PI)
      x += 2 * M_PI;
    return x;
  }
  
}


/**
 * @todo Resolve issue with blocking ino locking
 */

namespace ros {
  namespace highlevel_controllers {

    /**
     * @brief Specialization for the SBPL planner
     */
    class MoveBaseSBPL: public MoveBase {
    public:
      MoveBaseSBPL();

      virtual ~MoveBaseSBPL();

    protected:

      /**
       * @brief Builds a plan from current state to goal state
       */
      virtual bool makePlan();
      
      /** do MoveBase::updateGoalMsg() and increment goalCount_ */
      virtual void updateGoalMsg();
      
    private:
      typedef list<ompl::SBPLPlannerStatsEntry> plannerStats_t;

      bool isMapDataOK();

      MDPConfig mdpCfg_;
      ompl::EnvironmentWrapper * env_;
      ompl::SBPLPlannerManager * pMgr_;
      plannerStats_t pStat_;
      double plannerTimeLimit_; /* The amount of time given to the planner to find a plan */
      std::string planStatsFile_;
      size_t goalCount_;
    };
    
    
    MoveBaseSBPL::MoveBaseSBPL()
      : MoveBase(),
	env_(0),
	pMgr_(0),
	goalCount_(0)
    {
      try {
	// We're throwing int exit values if something goes wrong, and
	// clean up any new instances in the catch clause. The sentry
	// gets destructed when we go out of scope, so unlock() gets
	// called no matter what.
	sentry<MoveBaseSBPL> guard(this);
	local_param("planStatsFile", planStatsFile_, string("/tmp/move_base_sbpl.log"));
	local_param("plannerTimeLimit", plannerTimeLimit_, 10.0);
	/*
	if (0 > plannerTimeLimit_) {
	  int blah;
	  local_param("plannerTimeLimit", blah, -1); // parameters are picky about dots
	  if (0 > blah) {
	    ROS_ERROR("invalid or no %s/plannerTimeLimit specified: %g",
		      get_name().c_str(), plannerTimeLimit_);
	    throw int(7);
	  }
	  plannerTimeLimit_ = blah;
	}
	*/
	string environmentType;
	local_param("environmentType", environmentType, string("2D"));
	
	if ("2D" == environmentType) {
	  // Initial Configuration is set with the threshold for
	  // obstacles set to the inscribed obstacle threshold. These,
	  // lethal obstacles, and cells with no information will thus
	  // be regarded as obstacles
	  env_ = new ompl::EnvironmentWrapper2D(ompl::createCostmapWrap(&getCostMap()), true,
						ompl::createIndexTransformWrap(&getCostMap()), true,
						0, 0, 0, 0,
						CostMap2D::INSCRIBED_INFLATED_OBSTACLE);
	}
	else if ("3DKIN" == environmentType) {
	  string const prefix("env3d/");
	  string obst_cost_thresh_str;
	  local_param(prefix + "obst_cost_thresh", obst_cost_thresh_str, string("lethal"));
	  unsigned char obst_cost_thresh(0);
	  if ("lethal" == obst_cost_thresh_str)
	    obst_cost_thresh = costmap_2d::CostMap2D::LETHAL_OBSTACLE;
	  else if ("inscribed" == obst_cost_thresh_str)
	    obst_cost_thresh = costmap_2d::CostMap2D::INSCRIBED_INFLATED_OBSTACLE;
	  else {
	    ROS_ERROR("invalid env3d/obst_cost_thresh \"%s\"\n"
		      "  valid options: lethal, inscribed, or circumscribed",
		      obst_cost_thresh_str.c_str());
	    throw int(6);
	  }
	  double goaltol_x, goaltol_y, goaltol_theta,
	    nominalvel_mpersecs, timetoturn45degsinplace_secs;
	  local_param(prefix + "goaltol_x", goaltol_x, 0.3);
	  local_param(prefix + "goaltol_y", goaltol_y, 0.3);
	  local_param(prefix + "goaltol_theta", goaltol_theta, 30.0);
	  local_param(prefix + "nominalvel_mpersecs", nominalvel_mpersecs, 0.4);
	  local_param(prefix + "timetoturn45degsinplace_secs", timetoturn45degsinplace_secs, 0.6);
	  // Could also sanity check the other parameters...
	  env_ = new ompl::EnvironmentWrapper3DKIN(ompl::createCostmapWrap(&getCostMap()), true,
						   ompl::createIndexTransformWrap(&getCostMap()), true,
						   obst_cost_thresh,
						   0, 0, 0, // start (x, y, th)
						   0, 0, 0, // goal (x, y, th)
						   goaltol_x, goaltol_y, goaltol_theta,
						   getFootprint(), nominalvel_mpersecs,
						   timetoturn45degsinplace_secs);
	}
	else {
	  ROS_ERROR("in MoveBaseSBPL ctor: invalid environmentType \"%s\", use 2D or 3DKIN",
		    environmentType.c_str());
	  throw int(2);
	}
	
	// This (weird?) order of inits is historical, could maybe be
	// cleaned up...
	bool const success(env_->InitializeMDPCfg(&mdpCfg_));
// 	try {
// 	  // This one throws a string if something goes awry... and
// 	  // always returns true. So no use cecking its
// 	  // retval. Another candidate for cleanup.
// 	  isMapDataOK();
// 	}
// 	catch (char const * ee) {
// 	  ROS_ERROR("in MoveBaseSBPL ctor: isMapDataOK() said %s", ee);
// 	  throw int(3);
// 	}
	if ( ! success) {
	  ROS_ERROR("in MoveBaseSBPL ctor: env_->InitializeMDPCfg() failed");
	  throw int(4);
	}
	
	string plannerType;
	local_param("plannerType", plannerType, string("ARAPlanner"));
	pMgr_ = new ompl::SBPLPlannerManager(env_->getDSI(), false, &mdpCfg_);
	if ( ! pMgr_->select(plannerType, false, 0)) {
	  ROS_ERROR("in MoveBaseSBPL ctor: pMgr_->select(%s) failed", plannerType.c_str());
	  throw int(5);
	}
      }
      catch (int ii) {
	delete env_;
	delete pMgr_;
	exit(ii);
      }
      
      //Now initialize
      initialize();
    }
    
    MoveBaseSBPL::~MoveBaseSBPL(){
      delete env_;
      delete pMgr_;
    }

    bool MoveBaseSBPL::isMapDataOK() {
      const CostMap2D& cm = getCostMap();
      
      for(unsigned int i = 0; i<cm.getWidth(); i++){
	for(unsigned int j = 0; j < cm.getHeight(); j++){
	  if(env_->IsObstacle(i, j) && cm.getCost(i, j) < CostMap2D::INSCRIBED_INFLATED_OBSTACLE){
	    ROS_DEBUG("Extra obstacle at <%d, %d>", i, j);
	    throw "Extra obstacle in sbpl";
	  }
	  if(!env_->IsObstacle(i, j) && cm.getCost(i, j) >= CostMap2D::INSCRIBED_INFLATED_OBSTACLE){
	    ROS_DEBUG("Missing obstacle at <%d, %d>", i, j);
	    throw "Missing obstacle in sbpl";
	  }
	}
      }

      return true;
    }

    bool MoveBaseSBPL::makePlan(){
      ROS_DEBUG("Planning for new goal...\n");
      
      ompl::SBPLPlannerStatsEntry statsEntry(pMgr_->getName(), env_->getName());      
      try {
	// Update costs
	lock();
	const CostMap2D& cm = getCostMap();
	unsigned int x = cm.getWidth();
	while(x > 0){
	  x--;
	  unsigned int y = cm.getHeight();
	  while(y > 0){
	    y--;
	    // Note that ompl::EnvironmentWrapper::UpdateCost() will
	    // check if the cost has actually changed, and do nothing
	    // if it hasn't.
	    env_->UpdateCost(x, y, (unsigned char) cm.getCost(x, y));
	  }
	}
	unlock();
	
	// Tell the planner about the changed costs. Again, the called
	// code checks whether anything has really changed before
	// embarking on expensive computations.
	pMgr_->flush_cost_changes(*env_);

	// Copy out start and goal states to minimize locking requirement. Lock was not previously required because the
	// planner and controller were running on the same thread and the only contention was for cost map updates on call
	// backs. Note that cost map queries here are const methods that merely do co-ordinate transformations, so we do not need
	// to lock protect those.
	stateMsg.lock();
	statsEntry.start = stateMsg.pos;
	statsEntry.goal = stateMsg.goal;
	stateMsg.unlock();

	// Set start state based on global pose, updating statistics in the process.
	cm.WC_MC(statsEntry.start.x, statsEntry.start.y, statsEntry.startIx, statsEntry.startIy);
 	env_->SetStart(statsEntry.start);
 	statsEntry.startState = env_->GetStateFromPose(statsEntry.start);
	if (0 > statsEntry.startState) {
	  ROS_ERROR("invalid start state ID %d from pose (%+8.3f, %+8.3f): outside of map?\n",
		    statsEntry.startState, statsEntry.start.x, statsEntry.start.y);
	  return false;
	}
	int status(pMgr_->set_start(statsEntry.startState));
	if (1 != status) {
	  ROS_ERROR("failed to set start state ID %d from (%ud, %ud): pMgr_->set_start() returned %d\n",
		    statsEntry.startState, statsEntry.startIx, statsEntry.startIy, status);
	  return false;
	}
	
	// Set goal state, updating statistics in the process.
	cm.WC_MC(statsEntry.goal.x, statsEntry.goal.y, statsEntry.goalIx, statsEntry.goalIy);
 	env_->SetGoal(statsEntry.goal);
 	statsEntry.goalState = env_->GetStateFromPose(statsEntry.goal);
	if (0 > statsEntry.goalState) {
	  ROS_ERROR("invalid goal state ID %d from pose (%+8.3f, %+8.3f): outside of map?\n",
		    statsEntry.goalState, statsEntry.goal.x, statsEntry.goal.y);
	  return false;
	}
	status = pMgr_->set_goal(statsEntry.goalState);
	if (1 != status) {
	  ROS_ERROR("failed to set goal state ID %d from (%ud, %ud): pMgr_->set_goal() returned %d\n",
		    statsEntry.goalState, statsEntry.goalIx, statsEntry.goalIy, status);
	  return false;
	}
	
	// Invoke the planner, updating the statistics in the process.
	std::vector<int> solutionStateIDs;
	statsEntry.allocated_time_sec = plannerTimeLimit_;
	statsEntry.stop_at_first_solution = false;
	statsEntry.plan_from_scratch = false;
	statsEntry.status = pMgr_->replan(statsEntry.stop_at_first_solution,
					  statsEntry.plan_from_scratch,
					  statsEntry.allocated_time_sec,
					  &statsEntry.actual_time_wall_sec,
					  &statsEntry.actual_time_user_sec,
					  &statsEntry.actual_time_system_sec,
					  &statsEntry.number_of_expands,
					  &statsEntry.solution_cost,
					  &statsEntry.solution_epsilon,
					  &solutionStateIDs);

	// Extract the solution, if available, and update statistics (as usual).
	statsEntry.plan_length_m = 0;
	statsEntry.plan_angle_change_rad = 0;
	if ((1 == statsEntry.status) && (1 < solutionStateIDs.size())) {
	  ompl::waypoint_plan_t plan;
	  ompl::convertPlan(*env_,
			    solutionStateIDs,
			    &plan,
			    &statsEntry.plan_length_m,
			    &statsEntry.plan_angle_change_rad,
			    0	// should be non-null for 3DKIN planning...
			    );
	  {
	    ostringstream prefix_os;
	    prefix_os << "[" << goalCount_ << "] ";
	    static size_t lastPlannedGoal(171717);
	    char const * title("\nREPLAN SUCCESS");
	    if (lastPlannedGoal != goalCount_) {
	      lastPlannedGoal = goalCount_;
	      title = "\nPLAN SUCCESS";
	    }
	    statsEntry.logFile(planStatsFile_.c_str(), title, prefix_os.str().c_str());
	  }
	  ////	  statsEntry.logInfo("move_base_sbpl: ");
	  pStat_.push_back(statsEntry);
	  
	  updatePlan(plan);
	  return true;
	}
      }
      catch (std::runtime_error const & ee) {
	ROS_ERROR("runtime_error in makePlan(): %s\n", ee.what());
	return false;
      }

      ROS_ERROR("No plan found\n");
      {
	static size_t lastFailedGoal(424242);
	if (lastFailedGoal != goalCount_) {
	  lastFailedGoal = goalCount_;
	  ostringstream prefix_os;
	  prefix_os << "[" << goalCount_ << "] ";
	  statsEntry.logFile(planStatsFile_.c_str(), "\nPLAN FAILURE", prefix_os.str().c_str());
	}
      }
      return false;
    }
    
    
    void MoveBaseSBPL::updateGoalMsg()
    {
      MoveBase::updateGoalMsg();
      ++goalCount_;
    }
    
  }
}


int main(int argc, char** argv)
{
  ros::init(argc,argv); 

  ros::highlevel_controllers::MoveBaseSBPL node;

  try {
    node.run();
  }
  catch(char const* e){
    std::cout << e << std::endl;
  }

  ros::fini();

  return(0);
}
