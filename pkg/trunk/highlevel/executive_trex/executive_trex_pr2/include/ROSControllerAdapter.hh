#ifndef H_ROSControllerAdapter
#define H_ROSControllerAdapter

#include "ROSAdapter.hh"
#include "Token.hh"
#include "TokenVariable.hh"

namespace TREX {

  /**
   * @brief The last reported state of the controller
   */
  enum ControllerState {
    UNDEFINED = 0,
    INACTIVE,
    ACTIVE
  };

  template <class S, class G> class ROSControllerAdapter: public ROSAdapter {
  public:

    ROSControllerAdapter(const LabelStr& agentName, const TiXmlElement& configData)
      : ROSAdapter(agentName, configData, 1), inactivePredicate(timelineType + ".Inactive"), activePredicate(timelineType + ".Active"),
	state(UNDEFINED), lastPublished(-1), lastUpdated(0), goalTopic(extractData(configData, "goalTopic").toString()) {}

    virtual ~ROSControllerAdapter(){}

  protected:
    S stateMsg;

    /**
     * @brief Handle state update message from the controller. This will fill the stateMsg parameters and
     * also handle the interpretation of the control flags to trigger state updates
     */
    virtual void handleCallback(){
      // Parent callback will handle initialization
      ROSAdapter::handleCallback();

      // If we have already changed the state in this tick, we do not want to over-ride that. This will ensure we do not miss a state change
      // where the goal to move is accompished almost instantly, for example if the robot is already at the goal.
      if(lastUpdated == getCurrentTick() && state != UNDEFINED)
	return;

      if(stateMsg.active && state != ACTIVE){
	state = ACTIVE;
	lastUpdated = getCurrentTick();
	debugMsg("ROS", "Received transition to INACTIVE");
      }
      else if(!stateMsg.active && state != INACTIVE){
	state = INACTIVE;
	lastUpdated = getCurrentTick();
	debugMsg("ROS", "Received transition to INACTIVE");
      }
    }

    void registerSubscribers() {
      debugMsg("ROS", "Registering subscriber for " << timelineName << " on topic:" << stateTopic);
      m_node->registerSubscriber(stateTopic, stateMsg, &TREX::ROSControllerAdapter<S, G>::handleCallback, this, QUEUE_MAX());
    }

    void registerPublishers(){
      debugMsg("ROS", "Registering publisher for " << timelineName << " on topic:" << goalTopic);
      m_node->registerPublisher<G>(goalTopic, QUEUE_MAX());
    }

    Observation* getObservation(){
      // Nothing to do if we published for the last update
      if(((int) lastUpdated) == lastPublished)
	return NULL;

      // We shuold never be getting an observation when in an undefined state. If we are, it means we failed to
      // initialize, which will likely be a message subscription error or an absence of an expected publisher
      // to initialize state.
      if(state == UNDEFINED){
	throw "ROSControllerAdapter: Tried to get an observation on with no initial state set yet.";
      }

      ObservationByValue* obs = NULL;

      stateMsg.lock();
      if(state == INACTIVE){
	obs = new ObservationByValue(timelineName, inactivePredicate);
	fillInactiveObservationParameters(obs);
      }
      else {
	obs = new ObservationByValue(timelineName, activePredicate);
	fillActiveObservationParameters(obs);
      }
      stateMsg.unlock();

      lastPublished = lastUpdated;
      return obs;
    }

    /**
     * The goal can be enabled or disabled.
     * The predicate can be active or inactive
     */
    bool dispatchRequest(const TokenId& goal, bool enabled){
      debugMsg("ROS", "Received dispatch request for " << goal->toString());

      bool enableController = enabled;

      // If the request to move into the inactive state, then evaluate the time bound and only process
      // if it is a singleton
      if(goal->getPredicateName() != activePredicate){
	if(goal->start()->lastDomain().getUpperBound() > getCurrentTick())
	  return false;

	enableController = false;
      }

      G goalMsg;
      fillRequestParameters(goalMsg, goal);
      goalMsg.enable = enableController;
      debugMsg("BaseControllerAdapter", "Dispatching " << goal->toString());
      m_node->publishMsg<G>(goalTopic, goalMsg);
      return true;
    }

    virtual void fillActiveObservationParameters(ObservationByValue* obs) = 0;

    virtual void fillInactiveObservationParameters(ObservationByValue* obs) = 0;

    virtual void fillRequestParameters(G& goalMsg, const TokenId& goalToken) = 0;

  private:
    const LabelStr inactivePredicate;
    const LabelStr activePredicate;
    ControllerState state;
    int lastPublished;
    TICK lastUpdated;
    const std::string goalTopic;
  };
}
#endif
