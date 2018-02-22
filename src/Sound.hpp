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

  void stopBgm() noexcept
  {
  }

  void seopSe() noexcept
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

    std::map<std::string, std::function<ci::audio::SamplePlayerNodeRef (ci::audio::Context* ctx, const ci::audio::SourceFileRef&)>> funcs {
      { "bgm",
        [](ci::audio::Context* ctx, const ci::audio::SourceFileRef& source) noexcept
        {
          return ctx->makeNode(new ci::audio::FilePlayerNode(source));
        }},
      { "se",
        [](ci::audio::Context* ctx, const ci::audio::SourceFileRef& source) noexcept
        {
          auto buffer = source->loadBuffer();
          return ctx->makeNode(new ci::audio::BufferPlayerNode(buffer));
        }},
    };



    for (const auto& p : params)
    {
      const auto& path = p.getValueForKey<std::string>("path");
      auto source = ci::audio::load(Asset::load(path));
      
      const auto& type = p.getValueForKey<std::string>("type");
      auto node = funcs.at(type)(ctx, source);
      node >> ctx->getOutput();
      
      const auto& name = p.getValueForKey<std::string>("name");
      details_.emplace(name, node);

      DOUT << "Sound: " << name << " path: " << path << std::endl;
    }

    ctx->enable();

    holder_ += event.connect("UI:sound",
                             [this](const Connection&, const Arguments& args) noexcept
                             {
                               const auto& name = boost::any_cast<const std::string&>(args.at("name"));
                               play(name);
                             });

    holder_ += event.connect("SE:timeline",
                             [this](const Connection&, const Arguments& args) noexcept
                             {
                               count_exec_.clear();

                               const auto& params = boost::any_cast<const ci::JsonTree&>(args.at("timeline"));
                               for (const auto& p : params)
                               {
                                 auto delay = p.getValueAtIndex<double>(0);
                                 const auto& name = p.getValueAtIndex<std::string>(1);
                                 count_exec_.add(delay,
                                                 [this, name]() noexcept
                                                 {
                                                   play(name);
                                                 });
                               }
                             });
  }

  ~Sound() = default;
};

}
