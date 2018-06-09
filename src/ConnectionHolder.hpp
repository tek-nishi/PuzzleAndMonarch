#pragma once

//
// EventのConnectionを安全に切断するUtility
//

#include <vector>
#include <boost/noncopyable.hpp>
#include "Event.hpp"


namespace ngs {

class ConnectionHolder
  : private boost::noncopyable
{
  std::vector<Connection> connections_;


public:
  ConnectionHolder() noexcept
  {
    // TIPS:vectorの再確保を減らす
    connections_.reserve(32);
  }

  ~ConnectionHolder()
  {
    for (auto& connection : connections_)
    {
      connection.disconnect();
    }
  }


  void clear() noexcept
  {
    for (auto& connection : connections_)
    {
      connection.disconnect();
    }
    
    connections_.clear();
  }

  
  void operator += (Connection& connection) noexcept
  {
    connections_.push_back(connection);
  }

  void operator += (Connection&& connection) noexcept
  {
    connections_.push_back(connection);
  }
  
};

}
