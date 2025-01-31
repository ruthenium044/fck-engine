// gen.cpp : Source file for your target.
//

#include "gen.h"
#include <filesystem>
#include <fstream>
#include <stdio.h>
#include <unordered_map>

void make_enum_pretty(std::string &pretty, size_t offset = 0)
{
	constexpr char wildcards[] = {'/', '\\', '.', ' ', '-'};
	for (size_t index = 0; index < sizeof(wildcards); index++)
	{
		char wildcard = wildcards[index];
		size_t it = pretty.find(wildcard, offset);
		if (it != std::string::npos)
		{
			pretty = pretty.erase(it, 1);
			pretty[it] = '_';
			make_enum_pretty(pretty, it);
		}
	}
}

int main(int argc, char **argv)
{
	using namespace std;
	using namespace filesystem;

	constexpr const char *path = GEN_OUTPUT_DIRECTORY_PATH "/gen/gen_assets.h";

	printf("%s at: %s \n", "Running code generator...", path);
	
	struct element {
		filesystem::path path; 
		size_t index;
	};

	unordered_map<filesystem::path, vector<element>> map;

	ofstream output(path);
	output << "#ifndef GEN_ASSETS_INCLUDED\n#define GEN_ASSETS_INCLUDED\n";
	output << "// GENERATED\n";
	output << "constexpr const char* GEN_FILE_PATHS[] = \n";
	output << "{\n";

	size_t index = 0;
	for (const directory_entry &entry : recursive_directory_iterator(GEN_INPUT_DIRECTORY_PATH))
	{
		if (entry.is_regular_file())
		{
			filesystem::path target = entry.path();//relative(entry.path(), GEN_INPUT_DIRECTORY_PATH);
			printf("Found: %ls\n", target.c_str());
			output << '\t' << target << ",\n";
			map[target.extension()].emplace_back(target, index);
			index = index + 1;
		}
	}
	output << "};\n\n";

	for (auto &[extension, files] : map)
	{
		std::string ext = extension.string();
		ext = ext.erase(0, 1);
		output << "enum class gen_assets_" << ext;
		output << '\n';
		output << "{\n";
		for (const auto &element : files)
		{
			filesystem::path copy = element.path;
			while(copy.has_extension()) {
				copy = copy.stem();
			}
			std::string target = copy.string();
			
			make_enum_pretty(target);
			output << '\t' << target << " = " << element.index << ",\n";
		}
		output << "};\n\n";

		output << "constexpr gen_assets_" << ext << " gen_assets_" << ext << "_all" << "[] = {\n";
		for (const auto& element : files)
		{
			filesystem::path copy = element.path;
			while (copy.has_extension()) {
				copy = copy.stem();
			}
			std::string target = copy.string();

			make_enum_pretty(target);
			output << '\t' << "gen_assets_" << ext << "::" << target << ",\n";
		}
		output << "};\n\n";
	}

	for (auto& [extension, files] : map)
	{
		std::string ext = extension.string();
		ext = ext.erase(0, 1);
		output << "inline const char* gen_get_path(gen_assets_" << ext;
		output << " value)";
		output << '\n';
		output << "{\n";
		output << "\treturn GEN_FILE_PATHS[(int) value];\n";
		//for (const auto& element : files)
		//{
		//	filesystem::path copy = element.path;
		//	while (copy.has_extension()) {
		//		copy = copy.stem();
		//	}
		//	std::string target = copy.string();

		//	target[0] = toupper(target[0]);
		//	make_enum_pretty(target);
		//	output << '\t' << target << " = " << element.index << ",\n";
		//}
		output << "};\n\n";
	}
	output << "#endif // !GEN_ASSETS_INCLUDED";

	return 0;
}
