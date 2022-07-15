
#include "gtest/gtest.h"
#include "../../../SourceCode/copylib.h"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

TEST(CopyLibTests, getTempFN)
{
	const auto libFN = CopyLib::getTempFN();
	const std::string tempFNEtalon = "copy_plan_";
	EXPECT_TRUE(libFN == tempFNEtalon);
}

TEST(CopyLibTests, getTempExten)
{
	const auto libExtension = CopyLib::getTempExten();
	const std::string tempExtenEtalon = ".txt";
	EXPECT_TRUE(libExtension == tempExtenEtalon);
}

TEST(CopyLibTests, isEnoughSpace) 
{
	const std::string tempDir = fs::temp_directory_path().string();
	
	const uint64_t small = 1024U * 1024U * 10U; // 10 Mb
	EXPECT_TRUE(CopyLib::isEnoughSpace(tempDir, small));

	const uint64_t big = 1024ULL * 1024ULL * 1024ULL * 1024ULL * 500ULL; // 500 Tb
	EXPECT_FALSE(CopyLib::isEnoughSpace(tempDir, big));
}


TEST(CopyLibTests, removeCopyQueues_OneFile)
{
	const std::string tempDir = fs::temp_directory_path().string();
	const std::string fileName = tempDir + CopyLib::getTempFN() + std::to_string(0) + CopyLib::getTempExten();
	std::ofstream fout(fileName);
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

}

TEST(CopyLibTests, copyDirStructure_ManyFolders)
{
	//copyDirStructure();
}


