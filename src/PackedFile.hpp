#pragma once

//
// １つにまとめられたファイルから読み込む
//

#include <boost/noncopyable.hpp>
#include <vector>
#include <map>


class PackedFile
  : private boost::noncopyable
{
  struct Body
  {
    uint32_t offset;
    uint32_t size;
  };


public:
  PackedFile(const std::string& file_path)
    : fstr_(file_path, std::ios::binary)
  {
    // ファイル数
    uint32_t file_num;
    fstr_.read((char*)&file_num, sizeof(file_num));

    DOUT << file_num << " files included." << std::endl;

    // ヘッダー解析
    for (uint32_t i = 0; i < file_num; ++i)
    {
      uint32_t path_size;
      fstr_.read((char*)&path_size, sizeof(path_size));

      std::vector<char> path(path_size);
      fstr_.read(&path[0], path_size);

      uint32_t offset;
      fstr_.read((char*)&offset, sizeof(offset));

      uint32_t size;
      fstr_.read((char*)&size, sizeof(size));

      Body body{ offset, size };
      std::string filename{ std::begin(path), std::end(path) };
      info_.insert({ filename, body });

      DOUT << filename << std::endl;
    }
  }

  ~PackedFile() = default;


  std::vector<char> read(const std::string& path)
  {
    if (!info_.count(path))
    {
      DOUT << "File not found: " << path << std::endl;
      return std::vector<char>();
    }

    const auto& body = info_.at(path);
    fstr_.seekg(body.offset);

    std::vector<char> input(body.size);
    fstr_.read(&input[0], input.size());

    return input;
  }


private:
  // TIPS ファイルは開きっぱなし
  std::ifstream fstr_;
  std::map<std::string, Body> info_;
};
