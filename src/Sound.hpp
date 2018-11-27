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
  struct Detail
  {
    std::string category;

    std::function<void ()> play;
    std::function<void ()> stop;
  };

  
  bool update(double current_time, double delta_time) noexcept override
  {
    count_exec_.update(delta_time);

    return active_;
  }


  template <typename T>
  void stopAll(const T& array)
  {
    for (const auto& it : array)
    {
      it.second->stop();
    }
  }

  void stopAll() noexcept
  {
    stopAll(bgm_nodes_);
    stopAll(se_nodes_);
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
    DOUT << "active nodes:" << output->getNumConnectedInputs() << std::endl;

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
      bgm_nodes_.insert({ slot, node });

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
                    auto* ctx = ci::audio::Context::master();
                    node->disconnect(ctx->getOutput());
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
      se_nodes_.insert({ slot, node });

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
                    auto* ctx = ci::audio::Context::master();
                    node->disconnect(ctx->getOutput());
                  }
                };

    return { type, play, stop };
  }


  // Game内サウンドリスト読み込み
  void createEventSound(const ci::JsonTree& params)
  {
    using namespace std::literals;

    const auto& pp = params["game-sound"s];
    for (const auto& p : pp)
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
                   auto i = ci::randInt(int(list.size()));
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

#if defined (DEBUG)

  void setEnabled(bool enable)
  {
    auto* ctx = ci::audio::Context::master();
    ctx->setEnabled(enable);

    DOUT << "Sound: " << enable << std::endl;
  }

  bool isEnabled()
  {
    auto* ctx = ci::audio::Context::master();
    return ctx->isEnabled();
  }

#endif


public:
  Sound(const ci::JsonTree& params, Event<Arguments>& event) noexcept
    : event_(event)
  {
    using namespace std::literals;

    // TIPS iOS:ヘッドホンプラグの抜き差しに対応
    AudioSession::begin();

    auto* ctx = ci::audio::Context::master();
    ctx->getOutput()->enableClipDetection(false);

    ci::audio::Node::Format format;
    format.channelMode(ci::audio::Node::ChannelMode::SPECIFIED);

    // カテゴリ別のNode生成
    const std::map<std::string,
                   std::function<Detail (const std::string&, const std::string&,
                                         ci::audio::Context*, const ci::audio::SourceFileRef&)>> funcs{
      { "bgm"s,
        [this, format](const std::string& type, const std::string& slot,
                 ci::audio::Context* ctx, const ci::audio::SourceFileRef& source) noexcept
        {
          return setupBgm(type, slot, ctx, source, format);
        }},
      { "se"s,
        [this, format](const std::string& type, const std::string& slot,
                 ci::audio::Context* ctx, const ci::audio::SourceFileRef& source) noexcept
        {
          return setupSe(type, slot, ctx, source, format);
        }},
    };

    const auto& pp = params["sound"s];
    for (const auto& p : pp)
    {
      const auto& path = p.getValueForKey<std::string>("path"s);
      auto source = ci::audio::load(Asset::load(path), ctx->getSampleRate());

      const auto& type = p.getValueForKey<std::string>("type"s);
      auto slot        = Json::getValue(p, "slot"s, type);
      auto detail = funcs.at(type)(type, slot, ctx, source);

      const auto& name = p.getValueForKey<std::string>("name"s);
      details_.insert({ name, detail });

      DOUT << "Sound: " << name
           << " type: " << type
           << " slot: " << slot
           << " path: " << path
           << std::endl;
    }

    createEventSound(params);

    holder_ += event.connect("Settings:Changed"s,
                             [this](const Connection&, const Arguments& args) noexcept
                             {
                               // ON/OFF
                               auto bgm_enable = getValue<bool>(args, "bgm-enable"s);
                               enableCategory("bgm"s, bgm_enable);

                               auto se_enable  = getValue<bool>(args, "se-enable"s);
                               enableCategory("se"s, se_enable);

                               DOUT << "bgm: " << bgm_enable
                                    << " se: " << se_enable
                                    << std::endl;
                             });

    holder_ += event.connect("UI:sound"s,
                             [this](const Connection&, const Arguments& args) noexcept
                             {
                               const auto& name = boost::any_cast<const std::string&>(args.at("name"s));
                               play(name);
                             });

    holder_ += event.connect("SE:timeline"s,
                             [this](const Connection&, const Arguments& args) noexcept
                             {
                               count_exec_.clear();

                               const auto& params = boost::any_cast<const ci::JsonTree&>(args.at("timeline"s));
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

    holder_ += event.connect("Game:Event"s,
                             [this](const Connection&, const Arguments& args) noexcept
                             {
                               const auto& events = boost::any_cast<const std::set<std::string>&>(args.at("event"s));
                               
                               if (events.empty()) return;

                               for (const auto& e : events)
                               {
                                 if (game_sound_.count(e))
                                 {
                                   // TIPS 関数ポインタを使っている
                                   game_sound_.at(e)();
                                 }
#if defined (DEBUG)
                                 else
                                 {
                                   DOUT << "no sound event: " << e << std::endl;
                                 }
#endif
                               }
                             });

#if defined (DEBUG)
    holder_ += event.connect("debug-sound"s,
                             [this](const Connection&, const Arguments&)
                             {
                               setEnabled(!isEnabled()); 
                             });
#endif

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


private:
  Event<Arguments>& event_;
  ConnectionHolder holder_;

  CountExec count_exec_;

  bool active_ = true;

  std::map<std::string, Detail> details_;

  // BGMとSE別再生Node
  std::map<std::string, ci::audio::FilePlayerNodeRef> bgm_nodes_;
  std::map<std::string, ci::audio::BufferPlayerNodeRef> se_nodes_;

  // BGMとSEのカテゴリ別ON-OFF用途 
  std::map<std::string, std::vector<ci::audio::SamplePlayerNodeRef>> category_;
  std::map<std::string, bool> enable_category_;

  // Game内サウンド用
  std::map<std::string, std::function<void ()>> game_sound_;

};

}
