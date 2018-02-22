#pragma once

//
// サウンド管理
//

#include <cinder/audio/audio.h>
#include "Task.hpp"
#include "Asset.hpp"
#include "CountExec.hpp"


namespace ngs {

class Sound
  : public Task
{
  Event<Arguments>& event_;
  ConnectionHolder holder_;

  CountExec count_exec_;

  bool bgm_ = true;
  bool se_  = true;

  bool active_ = true;

  // struct Detail {
  //   ci::audio::SamplePlayerNodeRef node;
  // };

  std::map<std::string, ci::audio::SamplePlayerNodeRef> details_;


  
  bool update(double current_time, double delta_time) noexcept override
  {
    count_exec_.update(delta_time);

    return active_;
  }


  void stopAll() noexcept
  {
  }

  void play(const std::string& name) noexcept
  {
    if (!details_.count(name))
    {
      DOUT << "Sound::play No sound: " << name << std::endl;
      return;
    }

    details_.at(name)->start();
  }

  void stop(const std::string& name) noexcept
  {
    if (!details_.count(name))
    {
      DOUT << "Sound::stop No sound: " << name << std::endl;
      return;
    }

    details_.at(name)->stop();
  }


public:
  Sound(const ci::JsonTree& params, Event<Arguments>& event) noexcept
    : event_(event)
  {
    auto ctx = ci::audio::Context::master();

    for (const auto& p : params)
    {
      const auto& name = p.getValueForKey<std::string>("name");
      const auto& path = p.getValueForKey<std::string>("path");

      auto source = ci::audio::load(Asset::load(path));
      auto buffer = source->loadBuffer();

      auto node = ctx->makeNode(new ci::audio::BufferPlayerNode(buffer));
      details_.emplace(name, node);
      node >> ctx->getOutput();

      DOUT << "Sound: " << name << " path: " << path << std::endl;
    }

    ctx->enable();

    holder_ += event.connect("UI:sound",
                             [this](const Connection&, const Arguments& args) noexcept
                             {
                               const auto& name = boost::any_cast<const std::string&>(args.at("name"));
                               play(name);
                             });
  }

  ~Sound() = default;
};

}
