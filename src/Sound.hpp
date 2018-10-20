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
    std::string category;

    std::function<void ()> play;
    std::function<void ()> stop;
  };
  std::map<std::string, Detail> details_;

  // BGMとSE別再生Node
  std::map<std::string, ci::audio::FilePlayerNodeRef> bgm_nodes_;
  std::map<std::string, ci::audio::BufferPlayerNodeRef> se_nodes_;

  // BGMとSEのカテゴリ別ON-OFF用途 
  std::map<std::string, std::vector<ci::audio::SamplePlayerNodeRef>> category_;
  std::map<std::string, bool> enable_category_;

  // Game内サウンド用
  std::map<std::string, std::function<void ()>> game_sound_;

  
  bool update(double current_time, double delta_time) noexcept override
  {
    count_exec_.update(delta_time);

    return active_;
  }


  void stopAll() noexcept
  {
    for (auto it : bgm_nodes_)
    {
      it.second->stop();
    }
    for (auto it : se_nodes_)
    {
      it.second->stop();
    }

    disconnectInactiveNode();
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

    disconnectInactiveNode();
  }


  void play(const std::string& name) noexcept
  {
    disconnectInactiveNode();

    if (!details_.count(name))
    {
      DOUT << "Sound::play No sound: " << name << std::endl;
      return;
    }

    const auto& detail = details_.at(name);

    if (!enable_category_.at(detail.category)) return;
    detail.play();
  }

  void stop(const std::string& name) noexcept
  {
    if (!details_.count(name))
    {
      DOUT << "Sound::stop No sound: " << name << std::endl;
      return;
    }

    const auto& detail = details_.at(name);
    detail.stop();

    disconnectInactiveNode();
  }

  
  void enableCategory(const std::string& category, bool enable) noexcept
  {
    enable_category_.at(category) = enable;
    if (!enable)
    {
      stopCategory(category);
    }
  }

  // FIXME:iOSではNodeをOutputにたくさん繋げると、音量が小さくなる
  static void disconnectInactiveNode()
  {
    const auto& output = ci::audio::Context::master()->getOutput();
    // DOUT << "active nodes:" << output->getNumConnectedInputs() << std::endl;

    // NOTICE disconnectするとイテレーターが無効になるので意図的にコピーを受け取っている
    auto nodes = output->getInputs();
    for (const auto& node : nodes)
    {
      if (!node->isEnabled()) node->disconnect(output);
    }
  }


  // BGM向けセットアップ
  Detail setupBgm(const std::string& type, const std::string& slot,
                  ci::audio::Context* ctx, const ci::audio::SourceFileRef& source, const ci::audio::Node::Format& format)
  {
    if (!bgm_nodes_.count(slot))
    {
      auto node = ctx->makeNode(new ci::audio::FilePlayerNode(format));
      bgm_nodes_.insert({ slot, node});

      category_[type].push_back(node);
      enable_category_[type] = true;
    }

    auto node = bgm_nodes_.at(slot);

    // 再生と停止
    auto play = [node, source]()
                {
                  if (node->isEnabled())
                  {
                    node->stop();
                  }
                  auto* ctx = ci::audio::Context::master();

                  node->setSourceFile(source);

                  node >> ctx->getOutput();
                  node->start();
                };

    auto stop = [node]()
                {
                  if (node->isEnabled())
                  {
                    node->stop();
                  }
                };

    return { type, play, stop };
  }

  // SE向けセットアップ
  Detail setupSe(const std::string& type, const std::string& slot,
                 ci::audio::Context* ctx, const ci::audio::SourceFileRef& source, const ci::audio::Node::Format& format)
  {
    if (!se_nodes_.count(slot))
    {
      auto node = ctx->makeNode(new ci::audio::BufferPlayerNode(format));
      se_nodes_.insert({ slot, node});

      category_[type].push_back(node);
      enable_category_[type] = true;
    }

    auto node   = se_nodes_.at(slot);
    auto buffer = source->loadBuffer();

    auto play = [node, buffer]()
                {
                  if (node->isEnabled())
                  {
                    node->stop();
                  }

                  auto* ctx = ci::audio::Context::master();

                  node->setBuffer(buffer);
          
                  node >> ctx->getOutput();
                  node->start();
                };

    auto stop = [node]()
                {
                  if (node->isEnabled())
                  {
                    node->stop();
                  }
                };

    return { type, play, stop };
  }


  // Game内サウンドリスト読み込み
  void createEventSound(const ci::JsonTree& params)
  {
    for (const auto& p : params["game-sound"])
    {
      std::function<void ()> func;

      if (p.hasChildren())
      {
        std::vector<std::string> list;
        // 最初のbool値で再生開始なのか、再生中断なのかを決める
        auto f = p[0].getValue<bool>();
        if (f)
        {
          // ランダムに選んで再生
          for (int i = 1; i < p.getNumChildren(); ++i)
          {
            list.push_back(p[i].getValue<std::string>());
          }

          func = [this, list]()
                 {
                   auto i = ci::randInt(list.size());
                   play(list[i]);
                 };
        }
        else
        {
          // 再生停止
          auto name = p[1].getValue<std::string>();
          func = [this, name]()
                 {
                   DOUT << ci::app::getElapsedFrames() << ": sound stop: " << name << std::endl;
                   stop(name);
                 };
        }
      }
      else
      {
        const auto& name = p.getValue<std::string>();
        func = [this, name]()
               {
                 DOUT << ci::app::getElapsedFrames() << ": sound play: " << name << std::endl;
                 play(name);
               };
      }

      game_sound_.insert({ p.getKey(), func });
    }
  }


public:
  Sound(const ci::JsonTree& params, Event<Arguments>& event) noexcept
    : event_(event)
  {
    // TIPS iOS:ヘッドホンプラグの抜き差しに対応
    AudioSession::begin();

    auto* ctx = ci::audio::Context::master();
    ctx->getOutput()->enableClipDetection(false);

    ci::audio::Node::Format format;
    format.channelMode(ci::audio::Node::ChannelMode::SPECIFIED);

    // カテゴリ別のNode生成
    std::map<std::string,
             std::function<Detail (const std::string&, const std::string&,
                                   ci::audio::Context*, const ci::audio::SourceFileRef&)>> funcs {
      { "bgm",
        [this, format](const std::string& type, const std::string& slot,
                 ci::audio::Context* ctx, const ci::audio::SourceFileRef& source) noexcept
        {
          return setupBgm(type, slot, ctx, source, format);
        }},
      { "se",
        [this, format](const std::string& type, const std::string& slot,
                 ci::audio::Context* ctx, const ci::audio::SourceFileRef& source) noexcept
        {
          return setupSe(type, slot, ctx, source, format);
        }},
    };

    for (const auto& p : params["sound"])
    {
      const auto& path = p.getValueForKey<std::string>("path");
      auto source = ci::audio::load(Asset::load(path), ctx->getSampleRate());

      const auto& type = p.getValueForKey<std::string>("type");
      auto slot        = Json::getValue(p, "slot", type);
      auto detail = funcs.at(type)(type, slot, ctx, source);

      const auto& name = p.getValueForKey<std::string>("name");
      details_.insert({ name, detail });

      DOUT << "Sound: " << name
           << " type: " << type
           << " slot: " << slot
           << " path: " << path
           << std::endl;
    }

    createEventSound(params);

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
                                   // TIPS 関数ポインタを使っている
                                   game_sound_.at(e)();
                                 }
#if !defined (DEBUG)
                                 else
                                 {
                                   DOUT << "no event: " << e << std::endl;
                                 }
#endif
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
