#pragma once

#include <filesystem>


#include "Saffron/Base.h"
#include "Saffron/Core/Buffer.h"
#include "Saffron/Core/Window.h"


namespace Se
{
namespace  fs = std::filesystem;
class FileIOManager
{
public:
	struct Filter
	{
		String Description;
		ArrayList<String> Extensions;

		static Filter Empty();

		Filter() = default;
		Filter(String description, ArrayList<String> extensions)
			: Description(std::move(description)),
			Extensions(std::move(extensions))
		{
		}
	};

public:
	static void Init(const Window &window);

	// Implemented per platform
	static Filepath OpenFile(const Filter &filter = Filter::Empty());
	static Filepath SaveFile(const Filter &filter = Filter::Empty());

	static ArrayList<DirectoryEntry> GetFiles(const Filepath &directoryPath, const String &extension = "");
	static size_t GetFileCount(const Filepath &directoryPath, const String &extension = "");

	static size_t Write(const Uint8 *data, size_t size, const Filepath &filepath, bool overwrite = true);
	static size_t Write(const Buffer buffer, const Filepath &filepath, bool overwrite = true);

	static bool FileExists(const Filepath &filepath);

	static bool Copy(const Filepath &source, const Filepath &destination);

private:
	static const Window *m_Window;

};
}

