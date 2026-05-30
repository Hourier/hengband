#include "io/temp-file.h"
#include "system/angband-exceptions.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <system_error>
#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

TempFile::TempFile()
{
#ifdef _WIN32
    char win_temp[MAX_PATH]{};
    GetTempPathA(MAX_PATH, win_temp);
    char temp_file_path[MAX_PATH]{};
    if (GetTempFileNameA(win_temp, "tmp", 0, temp_file_path) == 0) {
        THROW_EXCEPTION(std::runtime_error, "Failed to generate temporary file name with GetTempFileNameA");
    }

    this->path = temp_file_path;
#else
    const auto *tmp_dir_env = std::getenv("TMPDIR");
    std::filesystem::path temp_dir = tmp_dir_env ? tmp_dir_env : "/tmp";

    auto temp_str = (temp_dir / "tempfile_XXXXXX").string();
    const auto fd = mkstemp(&temp_str[0]);
    if (fd == -1) {
        THROW_EXCEPTION(std::runtime_error, "Failed to generate temporary file name with mkstemp");
    }

    close(fd);
    this->path = std::filesystem::path(temp_str);
#endif
}

TempFile::~TempFile()
{
    if (this->ifs && this->ifs.is_open()) {
        this->ifs.close();
    }

    if (!this->path.empty()) {
        std::error_code ec;
        std::filesystem::remove(this->path, ec);
    }
}

const std::filesystem::path &TempFile::get_path() const
{
    return this->path;
}

/*!
 * @details 日本語版ならばシフトJIS or EUC-JPでエンコードされた文字列しか書き込まないはずなので、再び読み込む時は何もしない.
 */
tl::optional<std::string> TempFile::read_line()
{
    this->open_for_reading();
    std::string line;
    std::getline(this->ifs, line);
    return this->ifs ? tl::make_optional(line) : tl::nullopt;
}

/*!
 * @details 日本語版ならばシフトJIS or EUC-JPでエンコードされた文字列しか書き込まないはずなので、再び読み込む時は何もしない.
 */
std::vector<std::string> TempFile::read_all()
{
    this->ifs = std::ifstream(this->path);
    if (!this->ifs) {
        THROW_EXCEPTION(std::runtime_error, "Cannot open file for reading");
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(this->ifs, line)) {
        lines.push_back(line);
    }

    return lines;
}

void TempFile::write_line(std::string_view line) const
{
    std::ofstream ofs(this->path, std::ios::app);
    if (!ofs) {
        THROW_EXCEPTION(std::runtime_error, "Cannot open file for writing");
    }

    ofs << line << std::endl;
}

void TempFile::write_lines(const std::vector<std::string> &lines) const
{
    std::ofstream ofs(this->path, std::ios::app);
    if (!ofs) {
        THROW_EXCEPTION(std::runtime_error, "Cannot open file for writing");
    }

    for (const auto &line : lines) {
        ofs << line << std::endl;
    }
}

void TempFile::open_for_reading()
{
    if (!this->ifs || !this->ifs.is_open()) {
        this->ifs = std::ifstream(this->path);
    }

    if (!this->ifs) {
        THROW_EXCEPTION(std::runtime_error, "Cannot open file for reading");
    }
}
