#include "lzw.hpp"
#include "bac.hpp"
#include "timer_guard.hpp"

#include "iostream"
#include "string"
#include "filesystem"
#include "algorithm"

namespace fs = std::filesystem;

const int STD_OUTPUT_BIT     = 1<<0;
const int DECOMPRESS_BIT     = 1<<1;
const int KEEP_ORIGIN_BIT    = 1<<2;
const int LIST_INFO_BIT      = 1<<3;
const int RECURSIVE_BIT      = 1<<4;
const int TEST_INTEGRITY_BIT = 1<<5;
const int USE_BAC_BIT        = 1<<6;
const int USE_LZW_AND_BAC_BIT= 1<<7;


struct IntegrityError {

};


class ArchiverData {
private:
    enum class Algorithm {
        LZW,
        BAC,
        LZW_BAC
    };

    enum class Mode {
        Compress,
        Decompress
    };

    const int flag;
    std::string filename;
    std::string output_name;
    Algorithm algo;
    Mode mode;

    void SetOutputName() {
        if (flag & STD_OUTPUT_BIT) {
            output_name = "stdout";
        } 
        else if (flag & DECOMPRESS_BIT) {
            output_name = filename;
            output_name = output_name.substr(0, output_name.find('.'));
            output_name += ".res";
        }
        else {
            if (flag & USE_BAC_BIT) {
                output_name = filename + ".bac";
            } 
            else if (flag & USE_LZW_AND_BAC_BIT) {
                output_name = filename;
            }
            else {
                output_name = filename + ".lzw";
            }
        }
    }

    void EstablishOptions() {
        if (flag & USE_BAC_BIT) {
            algo = Algorithm::BAC;
        } else if (flag & USE_LZW_AND_BAC_BIT) {
            algo = Algorithm::LZW_BAC;
        } else {
            algo = Algorithm::LZW;
        }

        if (flag & DECOMPRESS_BIT) {
            mode = Mode::Decompress;
        } else {
            mode = Mode::Compress;
        }
    }

    void ProcessFile() {
        if (mode == Mode::Compress) {
            if (algo == Algorithm::LZW) {
                LZW lzw;
                lzw.Compress(filename, output_name);
            }
            else if (algo == Algorithm::BAC) {
                BAC bac;
                bac.Compress(filename, output_name);
            } 
            else if (algo == Algorithm::LZW_BAC) {
                LZW lzw;
                BAC bac;
                output_name += ".lzw";
                lzw.Compress(filename, output_name);
                bac.Compress(output_name, output_name + ".bac");
                fs::path p = fs::current_path() / output_name;
                output_name += ".bac";
                fs::remove(p);
            }
        }
        else if (mode == Mode::Decompress) {
            if (algo == Algorithm::LZW) {
                LZW lzw;
                lzw.Decompress(filename, output_name);
            }
            else if (algo == Algorithm::BAC) {
                BAC bac;
                bac.Decompress(filename, output_name);
            } 
            else if (algo == Algorithm::LZW_BAC) {
                LZW lzw;
                BAC bac;

                bac.Decompress(filename, output_name + ".lzw");
                lzw.Decompress(output_name + ".lzw", output_name);
                
                output_name += ".lzw";
                fs::path p = fs::current_path() / output_name;
                
                fs::remove(p);
            }
        }
    }
    void Archive() {
        if (flag & LIST_INFO_BIT) {
            {
                TimerGuard t("\nProcessing " + filename + "(sec):");
                ProcessFile();    
            }
            if (mode == Mode::Compress) {
                std::cout.setf(std::ios::fixed);
                fs::path p = fs::current_path() / filename;
                long double s1 = fs::file_size(p);
                std::cout << std::setprecision(0) << "Size of file \'" << filename << "\'(bytes): " << s1 << '\n';

                fs::path p2 = fs::current_path() / output_name;
                long double s2 = fs::file_size(p2);
                std::cout << std::setprecision(0) << "After compressing(bytes): " << s2 << '\n';
                
                std::cout << "Compression ratio: " << std::setprecision(3) << s1 / s2 << "\n\n";
            }

        }
        else {
            ProcessFile();
        }
    }

public:
    ArchiverData(int flag, std::string filename) 
    : flag(flag), filename(filename) {
        EstablishOptions();
    }

    void Process() {
        if (flag & RECURSIVE_BIT && fs::is_directory(filename)) {
            std::vector<std::string> files;
            
            for (auto& entry : fs::recursive_directory_iterator(filename)) {
                fs::path p = entry;
                if (!fs::is_directory(p)) {
                    files.push_back(std::string(p.c_str()));
                }
            }

            for (std::string curfile : files) {
                filename = curfile;
                SetOutputName();
                Archive();
                if (!(flag & KEEP_ORIGIN_BIT)) {
                    fs::remove(filename);
                }
            }
        } else {
            SetOutputName();
            Archive();
            if (!(flag & KEEP_ORIGIN_BIT)) {
                fs::remove(filename);
            }
        }
    }
};



int main(int argc, char const *argv[])
{
    if (argc <= 1) {
        std::cout << "Not enough arguments\n";
        return 1;
    }

    int flag = 0;
    int file_arg_idx = 0;

    for (int i = 1; i < argc; ++i) {
        std::string curArg(argv[i]);
        if (i == 1 && std::string(argv[1]).substr(0, 2) != "--") {
            int end = std::string(argv[1]).length();
            std::string flags(argv[1]);
            for (int j = 1; j < end; ++j) {
                switch(flags[j]) {
                    case 'c':
                        flag |= STD_OUTPUT_BIT;
                        break;
                    case 'd':
                        flag |= DECOMPRESS_BIT;
                        break;
                    case 'k':
                        flag |= KEEP_ORIGIN_BIT;
                        break;
                    case 'l':
                        flag |= LIST_INFO_BIT;
                        break;
                    case 'r':
                        flag |= RECURSIVE_BIT;
                        break;
                    case 't':
                        flag |= TEST_INTEGRITY_BIT;
                        break;
                    case '1':
                        flag |= USE_BAC_BIT;
                        break;
                    case '9':
                        flag |= USE_LZW_AND_BAC_BIT;
                        break;
                    default:
                        std::cout << "Invalid flag: " << flags[i] << '\n';
                        return 1;
                }
            }
            file_arg_idx = i+1;
            break;
        }
        else if (curArg == "-c" || curArg == "--stdout") {
            flag |= STD_OUTPUT_BIT;
        }
        else if (curArg == "-d" || curArg == "--decompress") {
            flag |= DECOMPRESS_BIT;
        }
        else if (curArg == "-k" || curArg == "--keep") {
            flag |= KEEP_ORIGIN_BIT;
        }
        else if (curArg == "-l" || curArg == "--list") {
            flag |= LIST_INFO_BIT;
        }
        else if (curArg == "-r" || curArg == "--recursive") {
            flag |= RECURSIVE_BIT;
        }
        else if (curArg == "-t" || curArg == "--test") {
            flag |= TEST_INTEGRITY_BIT;
        }
        else if (curArg == "-1" || curArg == "--arifm") {
            flag |= USE_BAC_BIT;
        }
        else if (curArg == "-9" || curArg == "--all") {
            flag |= USE_LZW_AND_BAC_BIT;
        }
        else {
            file_arg_idx = i;
            break;
        }
    }

    for (int i = file_arg_idx; i < argc; ++i) {
        std::string filename(argv[i]);
        if (fs::exists(filename)) {
            ArchiverData a(flag, filename);
            a.Process();
        } else {
            std::cout << "File " << filename << " was not found\n";
            return 1;
        }
    }

    return 0;
}
