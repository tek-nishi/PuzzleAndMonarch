#pragma once

//
// boost::signals2を利用した汎用的なイベント
//

#include <boost/signals2.hpp>
#include <boost/noncopyable.hpp>
#include <map>


namespace ngs {

using Connection = boost::signals2::connection;

template <typename... Args>
class Event
  : private boost::noncopyable
{
  using SignalType = boost::signals2::signal<void(Args&...)>;

  std::map<std::string, SignalType> signals_;


public:
  Event()  = default;
  ~Event() = default;


  template<typename F>
  Connection connect(const std::string& msg, F callback) noexcept
  {
    return signals_[msg].connect_extended(callback);
  }  

  
  template <typename... Args2>
  void signal(const std::string& msg, Args2&&... args) noexcept
  {
    signals_[msg](args...);
  }

  
private:
  
};

}
