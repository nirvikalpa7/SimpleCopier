#ifndef COPYLIB_H
#define COPYLIB_H

#include <string>
#include <atomic>
#include <mutex>
#include <fstream>

namespace CopyLib {

    bool createCopyQueues(const std::string & origin, const std::string & dest,
                          const uint32_t hardwConcur, uint64_t & scopeSize, uint64_t & fileNum);

    void copyDirStructure();

    bool isEnoughSpace(const std::string & dest, const uint64_t spaceNeeded);

    void removeCopyQueues(const uint32_t hardwConcur);

    void worker(const std::string queue, std::atomic<uint64_t>& copiedFileSize,
                std::atomic<uint64_t>& copiedFileNum, const std::atomic<bool>& copyCancel);

    std::string getTempFN();

    std::string getTempExten();

    bool isCopyErrorHappened();

    //===================================================================================================================================

    // Logger singleton for multithreaded worker fun
    class TLogger
    {
    public:

        static TLogger & getInstance()
        {
            static TLogger logger;
            return logger;
        }

        bool logMessage(const std::string & message)
        {
            if (message.empty())
            {
                return false;
            }

            const std::lock_guard<std::mutex> lock(fStreamMutex);
            if (fout.is_open())
            {
                fout << logMessageNum++ << ": "<< message << std::endl;
                return true;
            }
            return false;
        }

        void startLogging()
        {
            if(!fout.is_open())
            {
                fout.open(logFileName);
                logMessageNum = 1U;
            }
        }

        void finishLogging()
        {
            if(fout.is_open())
            {
                fout.close();
            }
        }

        std::string getLogFileName() const { return logFileName; }

    private:

        TLogger() { }
        ~TLogger() { finishLogging(); }
        TLogger(const TLogger & log) = delete;
        TLogger operator=(const TLogger & log) = delete;

        std::mutex fStreamMutex;
        std::ofstream fout;
        const std::string logFileName{"simpleCopyLog.txt"};
        uint64_t logMessageNum{ 1U };

    }; // TLogger

    //===================================================================================================================================

} // namespace CopyLib

#endif // COPYLIB_H
