#include "bitstream.hpp"
#include "../../../mylib/utils/timer_guard.hpp"

#include "cstdint" //uint32
#include "unordered_map"
#include "string"

#include "iostream"


class LZW {
private:
    std::unordered_map<std::string, uint32_t> compress;
    std::unordered_map<uint32_t, std::string> decompress;

    const uint32_t MAX_CODE = 4194304; // 2^22
    const int BYTE_SIZE = 8;
    const int START_CODE = fastPow(2, BYTE_SIZE) - 1;

    bool isPowerOfTwo(uint32_t x) {
        return (x != 0) && ((x & (x - 1)) == 0);
    }

    int fastPow(int base, int pow) {
        int res = 1;
        int curmult = base;
        int curpow = pow;
        while (curpow != 0) {
            if (curpow % 2 == 1) {
                res *= curmult;
            }
            curpow >>= 1;
            curmult *= curmult;
        }
        return res;
}

public:
    LZW() {
        resetDicts();
    }
    void resetDicts() {
        compress.clear();
        decompress.clear();
        for (uint32_t i = 0; i <= 255; ++i) {
            compress[std::string(1, char(i))] = i;
            decompress[i] = std::string(1, char(i));
        }
    }

    void Compress(std::string in, std::string out) {
        BitStream fi(in, "r");
        BitStream fo(out,"w");

        uint32_t cur_max_code = 255;
        int code_length = BYTE_SIZE;

        std::string s = "";
        char cur;

        while (true) {
            try {
                cur = fi.readBits(BYTE_SIZE);
            } catch (EOFReachedException& ex) {
                break;
            }

            if (compress.count(s + cur) == 1) {
                s += cur;
            } else {
                fo.writeBits(compress[s], code_length);
                compress[s + cur] = ++cur_max_code;

                if (isPowerOfTwo(cur_max_code)) {
                    code_length += 1;
                }
                s = cur;
            }

            if (cur_max_code == MAX_CODE) {
                fo.writeBits(compress[s], code_length);
                s = "";
                resetDicts();
                cur_max_code = 255;
                code_length = BYTE_SIZE;
            }

        }
        if (!s.empty()) {
            fo.writeBits(compress[s], code_length);
        }

        // std::cout << cur_max_code << '\n';
    }

    void Decompress(std::string in, std::string out) {
        BitStream fi(in, "r");
        BitStream fo(out,"w");

        uint32_t cur_max_code = 255;
        uint32_t code_length = BYTE_SIZE;
        uint32_t prevcode;

        try {
            prevcode = fi.readBits(code_length);
        } catch (EOFReachedException& ex) {
            std::cout << "archive is empty!\n";
        }

        fo.writeBits(decompress[prevcode][0], BYTE_SIZE);
        std::string curstr = decompress[prevcode];
        uint32_t curcode;

        while (true) {

            if (cur_max_code == MAX_CODE) {
                code_length = BYTE_SIZE;
                try {
                    prevcode = fi.readBits(code_length);
                } catch (EOFReachedException& ex) {
                    break;
                }
                fo.writeBits(decompress[prevcode][0], BYTE_SIZE);
                curstr = decompress[prevcode];
                resetDicts();
                cur_max_code = 255;
            }


            if (isPowerOfTwo(cur_max_code+1)) {
                code_length += 1;
            }

            try {
                curcode = fi.readBits(code_length);
            } catch (EOFReachedException& ex) {
                break;
            }

            if (decompress.count(curcode) != 1) {
                curstr = curstr + curstr[0];
                decompress[++cur_max_code] = curstr;

            } else {
                curstr = decompress[curcode];
                decompress[++cur_max_code] = decompress[prevcode] + curstr[0];
            }

            for (char c : curstr) {
                fo.writeBits(c, BYTE_SIZE);
            }
            prevcode = curcode;
        }

        // std::cout << cur_max_code << '\n';
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
