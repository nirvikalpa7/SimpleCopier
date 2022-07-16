
#include "copylib.h"

#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>
#include <csignal>

namespace CopyLib {

namespace fs = std::filesystem;

using namespace std::chrono_literals;

namespace {

    const std::string tempFN{ "copy_plan_" };
    const std::string tempExten{ ".txt" };

    std::atomic<bool> copyErrorHappened{ false };

}; // namespace

//===================================================================================================================================

bool isCopyErrorHappened()
{
    return copyErrorHappened;
}

//===================================================================================================================================

bool createCopyQueues(const std::string & origin, const std::string & dest,
                      const uint32_t hardwConcur, uint64_t & scopeSize, uint64_t & fileNum)
{
    // Check input params
    if (hardwConcur == 0 || origin.empty() || dest.empty())
    {
        return false;
    }
    {
        const fs::path originPath = origin;
        const fs::path destPath = dest;
        if (!fs::exists(origin) || !fs::exists(dest) || originPath == destPath)
        {
            return false;
        }
    }
    std::ofstream * fplan = new (std::nothrow) std::ofstream [hardwConcur];
    if (fplan == nullptr)
    {
        return false;
    }
    const std::string tempDir = fs::temp_directory_path().string();
    for(size_t i = 0U; i < hardwConcur; i++)
    {
        const std::string path = tempDir + tempFN + std::to_string(i) + tempExten;
        fplan[i].open(path);
        if (!fplan[i].is_open()) // Safe exit
        {
            for(size_t j = 0U; j < i; j++)
            {
                fplan[j].close();
            }
            return false;
        }
    }

    for(size_t i = 0; i < hardwConcur; i++)
    {
        fplan[i] << origin << std:: endl;
        fplan[i] << dest << std:: endl;
    }

    scopeSize = 0U;
    fileNum = 0U;
    for (const auto& dir_entry : fs::recursive_directory_iterator(origin))
    {
        if (dir_entry.is_regular_file())
        {
            scopeSize += dir_entry.file_size();
            const std::string full_file = dir_entry.path().string();
            const std::string file = full_file.substr(origin.size());
            const uint32_t fStreamIndex = fileNum % hardwConcur;
            fplan[fStreamIndex] << file << std::endl;
            fileNum++;
        }
    }

    for(size_t i = 0; i < hardwConcur; i++)
    {
        fplan[i].close();
    }
    delete [] fplan;

    copyErrorHappened.store(false);
    return true;
}

//===================================================================================================================================

void worker(const std::string queue, std::atomic<uint64_t>& copiedFileSize,
            std::atomic<uint64_t>& copiedFileNum, const std::atomic<bool>& copyCancel)
{
    if (!queue.empty() && fs::exists(queue))
    {
        std::ifstream fin(queue);
        if (fin.is_open())
        {
            std::string origin;
            std::getline(fin, origin);
            std::string dest;
            std::getline(fin, dest);
            if (origin.empty() || dest.empty())
            {
                fin.close();
                return;
            }
            std::string currentFile, fullPath;
            std::error_code code;
            const auto copyOptions = fs::copy_options::overwrite_existing;
            while(!fin.eof() && !copyCancel.load())
            {
                std::getline(fin, currentFile);
                if (!currentFile.empty())
                {
                    fullPath = origin + currentFile;
                    if (fs::exists(fullPath) && fs::is_regular_file(fullPath))
                    {
                        fs::copy(fullPath, dest + currentFile, copyOptions, code);
                        if (code.value() != 0) // For access denied it is 5
                        {
                            copyErrorHappened.store(true);
                        }
                        code.clear();
                        copiedFileSize += fs::file_size(fullPath);
                        copiedFileNum++;
                    }
                }
            }
            fin.close();
        }
    }
}

//===================================================================================================================================

void copyDirStructure()
{
    const std::string tempDir = fs::temp_directory_path().string();
    const std::string pathFirstQueue = tempDir + tempFN + "0" + tempExten;
    if (fs::exists(pathFirstQueue))
    {
        const auto copyOptions = fs::copy_options::update_existing
                                   | fs::copy_options::recursive
                                   | fs::copy_options::directories_only;

        std::ifstream fin(pathFirstQueue);
        if (fin.is_open())
        {
            std::string origin;
            std::getline(fin, origin);
            std::string dest;
            std::getline(fin, dest);
            fs::copy(origin, dest, copyOptions);
        }
    }
}

//===================================================================================================================================

void removeCopyQueues(const uint32_t hardwConcur)
{
    const std::string tempDir = fs::temp_directory_path().string();
    for(size_t i = 0U; i < hardwConcur; i++)
    {
        const std::string path = tempDir + tempFN + std::to_string(i) + tempExten;
        if (fs::exists(path))
        {
            fs::remove(path);
        }
    }
}

//===================================================================================================================================

bool isEnoughSpace(const std::string & dest, const uint64_t spaceNeeded)
{
    const auto space = fs::space(dest);
    return (space.available >= spaceNeeded);
}

//===================================================================================================================================

std::string getTempFN() // for unit tests
{
    return tempFN;
}

//===================================================================================================================================

std::string getTempExten() // for unit tests
{
    return tempExten;
}

//===================================================================================================================================

}; // namespace CopyLib
