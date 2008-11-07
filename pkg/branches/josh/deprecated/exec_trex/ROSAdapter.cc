#include "ROSAdapter.hh"
#include "Agent.hh"
#include "Observer.hh"
#include "Object.hh"
#include "Debug.hh"
#include "Token.hh"
#include "TokenVariable.hh"
#include "Observer.hh"
#include "ROSNode.hh"
#include "Logger.hh"

namespace TREX {

  ROSAdapter::ROSAdapter(const LabelStr& agentName, const TiXmlElement& configData)
    : Adapter(agentName, configData){
  }

  void ROSAdapter::handleInit(TICK initialTick, const std::map<double, ServerId>& serversByTimeline, 
				 const ObserverId& observer){
    m_observer = observer;
    m_node = ROSNode::request();

    // Wait till we get a message before starting the agent
    while(!m_node->isInitialized() && m_node->ok()){
      debugMsg("ROSAdapter:Create", "Waiting...");
      sleep(1);
    }

  }

  ROSAdapter::~ROSAdapter() {
    m_node->release();
  }

  bool ROSAdapter::synchronize(){
    debugMsg("ROSAdapter:synchronize", nameString() << "Checking..");
    std::vector<Observation*> obsBuffer;
    m_node->get_obs(obsBuffer,getCurrentTick());

    // Because the VelCommander is an abstracted timeline we are generating a value for it
    // that is hardwired
    if(getCurrentTick() == 0 && Agent::instance()->getOwner("baseGoal") == getId()){
      ObservationByValue* obs = new ObservationByValue("baseGoal", "BaseGoal.Holds");
      obs->push_back("cmd_x", new IntervalDomain(0.0));
      obs->push_back("cmd_th", new IntervalDomain(0.0));
      obsBuffer.push_back(obs);
    }

    if(getCurrentTick() == 0 && Agent::instance()->getOwner("rightEndEffectorGoal") == getId()){
      ObservationByValue* obs = new ObservationByValue("rightEndEffectorGoal", "EndEffectorGoal.Holds");
      obs->push_back("cmd_rot1_1", new IntervalDomain(0.0));
      obs->push_back("cmd_rot1_2", new IntervalDomain(0.0));
      obs->push_back("cmd_rot1_3", new IntervalDomain(0.0));
      obs->push_back("cmd_rot2_1", new IntervalDomain(0.0));
      obs->push_back("cmd_rot2_2", new IntervalDomain(0.0));
      obs->push_back("cmd_rot2_3", new IntervalDomain(0.0));
      obs->push_back("cmd_rot3_1", new IntervalDomain(0.0));
      obs->push_back("cmd_rot3_2", new IntervalDomain(0.0));
      obs->push_back("cmd_rot3_3", new IntervalDomain(0.0));
      obs->push_back("cmd_x", new IntervalDomain(0.0));
      obs->push_back("cmd_y", new IntervalDomain(0.0));
      obs->push_back("cmd_z", new IntervalDomain(0.0));
      obsBuffer.push_back(obs);
    }

    if(getCurrentTick() == 0 && Agent::instance()->getOwner("leftEndEffectorGoal") == getId()){
      ObservationByValue* obs = new ObservationByValue("leftEndEffectorGoal", "EndEffectorGoal.Holds");
      obs->push_back("cmd_rot1_1", new IntervalDomain(0.0));
      obs->push_back("cmd_rot1_2", new IntervalDomain(0.0));
      obs->push_back("cmd_rot1_3", new IntervalDomain(0.0));
      obs->push_back("cmd_rot2_1", new IntervalDomain(0.0));
      obs->push_back("cmd_rot2_2", new IntervalDomain(0.0));
      obs->push_back("cmd_rot2_3", new IntervalDomain(0.0));
      obs->push_back("cmd_rot3_1", new IntervalDomain(0.0));
      obs->push_back("cmd_rot3_2", new IntervalDomain(0.0));
      obs->push_back("cmd_rot3_3", new IntervalDomain(0.0));
      obs->push_back("cmd_x", new IntervalDomain(0.0));
      obs->push_back("cmd_y", new IntervalDomain(0.0));
      obs->push_back("cmd_z", new IntervalDomain(0.0));
      obsBuffer.push_back(obs);
    }

    for(std::vector<Observation*>::const_iterator it = obsBuffer.begin(); it != obsBuffer.end(); ++it){
      Observation* obs = *it;
      debugMsg("ROSAdapter:synchronize", nameString() << obs->toString());
      m_observer->notify(*obs);
      delete obs;
    }

    return true;
  }

  void ROSAdapter::handleNextTick(){}

  void ROSAdapter::handleRequest(const TokenId& token){
    // Constants for Timelines handled
    static const LabelStr BASE_GOAL("BaseGoal");
    static const LabelStr MOVE_ARM_BEHAVIOR("MoveArm");
    static const LabelStr MOVE_ENDEFFECTOR_BEHAVIOR("EndEffectorGoal");

    bool returnObservation = false;

    if(token->getBaseObjectType() == BASE_GOAL){
      debugMsg("ROSAdapter:handleRequest", token->toString());
      m_node->dispatchVel(token);
      returnObservation = true;
    } else if(token->getBaseObjectType() == MOVE_ARM_BEHAVIOR) {
      debugMsg("ROSArmAdapter:handleRequest", token->toString());
      m_node->dispatchArm(token, getCurrentTick());
      //assuming instantly active
      //if(token->getPredicateName() == LabelStr("MoveArm.Active"))
      //returnObservation = true;
    } else if(token->getBaseObjectType() == MOVE_ENDEFFECTOR_BEHAVIOR) {
      debugMsg("ROSAdapter:handleRequest", token->toString());
      m_node->dispatchEndEffector(token, getCurrentTick());
      if(token->getPredicateName() == LabelStr("EndEffectorGoal.Holds"))
	returnObservation = true;
    }

    if(returnObservation){
      ObservationByReference obs(token);
      m_observer->notify(obs);
    }
  }

  // Allocate a Factory
  TeleoReactor::ConcreteFactory<ROSAdapter> l_ROSAdapter_Factory("ROS");
}
