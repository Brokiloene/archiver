#pragma once

#include "fstream"
#include "string"
#include "queue"
#include "iostream"


struct EOFReachedException {
    
};

class BitStream {
private:
    std::fstream f;

    std::string filename;
    std::string mode;
    std::deque<unsigned char> buffer;
    int bitsAvailable = 0;

    const int BYTE_SIZE = 8;
    const int MAX_SHIFT = BYTE_SIZE - 1;
    const int BUFFER_MAX_SIZE = 131072; // 128 kb

public:
    BitStream() {}
    BitStream(std::string filename, std::string mode) {
        this->mode = mode;
        this->filename = filename;
        if (mode == "w") {
            if (filename != "stdout") {
                f.open(filename, std::ios::out | std::ios::trunc | std::ios::binary);
            }
        } else if (mode == "r") {
            f.open(filename, std::ios::in | std::ios::binary);
        } else {
            throw std::runtime_error(std::string("BitStream incorrect mode: " + mode));
        }
    }

    ~BitStream() {
        if (mode == "w" && buffer.size() != 0) {
            flushBuffer();
        }
    }

    void fillBuffer() {
        char temp_buf[BUFFER_MAX_SIZE];
        f.read(temp_buf, sizeof(temp_buf));
        int readCnt = f.gcount();
        for (int i = 0; i < readCnt; ++i) {
            buffer.push_back(temp_buf[i]);
        }
        bitsAvailable += readCnt * BYTE_SIZE;
        if (bitsAvailable == 0) {
            throw EOFReachedException();
        }
    }

    void flushBuffer() {
        if (buffer.size() > 0 && bitsAvailable >= BYTE_SIZE) {
            std::vector<char> v;
            std::copy(buffer.begin(), buffer.end(), std::back_inserter(v));
            if (filename == "stdout") {
                std::cout.write(reinterpret_cast<char*>(v.data()), v.size());
            } else {
                f.write(reinterpret_cast<char*>(v.data()), v.size());
            }
            buffer.clear();
            bitsAvailable = 0;
        }
    }

    uint32_t readBits(int bitsNeeded) {
        uint32_t res = 0;
        unsigned char tmp = 0;

        if (bitsAvailable < bitsNeeded) {
            fillBuffer();
        }

        if (bitsAvailable < bitsNeeded) {
            throw EOFReachedException();
        }

        // check if the last byte was partially used
        if (bitsAvailable % BYTE_SIZE != 0) {
            int bitsUnused = bitsAvailable & (BYTE_SIZE - 1); // == bitsAvailable % BYTE_SIZE
            tmp = buffer.front();
            buffer.pop_front();

            if (bitsUnused > bitsNeeded) {
                for (int i = 0; i < bitsNeeded; ++i) {
                    --bitsUnused;

                    res <<= 1;
                    res |= (tmp >> bitsUnused & 1);
                    tmp &= ~(1 << bitsUnused);
                }
                bitsAvailable -= bitsNeeded;
                bitsNeeded = 0; 
                buffer.push_front(tmp);

            } else {
                res += static_cast<uint32_t>(tmp);
                bitsNeeded -= bitsUnused;
                bitsAvailable -= bitsUnused;
            }
        }

        while (bitsNeeded >= BYTE_SIZE) {
            tmp = buffer.front();
            buffer.pop_front();
            
            res <<= BYTE_SIZE;
            res += static_cast<uint32_t>(tmp);
            bitsNeeded -= BYTE_SIZE;
            bitsAvailable -= BYTE_SIZE;
        }

        if (bitsNeeded < BYTE_SIZE && bitsNeeded > 0) {
            tmp = buffer.front();
            buffer.pop_front();
            int tmpBits = BYTE_SIZE;

            for (int i = 0; i < bitsNeeded; ++i) {
                --tmpBits;
                res <<= 1;
                res |= (tmp >> tmpBits & 1);
                tmp &= ~(1 << tmpBits);
            }
            
            buffer.push_front(tmp);
            bitsAvailable -= bitsNeeded;
        }

        return res;
    }

    void writeBits(uint32_t val, int bits) {
        unsigned char tmp = 0;
        int tmpBits = 0;
        if (bitsAvailable % BYTE_SIZE != 0) {
            tmp = buffer.back();
            buffer.pop_back();
            tmpBits = bitsAvailable % BYTE_SIZE;

            // fill the last char
            if (bits <= BYTE_SIZE - tmpBits) {
                tmp += static_cast<unsigned char> (val << (BYTE_SIZE - tmpBits - bits));
                tmpBits = bits;
                bits = 0;
            } else {            
                for (int i = 0, end = BYTE_SIZE - tmpBits; i < end; ++i) {
                    // get i-th from msb bit value from val
                    // set it to the non-used msb of temp
                    tmp |= ((val >> (bits - i - 1) & 1) << (MAX_SHIFT - tmpBits));
                    ++tmpBits;

                    if (tmpBits == BYTE_SIZE) {
                        buffer.push_back(tmp);
                        bitsAvailable += (i+1);
                        bits -= (i+1);
                    }
                }
                tmp = 0;
                tmpBits = 0;
            }
        }

        for (int i = 0; i < bits; ++i) {
            // get i-th from msb bit value from val
            // set it to the non-used msb of temp
            tmp |= ((val >> (bits - i - 1) & 1) << (MAX_SHIFT - tmpBits));
            ++tmpBits;

            if (tmpBits == BYTE_SIZE) {
                if (buffer.size() == BUFFER_MAX_SIZE) {
                    flushBuffer();
                }
                buffer.push_back(tmp);
                tmp = 0;
                tmpBits = 0;
                bitsAvailable += BYTE_SIZE;
            }
        }

        if (!f) {
            throw std::runtime_error("Output filestream is not available\n");
        }

        if (tmpBits != 0) {
            if (buffer.size() == BUFFER_MAX_SIZE) {
                flushBuffer();
            }
            buffer.push_back(tmp);
            bitsAvailable += tmpBits;
        }
    }

    int getByte() {
        return f.get();
    }
};
