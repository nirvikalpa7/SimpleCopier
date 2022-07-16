
#include "gtest/gtest.h"
#include "../../../SourceCode/copylib.h"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

//======================================================================================================

TEST(CopyLibTests, isCopyErrorHappened)
{
	EXPECT_FALSE(CopyLib::isCopyErrorHappened());
}

//======================================================================================================

TEST(CopyLibTests, getTempFN)
{
	const auto libFN = CopyLib::getTempFN();
	const std::string tempFNEtalon = "copy_plan_";
	EXPECT_TRUE(libFN == tempFNEtalon);
}

//======================================================================================================

TEST(CopyLibTests, getTempExten)
{
	const auto libExtension = CopyLib::getTempExten();
	const std::string tempExtenEtalon = ".txt";
	EXPECT_TRUE(libExtension == tempExtenEtalon);
}

//======================================================================================================

TEST(CopyLibTests, isEnoughSpace) 
{
	const std::string tempDir = fs::temp_directory_path().string();
	
	const uint64_t small = 1024ULL * 1024ULL * 10ULL; // 10 Mb
	EXPECT_TRUE(CopyLib::isEnoughSpace(tempDir, small));

	const uint64_t big = 1024ULL * 1024ULL * 1024ULL * 1024ULL * 500ULL; // 500 Tb
	EXPECT_FALSE(CopyLib::isEnoughSpace(tempDir, big));
}

//======================================================================================================

TEST(CopyLibTests, removeCopyQueues_OneFile)
{
	const std::string tempDir = fs::temp_directory_path().string();
	const std::string fileName = tempDir + CopyLib::getTempFN() + std::to_string(0) + CopyLib::getTempExten();
	std::ofstream fout(fileName);
	ASSERT_TRUE(fout.is_open());
	if (fout.is_open())
	{
		fout << fileName << std::endl;
		fout.close();
	}

	EXPECT_TRUE(fs::exists(fileName));

	const uint32_t concur = 1U;
	CopyLib::removeCopyQueues(concur);

	EXPECT_FALSE(fs::exists(fileName));
}

TEST(CopyLibTests, removeCopyQueues_ManyFiles)
{
    const uint32_t hardwConcur = 8U;
    std::ofstream* fplan = new (std::nothrow) std::ofstream[hardwConcur];
    ASSERT_TRUE(fplan != nullptr);
	if (fplan != nullptr)
	{
		const std::string tempDir = fs::temp_directory_path().string();
		std::vector<std::string> fileNames;
		fileNames.reserve(hardwConcur);
		std::string path;
		for (size_t i = 0U; i < hardwConcur; i++)
		{
			path = tempDir + CopyLib::getTempFN() + std::to_string(i) + CopyLib::getTempExten();
			fileNames.push_back(path);
		}

		for (size_t i = 0U; i < hardwConcur; i++)
		{
			fplan[i].open(fileNames[i]);
			ASSERT_TRUE(fplan[i].is_open());
		}

		for (size_t i = 0; i < hardwConcur; i++)
		{
			fplan[i] << "Some info" << std::endl;
			fplan[i].close();
		}
		delete[] fplan;

		for (size_t i = 0; i < hardwConcur; i++)
		{
			EXPECT_TRUE(fs::exists(fileNames[i]));
		}

		CopyLib::removeCopyQueues(hardwConcur);

		for (size_t i = 0; i < hardwConcur; i++)
		{
			EXPECT_FALSE(fs::exists(fileNames[i]));
		}
	}
}

//======================================================================================================

TEST(CopyLibTests, copyDirStructure_ManyFolders)
{
	const std::string tempDir = fs::temp_directory_path().string();

	const std::string dir1 = "1/dir1/a";
	const std::string dir2 = "1/dir2/a";
	const std::string dir3 = "2/dir1/a";
	const std::string dir4 = "2/dir2/a";

	const auto origin = tempDir + "origin/";
	const auto originDir1 = origin + dir1;
	const auto originDir2 = origin + dir2;
	const auto originDir3 = origin + dir3;
	const auto originDir4 = origin + dir4;
	
	fs::create_directories(originDir1);
	fs::create_directories(originDir2);
	fs::create_directories(originDir3);
	fs::create_directories(originDir4);

	const auto dest = tempDir + "destination/";
	const auto destDir1 = dest + dir1;
	const auto destDir2 = dest + dir2;
	const auto destDir3 = dest + dir3;
	const auto destDir4 = dest + dir4;

	fs::create_directories(dest);

	const auto pathFirstQueue = tempDir + CopyLib::getTempFN() + "0" + CopyLib::getTempExten();
	std::ofstream fout(pathFirstQueue);
	ASSERT_TRUE(fout.is_open());
	if (fout.is_open())
	{
		fout << origin << std::endl;
		fout << dest << std::endl;
		fout.close();
	}

	ASSERT_TRUE(fs::exists(originDir1));
	ASSERT_TRUE(fs::exists(originDir2));
	ASSERT_TRUE(fs::exists(originDir3));
	ASSERT_TRUE(fs::exists(originDir4));

	CopyLib::copyDirStructure();

	EXPECT_TRUE(fs::exists(destDir1));
	EXPECT_TRUE(fs::exists(destDir2));
	EXPECT_TRUE(fs::exists(destDir3));
	EXPECT_TRUE(fs::exists(destDir4));

	fs::remove_all(origin);
	fs::remove_all(dest);

	EXPECT_FALSE(fs::exists(origin));
	EXPECT_FALSE(fs::exists(dest));
}

