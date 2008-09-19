#ifndef H_ROSAdapter
#define H_ROSAdapter

#include "Adapter.hh"
#include "ROSNode.hh"

namespace TREX {

  class ROSAdapter: public Adapter {
  public:
    ROSAdapter(const LabelStr& agentName, const TiXmlElement& configData, TICK lookAhead);

    virtual ~ROSAdapter();

    void handleInit(TICK initialTick, const std::map<double, ServerId>& serversByTimeline, const ObserverId& observer);

    void handleNextTick();

    bool synchronize();

  private:
    bool isInitialized() const;

    bool m_initialized; /*!< Hook to flag if expected ros initialization messages have arrived */

    void handleRequest(const TokenId& goal);

    void handleRecall(const TokenId& goal);

    std::vector<std::string> nddlNames_;
    std::vector<std::string> rosNames_;

  protected:
    static unsigned int QUEUE_MAX(){return 10;}
    virtual void handleCallback();
    virtual void registerSubscribers() {}
    virtual void registerPublishers() {}
    virtual Observation* getObservation() = 0;
    virtual void dispatchRequest(const TokenId& goal, bool enabled){}

    const std::vector<std::string>& nddlNames() const {return nddlNames_;}
    const std::vector<std::string>& rosNames() const {return rosNames_;}

    /**
     * @brief Helper methiod to obtain an index for a given ros name.
     * @param The rosName to look for
     * @param The index to fill if found
     * @return True if found, else false
     */
    bool rosIndex(const std::string& rosName, unsigned int& ind) const;

    ROSNodeId m_node; /*! The ROS node. */
    const std::string timelineName;
    const std::string timelineType;
    const std::string stateTopic;
  };
}
#endif
