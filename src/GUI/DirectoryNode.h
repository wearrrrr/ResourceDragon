#include <string>
#include <vector>
#include <filesystem>

struct DirectoryNode
{
	std::string FullPath;
	std::string FileName;
	std::vector<DirectoryNode> Children;
	bool IsDirectory;
};

void RecursivelyAddDirectoryNodes(DirectoryNode& parentNode, std::filesystem::directory_iterator directoryIterator);
DirectoryNode CreateDirectryNodeTreeFromPath(const std::filesystem::path& rootPath);