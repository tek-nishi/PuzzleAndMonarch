//
// 各種ソース
// FIXME インクルード順序が依存している
//

#include "Defines.hpp"


#define NGS_PATH_IMPLEMENTATION
#include "Path.hpp"
#undef  NGS_PATH_IMPLEMENTATION

#define NGS_ASSET_IMPLEMENTATION
#include "Asset.hpp"
#undef  NGS_ASSET_IMPLEMENTATION

#define NGS_FONT_IMPLEMENTATION
#include "Font.hpp"
#undef  NGS_FONT_IMPLEMENTATION

#define NGS_PLY_IMPLEMENTATION
#include "PLY.hpp"
#undef  NGS_PLY_IMPLEMENTATION

#define NGS_DEBUG_IMPLEMENTATION
#include "Debug.hpp"
#undef  NGS_DEBUG_IMPLEMENTATION

#define NGS_SHADER_IMPLEMENTATION
#include "Shader.hpp"
#undef  NGS_SHADER_IMPLEMENTATION

#define NGS_EASEFUNC_IMPLEMENTATION
#include "EaseFunc.hpp"
#undef  NGS_EASEFUNC_IMPLEMENTATION

#define NGS_GL_IMPLEMENTATION
#include "gl.hpp"
#undef  NGS_GL_IMPLEMENTATION
