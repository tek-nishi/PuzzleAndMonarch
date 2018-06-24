#pragma once

//
// モデルの読み込み＆書き出し
// FIXME Panel専用
//

#include <cinder/DataTarget.h>
#include "PLY.hpp"


namespace ngs { namespace Model {

// バイナリ形式で保存されたTriMeshを読み込む
ci::TriMesh loadTriMesh(const std::string& path)
{
  auto data_ref = ci::loadFile(path);

  // 頂点カラーを含むTriMeshを準備
  ci::TriMesh mesh(ci::TriMesh::Format().positions().normals().colors());
  mesh.read(data_ref);

  return mesh;
}

// TriMeshを書き出す
void writeTriMesh(const std::string& path, const ci::TriMesh& mesh)
{
  auto full_path = getAssetPath(path).replace_extension("mesh");
  auto data_ref  = ci::DataTargetPath::createRef(full_path);
  mesh.write(data_ref);
}


// .meshがダメなら.plyを読む
// FIXME Releaseビルドでは.meshのみ読む
ci::TriMesh load(const std::string& path)
{
  auto full_path = getAssetPath(path);
  auto mesh_path = full_path.replace_extension("mesh");
  if (ci::fs::is_regular_file(mesh_path))
  {
    return loadTriMesh(mesh_path.string());
  }

  return PLY::load(path);
}

} }
