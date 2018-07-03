#pragma once

//
// PLY読み込み
// 
// FIXME:MagicaVoxelから書き出したデータにのみ対応しているので
//       かなり大雑把
//

#include "Defines.hpp"
#include "Path.hpp"
#include <fstream>
#include <sstream> 
#include <vector> 
#include <glm/gtc/random.hpp>


namespace ngs { namespace PLY {

ci::TriMesh load(const std::string& path, bool do_optimize = false);
ci::TriMesh optimize(const ci::TriMesh& mesh);

#if defined (NGS_PLY_IMPLEMENTATION)

std::vector<std::string> split(const std::string& text) noexcept
{
  std::istringstream line_separater(text);
  const char delimiter = ' ';

  std::vector<std::string> split_text;
  while (!line_separater.eof())
  {
    std::string separated_string;
    std::getline(line_separater, separated_string, delimiter);
    split_text.push_back(separated_string);
  }

  return split_text;
}


// ファイルから読み込んでstringstreamにする
std::istringstream createStringStream(const std::string& path)
{
  std::ifstream ifs(getAssetPath(path).string());
  assert(ifs);

  // ファイルから一気に読み込む
  std::string str((std::istreambuf_iterator<char>(ifs)),
                  std::istreambuf_iterator<char>());

  return std::istringstream(str);
}


ci::TriMesh load(const std::string& path, bool do_optimize)
{
  auto iss = createStringStream(path);

  // 頂点カラーを含むTriMeshを準備
  ci::TriMesh mesh(ci::TriMesh::Format().positions().normals().colors());

  int vertex_num = 0;
  int face_num = 0;

  // ヘッダ解析
  while (!iss.eof())
  {
    // １行読み込む
    std::string line_buffer;
    std::getline(iss, line_buffer);

    auto split_text = split(line_buffer);
    if (split_text[0] == "element" && split_text[1] == "vertex")
    {
      // 頂点数
      vertex_num = std::stoi(split_text[2]);
    }
    else if (split_text[0] == "element" && split_text[1] == "face")
    {
      // ポリゴン数
      face_num = std::stoi(split_text[2]);
    }
    else if (split_text[0] == "end_header")
    {
      // ヘッダ終了
      break;
    }
  }

  assert((vertex_num != 0) && (face_num != 0));
  
  for (int i = 0; i < vertex_num; ++i)
  {
    std::string line_buffer;
    std::getline(iss, line_buffer);
    auto split_text = split(line_buffer);

    glm::vec3 p(std::stof(split_text[0]), std::stof(split_text[1]), std::stof(split_text[2]));
    mesh.appendPosition(p);

    ci::Color c(std::stof(split_text[3]) / 255.0f, std::stof(split_text[4]) / 255.0f, std::stof(split_text[5]) / 255.0f);
    mesh.appendColorRgb(c);
  }
  
  for (int i = 0; i < face_num; ++i)
  {
    std::string line_buffer;
    std::getline(iss, line_buffer);
    auto split_text = split(line_buffer);

    switch (std::stoi(split_text[0]))
    {
    case 3:
      {
        uint32_t v0 = uint32_t(std::stoul(split_text[1]));
        uint32_t v1 = uint32_t(std::stoul(split_text[2]));
        uint32_t v2 = uint32_t(std::stoul(split_text[3]));
        mesh.appendTriangle(v0, v1, v2);
      }
      break;

    case 4:
      {
        uint32_t v0 = uint32_t(std::stoul(split_text[1]));
        uint32_t v1 = uint32_t(std::stoul(split_text[2]));
        uint32_t v2 = uint32_t(std::stoul(split_text[3]));
        uint32_t v3 = uint32_t(std::stoul(split_text[4]));
        mesh.appendTriangle(v0, v1, v2);
        mesh.appendTriangle(v0, v2, v3);
      }
      break;
    }
  }

  mesh.recalculateNormals();

  // DOUT << path << '\n'
  //      << "  face: " << face_num << '\n'
  //      << "vertex: " << vertex_num << '\n'
  //      << std::endl;

  return do_optimize ? optimize(mesh)
                     : mesh;
}


void displaceNormals(ci::TriMesh& mesh)
{
  auto& normals = mesh.getNormals();
  for (auto& n : normals)
  {
    auto v = glm::sphericalRand(1.0f);
    auto r = glm::linearRand(-0.05f, 0.05f);
    auto q = glm::angleAxis(r, v);
    auto nn = q * glm::vec4(n, 1);
    n = nn;
  }
}


// 同じ頂点を削除する
ci::TriMesh optimize(const ci::TriMesh& mesh)
{ 
  // 頂点カラーを含むTriMeshを準備
  ci::TriMesh opt_mesh(ci::TriMesh::Format().positions().normals().colors());

  // 頂点索引変換情報
  std::map<uint32_t, uint32_t> indices;

  // 全頂点情報を取り出す
  const auto* pos    = mesh.getPositions<3>();
  const auto& color  = mesh.getColors<3>();
  const auto& normal = mesh.getNormals();

  uint32_t vertex_num = uint32_t(mesh.getNumVertices());
  uint32_t index = 0;
  for (uint32_t i = 0; i < vertex_num; ++i)
  {
    for (uint32_t j = 0; j < i; ++j)
    {
      if (pos[j] == pos[i]
          && color[j] == color[i]
          && normal[j] == normal[i])
      {
        // 同じ頂点が見つかった
        indices.insert({ i, indices.at(j) });
        goto NEXT;
      }
    }

    // 見つからなかった
    opt_mesh.appendPosition(pos[i]);
    opt_mesh.appendColorRgb(color[i]);
    opt_mesh.appendNormal(normal[i]);
    indices.insert({ i, index });
    ++index;
  NEXT:
    ;
  }

  // 再マップ
  const auto& mesh_indices = mesh.getIndices();
  std::vector<uint32_t> opt_indices(mesh_indices.size());
  for (size_t i = 0; i < mesh_indices.size(); ++i)
  {
    opt_indices[i] = indices.at(mesh_indices[i]);
  }
  opt_mesh.getIndices() = opt_indices;

  DOUT << "vtx: " << mesh.getNumVertices()
       << " -> " << opt_mesh.getNumVertices()
       << " (" << float(opt_mesh.getNumVertices()) / float(mesh.getNumVertices()) * 100.0f << "%)"
       << std::endl;

  displaceNormals(opt_mesh);

  return opt_mesh;
}

#endif

} }
