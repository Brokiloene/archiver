#include "fstream"
#include "string"
#include "stdexcept"
#include "queue"

#include "iostream"
#include "bitset"

struct EOFReachedException {
    
};


class BitStream {
private:
    std::fstream f;

    int bitsCnt = 0;
    unsigned char temp = 0;

    const int BYTE_SIZE = 8;

public:
    BitStream() {}
    BitStream(std::string filename, std::string mode) {
        if (mode == "w") {
            f.open(filename, std::ios::out | std::ios::trunc | std::ios::binary);
        } else if (mode == "r") {
            f.open(filename, std::ios::in | std::ios::binary);
        } else {
            throw std::runtime_error(std::string("BitStream incorrect mode: " + mode));
        }
    }
    
    uint32_t readBits(int bitsNeeded) {
        uint32_t res = 0;

        if (bitsCnt > bitsNeeded) {
            for (int i = 0; i < bitsNeeded; ++i) {
                --bitsCnt;

                res <<= 1;
                res |= (temp >> bitsCnt & 1);

                temp &= ~(1 << bitsCnt);
            }
            bitsNeeded = 0;
        } else {
            res = temp;
            bitsNeeded -= bitsCnt;
            bitsCnt = 0;
            temp = 0;
        }

        while (bitsNeeded > 0) {
            unsigned char c = 0;
            
            if (!f) {
                throw EOFReachedException();
            }
            
            f.read(reinterpret_cast<char*>(&c), sizeof(c));

            if (f.gcount() < sizeof(char)) {
                throw EOFReachedException();
            }            
            
            if (bitsNeeded >= BYTE_SIZE) {
                res <<= BYTE_SIZE;
                res += c;
                bitsNeeded -= BYTE_SIZE;
                continue;
            }

            // it remains to set less than 8 bits 
            bitsCnt = BYTE_SIZE;

            for (int i = 0; i < bitsNeeded; ++i) {
                --bitsCnt;

                res <<= 1;
                res |= (c >> bitsCnt & 1);

                c &= ~(1 << bitsCnt);
            }
            bitsNeeded = 0;
            temp = c;
        }
        return res;
    }

    void writeBits(uint32_t val, int bits, bool fill=false) {
        for (int i = 0; i < bits; ++i) {
            
            // get i-th from msb bit value from val
            // set it to the non-used msb of temp
            temp |= ((val >> (bits - i - 1) & 1) << (7 - bitsCnt));
            ++bitsCnt;

            if (bitsCnt == 8) {
                f.write(reinterpret_cast<char*>(&temp), sizeof(temp));
                temp = 0;
                bitsCnt = 0;
            }
        }

        if (!f) {
            throw EOFReachedException();
        }

        // align last write to the byte size
        if (fill && bitsCnt) {
            f.write(reinterpret_cast<char*>(&temp), sizeof(temp));
            temp = 0;
            bitsCnt = 0;
        }
    }
};
