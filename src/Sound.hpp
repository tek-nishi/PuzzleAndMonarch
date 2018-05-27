#pragma once

//
// サウンド管理
//

#include <cinder/audio/audio.h>
#include "Task.hpp"
#include "Asset.hpp"
#include "CountExec.hpp"
#include "AudioSession.h"


namespace ngs {

class Sound
  : public Task
{
  Event<Arguments>& event_;
  ConnectionHolder holder_;

  CountExec count_exec_;

  bool active_ = true;


  struct Detail
  {
    Detail(const std::string& c, const ci::audio::SamplePlayerNodeRef& n) noexcept
      : category(c),
        node(n)
    {}
    ~Detail() = default;

    std::string category;
    ci::audio::SamplePlayerNodeRef node;
  };

  std::map<std::string, Detail> details_;

  // カテゴリ用
  std::map<std::string, std::vector<ci::audio::SamplePlayerNodeRef>> category_;
  std::map<std::string, bool> enable_category_;

  // Game内サウンド用
  std::map<std::string, std::string> game_sound_;


  
  bool update(double current_time, double delta_time) noexcept override
  {
    count_exec_.update(delta_time);

    return active_;
  }


  void stopAll() noexcept
  {
    for (auto& it : details_)
    {
      it.second.node->stop();
    }
  }

  void stopCategory(const std::string& category) noexcept
  {
    if (!category_.count(category))
    {
      DOUT << "Sound::stopCategory No category: " << category << std::endl;
      return;
    }

    const auto& details = category_.at(category);
    for (const auto& node : details)
    {
      node->stop();
    }
  }


  void play(const std::string& name) noexcept
  {
    if (!details_.count(name))
    {
      DOUT << "Sound::play No sound: " << name << std::endl;
      return;
    }

    const auto& detail = details_.at(name);
    if (!enable_category_[detail.category]) return;

    auto ctx = ci::audio::Context::master();
    detail.node >> ctx->getOutput();
    detail.node->start();
  }

  void stop(const std::string& name) noexcept
  {
    if (!details_.count(name))
    {
      DOUT << "Sound::stop No sound: " << name << std::endl;
      return;
    }

    details_.at(name).node->stop();
  }

  
  void enableCategory(const std::string& category, bool enable) noexcept
  {
    enable_category_[category] = enable;
    if (!enable)
    {
      stopCategory(category);
    }
  }



public:
  Sound(const ci::JsonTree& params, Event<Arguments>& event) noexcept
    : event_(event)
  {
    // TIPS iOS:ヘッドホンプラグの抜き差しに対応
    AudioSession::begin();

    auto ctx = ci::audio::Context::master();

    ci::audio::Node::Format format;
    format.channelMode(ci::audio::Node::ChannelMode::SPECIFIED);

    // カテゴリ別のNode生成
    std::map<std::string,
             std::function<ci::audio::SamplePlayerNodeRef (ci::audio::Context* ctx, const ci::audio::SourceFileRef&)>> funcs {
      { "bgm",
        [format](ci::audio::Context* ctx, const ci::audio::SourceFileRef& source) noexcept
        {
          // Streaming
          return ctx->makeNode(new ci::audio::FilePlayerNode(source, true, format));
        }},
      { "se",
        [format](ci::audio::Context* ctx, const ci::audio::SourceFileRef& source) noexcept
        {
          // あらかじめBufferに読み込む
          auto buffer = source->loadBuffer();
          return ctx->makeNode(new ci::audio::BufferPlayerNode(buffer, format));
        }},
    };

    for (const auto& p : params["sound"])
    {
      const auto& path = p.getValueForKey<std::string>("path");
      auto source = ci::audio::load(Asset::load(path), ctx->getSampleRate());

      const auto& type = p.getValueForKey<std::string>("type");
      auto node = funcs.at(type)(ctx, source);

      const auto& name = p.getValueForKey<std::string>("name");
      details_.emplace(std::piecewise_construct,
                       std::forward_as_tuple(name),
                       std::forward_as_tuple(type, node));
      category_[type].push_back(node);
      enable_category_[type] = true;

      DOUT << "Sound: " << name
           << " type: " << type
           << " path: " << path
           << std::endl;
    }

    // Game内サウンドリスト読み込み
    for (const auto& p : params["game-sound"])
    {
      game_sound_.insert({ p.getKey(), p.getValue<std::string>() });
    }


    holder_ += event.connect("Settings:Changed",
                             [this](const Connection&, const Arguments& args) noexcept
                             {
                               // ON/OFF
                               auto bgm_enable = boost::any_cast<bool>(args.at("bgm-enable"));
                               enableCategory("bgm", bgm_enable);

                               auto se_enable  = boost::any_cast<bool>(args.at("se-enable"));
                               enableCategory("se", se_enable);

                               DOUT << "bgm: " << bgm_enable
                                    << " se: " << se_enable
                                    << std::endl;
                             });

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

    holder_ += event.connect("Game:Event",
                             [this](const Connection&, const Arguments& args) noexcept
                             {
                               const auto& events = boost::any_cast<const std::set<std::string>&>(args.at("event"));
                               for (const auto& e : events)
                               {
                                 if (game_sound_.count(e))
                                 {
                                   play(game_sound_.at(e));
                                 }
                               }
                             });

    ctx->enable();
  }

  // ~Sound()
  // {
  //   stopAll();

  //   auto ctx = ci::audio::Context::master();
  //   ctx->disable();

  //   AudioSession::end();

  //   DOUT << "~Sound()" << std::endl;
  // }
};

}
