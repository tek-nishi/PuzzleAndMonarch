//
// セーフエリアが存在するか調べる
//

#include "Defines.hpp"
#include <CoreGraphics/CoreGraphics.h>
#include <cinder/app/Window.h>
#include "SafeArea.h"


namespace ngs { namespace SafeArea {

// セーフエリアがあるか調べる
bool check()
{
  auto* view = (UIView*)ci::app::getWindow()->getNative();

	UIEdgeInsets insets = UIEdgeInsetsMake(0, 0, 0, 0);
	if ([view respondsToSelector: @selector(safeAreaInsets)])
  {
    insets = [view safeAreaInsets];
  }

  return (insets.top    > 0)
      || (insets.bottom > 0)
      || (insets.left   > 0)
      || (insets.right  > 0);
}

} }