//======================================================================================================

TEST(CopyLibTests, logger)
{
	const std::string shortMessage = "Test message";
	const std::string longMessage = "1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 123";
	const std::string emptyMessage;

	EXPECT_FALSE(CopyLib::TLogger::getInstance().logMessage(shortMessage));
	EXPECT_FALSE(CopyLib::TLogger::getInstance().logMessage(longMessage));
	EXPECT_FALSE(CopyLib::TLogger::getInstance().logMessage(emptyMessage));

	CopyLib::TLogger::getInstance().startLogging();
	EXPECT_TRUE(CopyLib::TLogger::getInstance().logMessage(shortMessage));
	EXPECT_TRUE(CopyLib::TLogger::getInstance().logMessage(longMessage));
	EXPECT_FALSE(CopyLib::TLogger::getInstance().logMessage(emptyMessage));
	CopyLib::TLogger::getInstance().finishLogging();

	std::ifstream fin(CopyLib::TLogger::getInstance().getLogFileName());
	ASSERT_TRUE(fin.is_open());
	if (fin.is_open())
	{
		std::string buf;
		std::getline(fin, buf);
		EXPECT_TRUE(buf.find(shortMessage) != std::string::npos);
		std::getline(fin, buf);
		EXPECT_TRUE(buf.find(longMessage) != std::string::npos);
		fin.close();
	}
}

//======================================================================================================

TEST(CopyLibTests, createCopyQueues_False)
{
	const auto tempDir = fs::temp_directory_path().string();
	const auto originDir = tempDir + "origin/";
	const auto destDir = tempDir + "dest/";

	fs::create_directories(originDir);
	fs::create_directories(destDir);

	ASSERT_TRUE(fs::exists(originDir));
	ASSERT_TRUE(fs::exists(destDir));

	bool ret{ false };
	uint64_t scopeSize{ 0U };
	uint64_t fileNum{ 0U };
	
	// (1) hardwConcur = 0
	ret = CopyLib::createCopyQueues(originDir, destDir, 0U, scopeSize, fileNum);
	EXPECT_FALSE(ret);

	// (2) origin is empty
	const uint32_t hardwConcur{ 8U };
	ret = CopyLib::createCopyQueues("", destDir, hardwConcur, scopeSize, fileNum);
	EXPECT_FALSE(ret);

	// (3) dest is empty
	ret = CopyLib::createCopyQueues(originDir, "", hardwConcur, scopeSize, fileNum);
	EXPECT_FALSE(ret);

	// (4) origin is not existing
	const auto notExistingDir = tempDir + "notExistingDir/";
	ASSERT_FALSE(fs::exists(notExistingDir));
	ret = CopyLib::createCopyQueues(notExistingDir, destDir, hardwConcur, scopeSize, fileNum);
	EXPECT_FALSE(ret);

	// (5) dest is not existing
	ret = CopyLib::createCopyQueues(originDir, notExistingDir, hardwConcur, scopeSize, fileNum);
	EXPECT_FALSE(ret);

	// (6) origin is equal dest
	ret = CopyLib::createCopyQueues(originDir, originDir, hardwConcur, scopeSize, fileNum);
	EXPECT_FALSE(ret);

	fs::remove_all(originDir);
	fs::remove_all(destDir);

	EXPECT_FALSE(fs::exists(originDir));
	EXPECT_FALSE(fs::exists(destDir));
}

TEST(CopyLibTests, createCopyQueues_True_OneFileConcur1)
{
	// Add later
}

TEST(CopyLibTests, createCopyQueues_True_OneFileConcur8)
{
	// Add later
}

TEST(CopyLibTests, createCopyQueues_True_ManyFilesConcur1)
{
	// Add later
}

TEST(CopyLibTests, createCopyQueues_True_ManyFilesConcur8)
{
	// Add later
}

//======================================================================================================

TEST(CopyLibTests, worker_OneThreadOneFile)
{
	// Add later 
}

TEST(CopyLibTests, worker_OneThreadManyFiles)
{
	// Add later 
}

TEST(CopyLibTests, worker_TwoThreadsManyFile)
{
	// Add later 
}

// If we want to cover all branches by unit tests in worker fun
// It is needed to add a lot extra tests

//======================================================================================================
