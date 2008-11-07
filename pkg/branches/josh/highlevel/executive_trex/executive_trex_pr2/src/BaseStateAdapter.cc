#include "ROSStateAdapter.hh"
#include "IntervalDomain.hh"
#include <std_msgs/RobotBase2DOdom.h>

namespace TREX {

  class BaseStateAdapter: public ROSStateAdapter<std_msgs::RobotBase2DOdom> {
  public:
    BaseStateAdapter(const LabelStr& agentName, const TiXmlElement& configData)
      : ROSStateAdapter<std_msgs::RobotBase2DOdom> ( agentName, configData) {
    }

    virtual ~BaseStateAdapter(){}

  private:
    void fillObservationParameters(ObservationByValue* obs){
      obs->push_back("x", new IntervalDomain(stateMsg.pos.x));
      obs->push_back("y", new IntervalDomain(stateMsg.pos.y));
      obs->push_back("th",new IntervalDomain(stateMsg.pos.th));
    }
  };

  // Allocate a Factory
  TeleoReactor::ConcreteFactory<BaseStateAdapter> l_BaseStateAdapter_Factory("BaseStateAdapter");
}
