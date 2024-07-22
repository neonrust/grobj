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
	auto print_usage = [prg=argv[0]](int exit_code=1) {

		auto &out = exit_code == 0? std::cout: std::cerr;

		out << "Usage: " << prg << " <options> <name.model>\n";
		out << "Where <options> are:\n";
		out << "  -d, --dump              Dump model information to stdout\n";
		out << "  -E, --include-empty     Dump also empty nodes\n";
		out << "  -B, --include-bones     Dump also bones\n";
		out << "  -M, --transforms        Dump transforms of various entries\n";
		out << "  -w, --write             Write Wavefront OBJ to <name.obj>\n";

		std::exit(exit_code);
	};

	if(argc < 2)
		print_usage();

	bool opt_dumpInfo = false;
	bool opt_writeObj = false;
	Filter dumpFilter { 0 };

	std::vector<std::string_view> filenames;

	auto parse_option = [&](auto &idx, auto argv) {
		auto arg = argv[idx];

		if(arg == "-h"sv or arg == "--help"sv)
			print_usage(0);
		else if(arg == "-d"sv or arg == "--dump"sv)
			opt_dumpInfo = true;
		else if(arg == "-w"sv or arg == "--write"sv)
			opt_writeObj = true;
		else if(arg == "-E"sv or arg == "--include-empty"sv)
			dumpFilter |= includeEmptyNodes;
		else if(arg == "-B" or arg == "--include-bones"sv)
			dumpFilter |= includeBones;
		else if(arg == "-M" or arg == "--transforms"sv)
			dumpFilter |= includeTransforms;
		else
			return false;

		return true;
	};

	for(auto idx = 1; idx < argc; ++idx)
	{
		if(argv[idx][0] == '\0')
			continue;

		if(argv[idx][0] == '-')
		{
			if(not parse_option(idx, argv))
				print_usage();
		}
		else
			filenames.push_back(std::string_view{ argv[idx], std::strlen(argv[idx]) });
	}

	for(const auto &filename: filenames)
	{
		const auto T0 = steady_clock::now();
		auto model_ = read_model(filename);

		fs::path path(filename);

		if(std::holds_alternative<ModelFile>(model_))
		{
			const auto T1 = steady_clock::now();

			const auto &model = std::get<ModelFile>(model_);

			std::cout << "[" << path.filename().generic_string() << "] read " << model.nodes.size() << " nodes  (" << duration_cast<microseconds>(T1 - T0).count() << " µs):\n";
			if(opt_dumpInfo)
				model.dump(std::cout, dumpFilter);

			if(opt_writeObj)
			{
				std::string base_name(filename);
				auto dot = base_name.rfind('.');
				if(dot == std::string_view::npos)
					dot = base_name.size();
				base_name.resize(dot);
				const auto output_file = base_name + ".obj";
				write_obj(output_file, model);
			}
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
