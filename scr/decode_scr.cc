#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <iomanip>

int main(int argc, char* argv[])
{
  // 実行時の引数チェック
  if(argc != 2){
    std::cout << "Usage: ./decode_scr_2 [input_file.bin]" << std::endl;
    return 0;
  }

  const std::string kInputPath = argv[1];

  // バイナリモードでファイルを開く
  std::ifstream ifs(kInputPath.c_str(), std::ios::binary);
  if(!ifs.is_open()){
    std::cerr << "#E: Cannot open file (" << kInputPath << ")." << std::endl;
    return 1;
  }

  uint32_t magic_number = 0;
  uint32_t num_ch = 0;
  int loop_count = 0;

  // ブロックごとに読み込むループ
  while(ifs.read(reinterpret_cast<char*>(&magic_number), sizeof(magic_number))){

    if(!ifs.read(reinterpret_cast<char*>(&num_ch), sizeof(num_ch))){
      std::cerr << "#E: Header is incomplete." << std::endl;
      break;
    }

    if((magic_number & 0xffff0000) != 0xff040000){
      std::cerr << "#E: Invalid magic number: 0x" << std::hex << magic_number << std::dec << std::endl;
      break;
    }

    uint32_t gate_num = magic_number & 0x0000ffff;
    loop_count++;

    std::cout << "========== Block " << loop_count << " ==========" << std::endl;
    std::cout << " Gate Number : " << gate_num << std::endl;
    std::cout << " Channels    : " << num_ch << std::endl;

    std::vector<uint32_t> scaler_data(num_ch);

    // 実際のカウントデータを一気に読み込む
    if(!ifs.read(reinterpret_cast<char*>(scaler_data.data()), num_ch * sizeof(uint32_t))){
      std::cerr << "#E: Data section is incomplete." << std::endl;
      break;
    }

    // --- 改修したデータ表示部分 ---
    const uint32_t kNumSystemCh = 18;
    const uint32_t kNumPhysicalCh = 64;

    // もし想定通りの82ch (18 + 64) だったら分けて表示する
    if(num_ch == (kNumSystemCh + kNumPhysicalCh)) {
      std::cout << "  [System Information]" << std::endl;
      for(uint32_t i = 0; i < kNumSystemCh; ++i){
        std::cout << "    Sys " << std::setw(2) << i << " : " << scaler_data[i] << std::endl;
      }

      std::cout << "  [Channels Scaler (0-63)]" << std::endl;
      for(uint32_t i = 0; i < kNumPhysicalCh; ++i){
        // 配列の18番目以降(インデックス i + kNumSystemCh)が実際のCh 0になる
        std::cout << "    Ch " << std::setw(2) << i << "  : " << scaler_data[i + kNumSystemCh] << std::endl;
      }
    } else {
      // 万が一82ch以外のデータが来た場合
      for(uint32_t i = 0; i < num_ch; ++i){
        std::cout << "  Data " << std::setw(2) << i << ": " << scaler_data[i] << std::endl;
      }
    }
    std::cout << std::endl;
  }

  std::cout << "#D: Decoding finished. Total blocks: " << loop_count << std::endl;

  ifs.close();
  return 0;
}
