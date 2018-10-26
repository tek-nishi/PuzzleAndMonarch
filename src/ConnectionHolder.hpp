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
  void disconnectAll()
  {
    std::for_each(std::begin(connections_), std::end(connections_),
                  [](const auto& it)
                  {
                    it.disconnect();
                  });
  }


public:
  ConnectionHolder() noexcept
  {
    // TIPS:vectorの再確保を減らす
    connections_.reserve(32);
  }

  ~ConnectionHolder()
  {
    disconnectAll();
  }


  void clear() noexcept
  {
    disconnectAll();
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
  

private:
  std::vector<Connection> connections_;
};

}
