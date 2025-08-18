#include "common.h"

byte *read_whole_file(std::string filename, u32 *o_size) {
    std::ifstream file(filename, std::ios::binary);
    
    if (!file.good() || !file.is_open() || file.fail()) {
        file.close();
        if (o_size) *o_size = -1;
        return nullptr;
    }

    file.seekg(0, std::fstream::beg);
    u32 start = file.tellg();
    file.seekg(0, std::fstream::end);
    u32 size = (u32) file.tellg() - start;
    file.seekg(0, std::fstream::beg);


    byte *buf = new byte[size];
    memset(buf, 0, size);
    file.read((char *) buf, size);

    if (o_size) *o_size = size;

    return buf;
}
