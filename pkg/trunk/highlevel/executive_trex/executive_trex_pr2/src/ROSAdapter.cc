#include "ROSAdapter.hh"
#include "Agent.hh"
#include "Observer.hh"
#include "Object.hh"
#include "Debug.hh"
#include "Observer.hh"
#include "ROSNode.hh"
#include "Logger.hh"

namespace TREX { 				     

  ROSAdapter::ROSAdapter(const LabelStr& agentName, const TiXmlElement& configData, TICK lookAhead)
    : Adapter(agentName, configData, lookAhead, 0), m_initialized(false),
      timelineName(extractData(configData, "timelineName").toString()),
      timelineType(extractData(configData, "timelineType").toString()), 
      stateTopic(extractData(configData, "stateTopic").toString()){
    m_node = ROSNode::request();

    // Iterate over child xml nodes and look for nodes of type Param to populate the nddl to ros mappings
    // Iterate over internal and external configuration specifications
    for (TiXmlElement * child = configData.FirstChildElement();
           child != NULL;
           child = child->NextSiblingElement()) {

        if(strcmp(child->Value(), "Param") == 0) {
	  LabelStr nddl = extractData(*child, "nddl");
	  LabelStr ros = extractData(*child, "ros");
	  nddlNames_.push_back(nddl.toString());
	  rosNames_.push_back(ros.toString());
	}
    }
  }

  void ROSAdapter::handleInit(TICK initialTick, const std::map<double, ServerId>& serversByTimeline, const ObserverId& observer){
    TREX::Adapter::handleInit(initialTick, serversByTimeline, observer);

    registerPublishers();

    registerSubscribers();

    // Wait till we get a message before starting the agent
    while(!isInitialized() && m_node->ok()){
      debugMsg("ROS:Create", "Waiting to connect for " << timelineName);
      sleep(1);
    }

    debugMsg("ROS:Create", "Connection established for " << timelineName);
  }

  ROSAdapter::~ROSAdapter() {
    m_node->release();
  }

  bool ROSAdapter::synchronize(){
    debugMsg("ROS:synchronize", nameString() << "Checking..");

    // Derived class will populate actual observations
    Observation* obs = NULL;
    obs = getObservation();

    if(obs != NULL){
      sendNotify(*obs);
      delete obs;
    }

    return true;
  }

  void ROSAdapter::handleNextTick(){}

  void ROSAdapter::handleCallback(){
    m_initialized = true;
  }

  bool ROSAdapter::isInitialized() const {
    return m_initialized;
  }

  void ROSAdapter::handleRequest(const TokenId& goal){
    dispatchRequest(goal, true);
  }

  void ROSAdapter::handleRecall(const TokenId& goal){
    dispatchRequest(goal, false);
  }
}
