#ifndef NAMELOOKUPCLIENT_HH
#define NAMELOOKUPCLIENT_HH

#include <ros/node.h>
#include <ros/time.h>
#include <namelookup/NameToNumber.h>
#include <iostream>
#include <map>

class nameLookupClient 
{
public:
  nameLookupClient(ros::node &aNode): myNode(aNode)
  {
    pthread_mutex_init(&protect_call, NULL);
  };

  virtual ~nameLookupClient(void)
  {
    pthread_mutex_destroy(&protect_call);
  };

  int lookup(const std::string& str_in)
  {
    std::map<std::string, int>::iterator it = nameMap.find(str_in);
    if ( it == nameMap.end())
      {
        
        int value = 0;
        value = lookupOnServer(str_in);
        while (value == 0 && myNode.ok())
          {
            printf("Waiting for namelookup_server\n");
	    ros::Duration(0.5).sleep();//wait for service to come up
            value = lookupOnServer(str_in);
          };
        nameMap[str_in] = value;
        return value;
      }
    else
      {
        return (*it).second;
      }
  }  
    
  
private:

  ros::node &myNode;
  std::map<std::string, int> nameMap;  
  pthread_mutex_t protect_call;

  int lookupOnServer(const std::string &str_in)
  {
    namelookup::NameToNumber::request req;
    namelookup::NameToNumber::response res;
    req.name = myNode.map_name(str_in);
    
    int result = 0;
    pthread_mutex_lock(&protect_call);   
    if (ros::service::call("/nameToNumber", req, res))
  	  result = res.number;
  
    pthread_mutex_unlock(&protect_call);   
    return result;
  }
  
};



#endif //NAMELOOKUPCLIENT_HH
