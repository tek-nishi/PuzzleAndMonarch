//
// ファイルを１つにまとめるやつ
//   圧縮はしません
//

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <cassert>
#include <boost/filesystem.hpp>


bool isHidden(const boost::filesystem::path& p)
{
  auto name = p.filename();
  if(name != ".." &&
     name != "."  &&
     name.string()[0] == '.')
  {
    return true;
  }

  return false;
}

// Packするファイルか??
//  FIXME ハードコーディング
bool isPack(const boost::filesystem::path& p)
{
  auto name = p.extension();
  if (name == ".m4a"
      || name == ".png"
      || name == ".data")
  {
    return false;
  }
  
  return true;
}


// FIXME ファイル上限4GB
struct File
{
  boost::filesystem::path path;
  // 先頭からのオフセット
  uint32_t offset;
  // データサイズ
  uint32_t size;
};

// ファイルに書き出す時は
//
//   header size 4
//
//   path size  4
//   path       可変
//   offset     4
//   size       4
//
//   data
//
// とする

uint32_t calcHeaderSize(const std::vector<File>& files)
{
  uint32_t size = sizeof(uint32_t);
  for (const auto& f : files)
  {
    size += sizeof(uint32_t) * 3;
    size += f.path.filename().size();
  }

  return size;
}

//
// 実装
//

struct PackedFile
{
  struct Body
  {
    uint32_t offset;
    uint32_t size;
  };


  PackedFile(const std::string& path)
    : fstr_(path, std::ios::binary)
  {
    // ファイル数
    uint32_t file_num;
    fstr_.read((char*)&file_num, sizeof(file_num));

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

      std::cout << filename << std::endl;
    }
  }

  std::vector<char> read(const std::string& path)
  {
    if (!info_.count(path))
    {
      std::cout << "File not found: " << path << std::endl;
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


int main(int argc, char* argv[])
{
  boost::filesystem::path p(argv[1]);

  std::vector<File> files;

  // ファイル情報収拾
  for (boost::filesystem::directory_entry& x : boost::filesystem::directory_iterator(p))
  {
    if (!isHidden(x.path()) && isPack(x.path()))
    {
      files.push_back({
          x.path(),
          0,
          uint32_t(boost::filesystem::file_size(x.path())) });

      std::cout << x.path().filename() << std::endl;
    }
  }
  std::cout << files.size() << " files." << std::endl;

  // ヘッダ情報確定
  auto header_size = calcHeaderSize(files);
  auto offset = header_size;
  for (auto& f : files)
  {
    f.offset = offset;
    offset += f.size;
  }


  {
    std::fstream file(argv[2], std::ios::binary | std::ios::out);

    // ヘッダ書き出し
    uint32_t num = files.size();
    file.write((char*)&num, sizeof(num));
    for (const auto& f : files)
    {
      uint32_t size = f.path.filename().size();
      file.write((char*)&size, sizeof(size));
      file.write(f.path.filename().string().c_str(), size);

      file.write((char*)&f.offset, sizeof(f.offset));
      file.write((char*)&f.size, sizeof(f.size));
    }

    // ファイル結合
    for (const auto& f : files)
    {
      std::ifstream fstr(f.path.string(), std::ios::binary);
      if (!fstr.is_open())
      {
        std::cout << "File open error:" << f.path << std::endl;
        return 0;
      }

      std::vector<char> input(f.size);
      fstr.read(&input[0], input.size());
      file.write(&input[0], input.size());
    }
  }

  // テスト
  {
    std::cout << "\nTest packed files.\n" << std::endl;

    std::string path{ argv[2] };
    PackedFile packed{ path };

    // 実際のファイルと比較する
    for (boost::filesystem::directory_entry& x : boost::filesystem::directory_iterator(p))
    {
      if (!isHidden(x.path()) && isPack(x.path()))
      {
        std::vector<char> src(boost::filesystem::file_size(x.path()));
        {
          std::ifstream fstr(x.path().string(), std::ios::binary);
          fstr.read(&src[0], src.size());
        }
        auto dst = packed.read(x.path().filename().string());

        // TIPS std::vectorの比較
        assert(src == dst);
      }
    }
  }
}
