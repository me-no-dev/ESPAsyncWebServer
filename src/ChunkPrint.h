#ifndef CHUNKPRINT_H
#define CHUNKPRINT_H

#include <Print.h>

class ChunkPrint : public Print {
  private:
    uint8_t* _destination;
    size_t _to_skip;
    size_t _to_write;
    size_t _pos;

  public:
    ChunkPrint(uint8_t* destination, size_t from, size_t len)
        : _destination(destination), _to_skip(from), _to_write(len), _pos{0} {}
    virtual ~ChunkPrint() {}
    size_t write(uint8_t c) {
      if (_to_skip > 0) {
        _to_skip--;
        return 1;
      } else if (_to_write > 0) {
        _to_write--;
        _destination[_pos++] = c;
        return 1;
      }
      return 0;
    }
    size_t write(const uint8_t* buffer, size_t size) {
      return this->Print::write(buffer, size);
    }
};
#endif
