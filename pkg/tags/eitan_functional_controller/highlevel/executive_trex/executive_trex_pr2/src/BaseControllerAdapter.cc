#include "ROSControllerAdapter.hh"
#include "IntervalDomain.hh"
#include "Token.hh"
#include <std_msgs/Planner2DState.h>
#include <std_msgs/Planner2DGoal.h>

namespace TREX {

  class BaseControllerAdapter: public ROSControllerAdapter<std_msgs::Planner2DState, std_msgs::Planner2DGoal> {
  public:

    BaseControllerAdapter(const LabelStr& agentName, const TiXmlElement& configData)
      : ROSControllerAdapter<std_msgs::Planner2DState, std_msgs::Planner2DGoal>(agentName, configData){
    }

    virtual ~BaseControllerAdapter(){}

  protected:

    void fillActiveObservationParameters(ObservationByValue* obs){
      obs->push_back("x", new IntervalDomain(stateMsg.goal.x));
      obs->push_back("y", new IntervalDomain(stateMsg.goal.y));
      obs->push_back("th", new IntervalDomain(stateMsg.goal.th));
    }

    void fillInactiveObservationParameters(ObservationByValue* obs){
      obs->push_back("x", new IntervalDomain(stateMsg.pos.x));
      obs->push_back("y", new IntervalDomain(stateMsg.pos.y));
      obs->push_back("th", new IntervalDomain(stateMsg.pos.th));
    }

    void fillRequestParameters(std_msgs::Planner2DGoal& goalMsg, const TokenId& goalToken){
      const IntervalDomain& x = goalToken->getVariable("x")->lastDomain();
      const IntervalDomain& y = goalToken->getVariable("y")->lastDomain();
      const IntervalDomain& th = goalToken->getVariable("th")->lastDomain();

      assertTrue(x.isSingleton() && y.isSingleton() && th.isSingleton(), "Values for dispatch are not bound");

      goalMsg.goal.x = x.getSingletonValue();
      goalMsg.goal.y = y.getSingletonValue();
      goalMsg.goal.th= th.getSingletonValue();
    }

  };

  // Allocate a Factory
  TeleoReactor::ConcreteFactory<BaseControllerAdapter> l_BaseControllerAdapter_Factory("BaseControllerAdapter");
}
