#include "bitstream.hpp"
#include "../../../mylib/utils/timer_guard.hpp"

#include "bitset"
#include "cstdint" //uint32, uint64
#include "unordered_map"
#include "string"

#include "iostream"
// using ustring = std::basic_string<unsigned char>;

class LZW {
private:
    std::unordered_map<std::string, uint32_t> compress;
    std::unordered_map<uint32_t, std::string> decompress;

    int CODE_LENGTH_MAX = 10;
    int MAX_CODE = 1000000;
    uint32_t EOF_CODE = 256;

    void setBit(int num, uint32_t& c) {
        c |= 1 << num;
    }

    bool isPowerOfTwo(uint32_t x) {
        return (x != 0) && ((x & (x - 1)) == 0);
    }

public:
    LZW() {
        resetDicts();
    }
    void resetDicts() {
        compress.clear();
        decompress.clear();
        for (uint32_t i = 0; i < 256; ++i) {
            compress[std::string(1, char(i))] = i;
            decompress[i] = std::string(1, char(i));
        }
    }

    void Compress(std::string in, std::string out) {
        BitStream fi(in, "r");
        BitStream fo(out,"w");

        uint32_t max_code = 255;
        int code_length = 8;

        std::string s = "";
        char cur;

        while (true) {
            try {
                cur = fi.readBits(8);
            } catch (EOFReachedException& ex) {
                break;
            }

            if (compress.count(s + cur) == 1) {
                s += cur;
            } else {
                fo.writeBits(compress[s], code_length);
                compress[s + cur] = ++max_code;

                if (isPowerOfTwo(max_code)) {
                    code_length += 1;
                }
                s = cur;
            }
            // ---
            if (max_code == MAX_CODE) {
                fo.writeBits(compress[s], code_length);
                s = "";
                resetDicts();
                max_code = 255;
                code_length = 8;
            }
            // ---
        }
        if (s != "") {
            fo.writeBits(compress[s], code_length);
        }

        std::cout << max_code << '\n';
    }

    void Decompress(std::string in, std::string out) {
        BitStream fi(in, "r");
        BitStream fo(out,"w");

        uint32_t max_code = 255;
        int code_length = 8;
        uint32_t prevcode;
        bool first_write = true;
        bool dicts_reset = false;

        try {
            prevcode = fi.readBits(code_length);
        } catch (EOFReachedException& ex) {
            std::cout << "archive is empty!\n";
        }

        fo.writeBits(decompress[prevcode][0], 8);
        std::string curstr = decompress[prevcode];
        uint32_t curcode;

        while (true) {
            // ---
            if (max_code == MAX_CODE) {
                code_length = 8;
                try {
                    prevcode = fi.readBits(code_length);
                } catch (EOFReachedException& ex) {
                    break;
                }
                fo.writeBits(decompress[prevcode][0], 8);
                curstr = decompress[prevcode];
                resetDicts();
                max_code = 255;
            }
            // ---

            if (isPowerOfTwo(max_code+1)) {
                code_length += 1;
            }

            try {
                curcode = fi.readBits(code_length);
            } catch (EOFReachedException& ex) {
                break;
            }

            if (decompress.count(curcode) != 1) {
                curstr = curstr + curstr[0];
                decompress[++max_code] = curstr;

            } else {
                curstr = decompress[curcode];
                decompress[++max_code] = decompress[prevcode] + curstr[0];
            }

            for (char c : curstr) {
                fo.writeBits(c, 8);
            }
            prevcode = curcode;
        }

        std::cout << max_code << '\n';
    }
};


int main(int argc, char const *argv[])
{
    {
    TimerGuard t;
    LZW lzw;
    lzw.Compress("in7", "out.lzw");
    lzw.Decompress("out.lzw", "res");
    }

    return 0;
}
