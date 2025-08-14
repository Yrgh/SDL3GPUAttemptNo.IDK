#include "common.h"

byte *read_whole_file(std::string filename, u32 &o_size) {
    std::ifstream file(filename, std::ios::binary);
    
    if (!file.good() || !file.is_open() || file.fail()) {
        file.close();
        o_size = -1;
        return nullptr;
    }

    file.seekg(0, std::fstream::beg);
    u32 start = file.tellg();
    file.seekg(0, std::fstream::end);
    o_size = (u32) file.tellg() - start;
    file.seekg(0, std::fstream::beg);


    byte *buf = new byte[o_size];
    memset(buf, 0, o_size);
    file.read((char *) buf, o_size);
    return buf;
}
