#pragma once

//
// アプリ内課金処理繋ぎ
//

#include <string>
#include <functional>


namespace ngs { namespace PurchaseDelegate {

#if defined(CINDER_COCOA_TOUCH)

    void init(const std::function<void ()>& purchase_completed, const std::function<void ()>& restore_completed);

    void start(const std::string& product_id);
    void restore(const std::string& product_id);
    
    void price(const std::string& product_id, const std::function<void (const std::string)>& completed);
    bool hasPrice();

#else

    template <typename T1, typename T2>
    void init(T1, T2) {}

    template <typename T>
    void start(T) {}
    template <typename T>
    void restore(T) {}

    template <typename T1, typename T2>
    void price(T1, T2) {}
    template <typename T = void>
    bool hasPrice() { return false; }

#endif

} }
