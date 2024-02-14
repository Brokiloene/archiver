#include "bitstream.hpp"

#include "cstdint"
#include "string"


class BAC {
private:
    const int BYTE_SIZE = 8;
    const int EOF_CODE = 256;

    uint32_t CODE_VALUE_BITS = 17;
    uint32_t MAX_CODE = (uint32_t(1) << CODE_VALUE_BITS) - 1;
    uint32_t ONE_FOURTH = (uint32_t(1) << (CODE_VALUE_BITS - 2));
    uint32_t ONE_HALF = ONE_FOURTH * 2;
    
    uint32_t THREE_FOURTHS = ONE_FOURTH * 3;

    struct Probability {
        uint32_t low;
        uint32_t high;
        uint32_t count;
    };

    class Model {
    private:
        uint32_t FREQUENCY_BITS = 15;
        uint32_t MAX_FREQUENCY = (uint32_t(1) << FREQUENCY_BITS) - 1;
        bool is_full;
    public:
        std::vector<uint32_t> cumulative_frequency;
        void reset() {
            cumulative_frequency.clear();
            for (int i = 0; i <= 257; ++i) {
                cumulative_frequency.push_back(i);
            }
            is_full = false;
        }

        void update(int c) {
            for (int i = c + 1; i <= 257; ++i) {
                ++cumulative_frequency[i];
            }
            if (cumulative_frequency[257] >= MAX_FREQUENCY) {
                is_full = true;
            }   
        }

        Probability getProbability(int c) {
            Probability prob =  {
                cumulative_frequency[c],
                cumulative_frequency[c + 1],
                cumulative_frequency[257]
            };
            if (!is_full) {
                update(c);
            }
            return prob;
        }

        Probability getChar(uint32_t scaled_value, int &c) {
            for (int i = 0; i <= 256; ++i) {
                if (scaled_value < cumulative_frequency[i + 1]) {
                    c = i;
                    Probability prob = {
                        cumulative_frequency[i],
                        cumulative_frequency[i+1],
                        cumulative_frequency[257]
                    };

                    if (!is_full) {
                        update(c);
                    }
                    return prob;
                }
            }
            throw std::logic_error("Error in getChar");
        }

        uint32_t getCount() {
            return cumulative_frequency[257];
        }
    };

    Model model;

    void flush_bits(uint8_t bit, uint32_t& pending_bits, BitStream& fo) {
        fo.writeBits(bit, 1);
        bit = !bit;
        for (int i = 0; i < pending_bits; ++i) {
            fo.writeBits(bit, 1);
        }
        pending_bits = 0;
    }

public:
    BAC() {}
    void Compress(std::string in, std::string out) {
        BitStream fi(in, "r");
        BitStream fo(out,"w");

        model.reset();

        uint32_t low = 0;
        uint32_t high = MAX_CODE;

        uint32_t pending_bits = 0;
        int c = 0;
        while (true) {
            c = fi.getByte();
            if (c == std::istream::traits_type::eof()) {
                c = EOF_CODE;
            }

            Probability prob = model.getProbability(c);
            uint32_t range = high - low + 1;
            high = low + (range * prob.high / prob.count) - 1;
            low = low + (range * prob.low / prob.count);

            for (;;) {
                if (high < ONE_HALF) {
                    flush_bits(0, pending_bits, fo);
                } else if (low >= ONE_HALF) {
                    flush_bits(1, pending_bits, fo);
                } else if (low >= ONE_FOURTH && high < THREE_FOURTHS) {
                    ++pending_bits;
                    low -= ONE_FOURTH;
                    high -= ONE_FOURTH;
                } else {
                    break;
                }
                high <<= 1;
                high |= 0x1;
                
                low <<= 1;
                low |= 0x0;

                high &= MAX_CODE;
                low &= MAX_CODE;
            }

            if (c == EOF_CODE) {
                break;
            }
        }
        ++pending_bits;
        if (low < ONE_FOURTH) {
            flush_bits(0, pending_bits, fo);
        } else {
            flush_bits(1, pending_bits, fo);
        }
    }


    void Decompress(std::string in, std::string out) {
        BitStream fi(in, "r");
        BitStream fo(out,"w");

        model.reset();

        uint32_t low = 0;
        uint32_t high = MAX_CODE;
        uint32_t val = 0;

        val += fi.readBits(CODE_VALUE_BITS);

        while (true) {
            uint32_t range = high - low + 1;
            uint32_t scaled_val = ((val - low + 1) * model.getCount() - 1) / range;
            
            int c;
            Probability prob = model.getChar(scaled_val, c);
            if (c == 256) {
                break;
            }
            fo.writeBits(c, BYTE_SIZE);

            high = low + (range * prob.high) / prob.count - 1;
            low = low + (range * prob.low) / prob.count;

            int t = 1;
            for (;;) {
                if (high < ONE_HALF) {

                } else if (low >= ONE_HALF) {
                    high -= ONE_HALF;
                    low -= ONE_HALF;
                    val -= ONE_HALF;
                } else if (high < THREE_FOURTHS && low >= ONE_FOURTH) {
                    high -= ONE_FOURTH;
                    low -= ONE_FOURTH;
                    val -= ONE_FOURTH;
                } else {
                    break;
                }

                high <<= 1;
                high |= 0x1;

                low <<= 1;
                low |= 0x0;

                val <<= 1;

                try {
                    val += fi.readBits(1);
                } catch (EOFReachedException& ex) {
                    break;
                }
            }
        }
    }
};
