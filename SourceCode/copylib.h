#ifndef COPYLIB_H
#define COPYLIB_H

#include <string>
#include <atomic>

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

} // namespace CopyLib

#endif // COPYLIB_H
