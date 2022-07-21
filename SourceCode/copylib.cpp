
#include "copylib.h"

#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>
#include <csignal>
#include <sstream>

namespace CopyLib {

namespace fs = std::filesystem;

using namespace std::chrono_literals;

namespace {

    const std::string tempFN{ "copy_plan_" };
    const std::string tempExten{ ".txt" };

    std::atomic<bool> copyErrorHappened{ false };

    std::string getCurrentThreadId()
    {
        const auto myid = std::this_thread::get_id();
        std::stringstream ss;
        ss << myid;
        return ss.str();
    }

}; // namespace

//===================================================================================================================================

bool isCopyErrorHappened()
{
    return copyErrorHappened;
}

//===================================================================================================================================

bool isEnoughSpace(const std::string_view & dest, const uint64_t spaceNeeded)
{
    if (dest.empty())
    {
        return false;
    }
    const auto space = fs::space(dest);
    return (space.available >= spaceNeeded);
}

//===================================================================================================================================

bool createCopyQueues(const std::string_view & origin, const std::string_view & dest,
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
    bool retValue{ true };
    const auto dirOption { std::filesystem::directory_options::skip_permission_denied };
    try {
        for (const auto& dir_entry : fs::recursive_directory_iterator(origin, dirOption))
        {
            if (dir_entry.is_regular_file())
            {
                scopeSize += dir_entry.file_size();
                const std::string full_file = dir_entry.path().string();
                const std::string file = full_file.substr(origin.size());
                const uint32_t fStreamIndex = fileNum % hardwConcur;
                fplan[fStreamIndex] << file << std::endl; // False positive warning, index is correct
                fileNum++;
            }
        }
    }
    catch(const std::exception & e) // Access denied. Can happens for C:/ or C:/Windows origin dir
    {
        retValue = false;
        auto & logger = TLogger::getInstance();
        logger.startLogging();
        const auto logMesBase = std::string(__FUNCTION__) + ", thread: " + getCurrentThreadId() + ". ";
        logger.logMessage(logMesBase + "Error! Can not create copy queue files. Access denied. Origin: " + origin.data() + " System info: " + e.what());
        logger.finishLogging();
    }

    for(size_t i = 0; i < hardwConcur; i++)
    {
        fplan[i].close();
    }
    delete [] fplan;

    if (retValue)
    {
        TLogger::getInstance().startLogging(); // create log file and open it
    }
    copyErrorHappened.store(false); // variable for worker fun copy error signalization

    return retValue;
}

//===================================================================================================================================

void copyDirStructure()
{
    const auto tempDir = fs::temp_directory_path().string();
    const auto pathFirstQueue = tempDir + tempFN + "0" + tempExten;
    const auto logMesBase = std::string(__FUNCTION__) + ", thread: " + getCurrentThreadId() + ". Error! ";
    auto & logger = TLogger::getInstance();
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
            if (!origin.empty() && !dest.empty())
            {
                if (fs::exists(origin) && fs::exists(dest))
                {
                    std::error_code code;
                    fs::copy(origin, dest, copyOptions, code);
                    if (code.value() != 0) // For access denied it is 5
                    {
                        copyErrorHappened.store(true);
                        logger.logMessage(logMesBase + "Can not copy a file, you do not have permissions for the destination folder or the file is being opened. Destination: " + dest);
                    }
                    code.clear();
                }
                else
                {
                    logger.logMessage(logMesBase + "Origin or destination dir does not exist! Origin: " + origin + " Dest: " + dest);
                }
            }
            else
            {
                logger.logMessage(logMesBase + "Origin or destination dir is en empty in the first queue file! " + pathFirstQueue);
            }
            fin.close();
        }
        else
        {
            logger.logMessage(logMesBase + "Can not open queue file! " + pathFirstQueue);
        }
    }
    else
    {
        logger.logMessage(logMesBase + "First queue file does not exist! " + pathFirstQueue);
    }
}

//===================================================================================================================================

void worker(const std::string queue, std::atomic<uint64_t>& copiedFileSize,
            std::atomic<uint64_t>& copiedFileNum, std::atomic<uint32_t>& finishedThreadsNum,
            const std::atomic<bool>& copyCancel)
{
    const std::string logMesBase = std::string(__FUNCTION__) + ", thread: " + getCurrentThreadId() + ". ";
    auto & logger = TLogger::getInstance();

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
                logger.logMessage(logMesBase + "Error! Incorrect structure in queue file: origin or destination dir is not provided! " + queue);
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
                    if (fs::exists(fullPath))
                    {
                        if(fs::is_regular_file(fullPath))
                        {
                            fs::copy(fullPath, dest + currentFile, copyOptions, code);
                            if (code.value() != 0) // For access denied it is 5
                            {
                                copyErrorHappened.store(true);
                                logger.logMessage(logMesBase + "Error! Can not copy a file, you do not have permissions for the destination folder or the file is being opened. " + fullPath);
                            }
                            code.clear();
                            copiedFileSize += fs::file_size(fullPath);
                            copiedFileNum++;
                        }
                        else
                        {
                            logger.logMessage(logMesBase + "Warning! File to copy from queue file is not regular and will be skipped! " + fullPath);
                        }
                    }
                    else
                    {
                        logger.logMessage(logMesBase + "Error! A file to copy from queue file does not exist! " + fullPath);
                    }
                }
            }
            fin.close();
        }
        else
        {
            logger.logMessage(logMesBase + "Error! Can not open input queue file! " + queue);
        }
    }
    else
    {
        logger.logMessage(logMesBase + "Error! Queue file param is an empty or does not exist!" + " Queue file param: " + queue);
    }

    finishedThreadsNum++;
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

    TLogger::getInstance().finishLogging(); // close log file
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
