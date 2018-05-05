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
#include "PackedFile.hpp"


namespace ngs { namespace PLY {

ci::TriMesh load(const std::string& path) noexcept;


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

ci::TriMesh load(const std::string& path) noexcept
{
#if defined (USE_PACKED_FILE)
  std::istringstream ifs(PackedFile::readString(path));
#else
  std::ifstream ifs(getAssetPath(path).string());
#endif
  assert(ifs);

  // 頂点カラーを含むTriMeshを準備
  ci::TriMesh mesh(ci::TriMesh::Format().positions().normals().colors());

  int vertex_num = 0;
  int face_num = 0;

  // ヘッダ解析
  while (!ifs.eof())
  {
    // １行読み込む
    std::string line_buffer;
    std::getline(ifs, line_buffer);

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
    std::getline(ifs, line_buffer);
    auto split_text = split(line_buffer);

    glm::vec3 p(std::stof(split_text[0]), std::stof(split_text[1]), std::stof(split_text[2]));
    mesh.appendPosition(p);

    ci::Color c(std::stof(split_text[3]) / 255.0f, std::stof(split_text[4]) / 255.0f, std::stof(split_text[5]) / 255.0f);
    mesh.appendColorRgb(c);
  }
  
  for (int i = 0; i < face_num; ++i)
  {
    std::string line_buffer;
    std::getline(ifs, line_buffer);
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
  
  DOUT << path << '\n'
       << "  face: " << face_num << '\n'
       << "vertex: " << vertex_num << '\n'
       << std::endl;

  return mesh;
}

#endif

} }
