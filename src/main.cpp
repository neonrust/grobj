// An attempt at reading/convert GrimRock .model files

#include <cstring>
#include <iostream>

#include <chrono>
using namespace std::chrono;
#include <filesystem>
#include <variant>
namespace fs = std::filesystem;

#include "grobj/grimrock.h"

using namespace std::literals;

std::variant<std::string, ModelFile> read_model(std::string_view filename);
bool write_obj(std::string filename, const ModelFile &model);

// ----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
	if(argc < 2)
	{
		std::cerr << "Usage: " << argv[0] << " <name.model>\n";
		std::cerr << "Will write mesh to name.obj\n";
		std::exit(1);
	}

	auto first_file = 1;

	// TODO: add some command-line options?

	for(int idx = first_file; idx < argc; ++idx)
	{
		std::string_view filename{ argv[idx], std::strlen(argv[1]) };

		auto dot = filename.rfind('.');
		if(dot == std::string_view::npos)
			dot = filename.size();

		std::string base_name(filename);
		base_name.resize(dot);

		const auto T0 = steady_clock::now();
		auto model_ = read_model(filename);

		fs::path path(filename);

		if(std::holds_alternative<ModelFile>(model_))
		{
			const auto T1 = steady_clock::now();

			const auto &model = std::get<ModelFile>(model_);

			std::cout << "[" << path.filename().generic_string() << "] read " << model.nodes.size() << " nodes  (" << duration_cast<microseconds>(T1 - T0).count() << " µs):\n";
			model.dump(std::cout);

			const auto output_file = base_name + ".obj";
			write_obj(output_file, model);
		}
		else
			std::cerr << "[" << path.filename().generic_string() << "]: " << std::get<std::string>(model_) << '\n';
	}

	return 0;
}

// ----------------------------------------------------------------------------

std::variant<std::string, ModelFile> read_model(std::string_view filename)
{
	auto *fp = std::fopen(filename.data(), "rb");
	if(not fp)
		return "FAILED: "s + std::strerror(errno);

	// make sure the file is closed in all cases
	struct Closer
	{
		~Closer() { std::fclose(fp); }
		std::FILE *fp;
	} closer{ fp };

	return ModelFile::read(fp);
}

// ----------------------------------------------------------------------------

bool write_obj(std::string filename, const ModelFile &model)
{
	(void)filename;
	(void)model;
	std::cout << "write_obj() NOT IMPLEMENTED\n";

	return false;
}
