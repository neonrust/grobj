// An attempt at reading/convert GrimRock .model files

#include <cstring>
#include <iostream>

#include <chrono>
using namespace std::chrono;
#include <assert.h>
#include <filesystem>
#include <variant>
namespace fs = std::filesystem;

#include "grobj/grimrock.h"
#include "grobj/dump.h"

using namespace std::literals;

std::variant<std::string, ModelFile> read_model(std::string_view filename);
std::string write_obj(std::string filename, const ModelFile &model);

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
		out << "  -o, --output NAME       Write Wavefront OBJ to NAME.obj\n";

		std::exit(exit_code);
	};

	if(argc < 2)
		print_usage();

	bool opt_dumpInfo = false;
	Filter dumpFilter { 0 };
	std::string output_file;

	std::vector<std::string_view> filenames;

	auto parse_option = [&](auto &idx, auto argv) {
		auto arg = argv[idx];

		if(arg == "-h"sv or arg == "--help"sv)
			print_usage(0);
		else if(arg == "-d"sv or arg == "--dump"sv)
			opt_dumpInfo = true;
		else if(arg == "-o"sv or arg == "--output"sv)
		{
			++idx;
			if(idx >= argc)
				print_usage();
			output_file = argv[idx];
		}
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
				dump(model, std::cout, dumpFilter);

			if(not output_file.empty())
			{
				const auto T0 = steady_clock::now();
				write_obj(output_file, model);
				const auto T1 = steady_clock::now();

				std::cout << "[" << path.filename().generic_string() << "] wrote Wavefront OBJ: " << output_file << "  (" << duration_cast<microseconds>(T1 - T0).count() << " µs)\n";
			}
		}
		else
			std::cerr << "[" << path.filename().generic_string() << "]: " << std::get<std::string>(model_) << '\n';
	}

	return 0;
}

// ----------------------------------------------------------------------------

struct Closer
{
	~Closer() { std::fclose(fp); }
	std::FILE *fp;
};

std::variant<std::string, ModelFile> read_model(std::string_view filename)
{
	auto *fp = std::fopen(filename.data(), "rb");
	if(not fp)
		return "FAILED: "s + std::strerror(errno);

	Closer _{ fp };

	return ModelFile::read(fp);
}

// ----------------------------------------------------------------------------

template<typename T>
void write_vertices(std::FILE *fp, const char *vtype, const VertexArray &va, int32 numVertices)
{
	const T *arrData = reinterpret_cast<const T *>(va.rawVertexData.data());
	for(auto idx = 0; idx < numVertices*va.dim; ++idx)
	{
		std::fputs(vtype, fp);
		for(auto comp = 0; comp < va.dim; ++comp, ++idx)
		{
			if constexpr (std::is_same_v<T, byte>)
				std::fprintf(fp, " %u", arrData[idx]);
			else if constexpr (std::is_same_v<T, int16> or std::is_same_v<T, int32>)
				std::fprintf(fp, " %d", arrData[idx]);
			else if constexpr (std::is_same_v<T, float32>)
				std::fprintf(fp, " %f", arrData[idx]);
		}
		std::putc('\n', fp);
	}
}


std::string write_obj(std::string filename, const ModelFile &model)
{
	auto *fp = std::fopen(filename.data(), "wb");
	if(not fp)
		return "FAILED: "s + std::strerror(errno);

	Closer _{ fp };


	std::fprintf(fp, "# %s\n", filename.c_str());

	for(const auto &node: model.nodes)
	{
		if(node.type != 0)
			continue;

		const auto &data = node.meshEntity.value().meshData;

		if(const auto &va = data.positionArray; va)
		{
			// TODO: check if colors exist (should be written on the same "v" line
			assert(va.dataType == Float32);
			write_vertices<float32>(fp, "v", va, data.numVertices);
		}
		if(const auto &va = data.normalArray; va)
		{
			assert(va.dataType == Float32);
			write_vertices<float32>(fp, "vn", va, data.numVertices);
		}
		if(const auto &va = data.texCoordArray[0]; va)
		{
			assert(va.dataType == Float32);
			write_vertices<float32>(fp, "vt", va, data.numVertices);
		}

		for(const auto &segment: data.segments)
		{
			std::fprintf(fp, "o %s\n", segment.material.c_str());
			for(auto tri = 0u; tri < size_t(segment.count); ++tri)
			{
				std::fprintf(fp, "f %d %d %d\n",
							 data.indices[tri*3],
							 data.indices[tri*3 + 1],
							 data.indices[tri*3 + 2]
				);
			}
		}
	}

	return {};
}
