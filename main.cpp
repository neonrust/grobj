// An attempt at reading/convert GrimRock .model files

#include <cstring>
#include <iostream>
#include <cstdint>
#include <assert.h>
#include <optional>
#include <vector>
#include <chrono>
using namespace std::chrono;
#include <filesystem>
namespace fs = std::filesystem;

using namespace std::literals;


// https://www.grimrock.net/modding/model-and-animation-file-formats/

using int32 = std::int32_t;
using byte = std::uint8_t;
using float32 = float;

// A FourCC is a four character code string used for headers
struct FourCC
{
	byte    data[4];

	static FourCC read(std::FILE *fp);
};

struct Vec3
{
	float32 x, y, z;

	static Vec3 read(std::FILE *fp);
};

struct Mat4x3
{
	Vec3    baseX;
	Vec3    baseY;
	Vec3    baseZ;
	Vec3    translation;

	static Mat4x3 read(std::FILE *fp);
};

struct VertexArray
{
	int32 dataType;   // 0 = byte, 1 = int16, 2 = int32, 3 = float32   0 if array unused
	int32 dim;        // dimensions of the data type (2-4)             0 if array unused
	int32 stride;     // byte offset from vertex to vertex             0 if array unused
	std::vector<byte>  rawVertexData;// rawVertexData[numVertices * stride];

	inline operator bool () const { return dataType >= 0 and dim > 0 and stride > 0; }

	static VertexArray read(std::FILE *fp, int32 numVertices);
};

struct MeshSegment
{
	std::string material;      // name of the material defined in Lua script
	int32  primitiveType; // always 2
	int32  firstIndex;    // starting location in the index list
	int32  count;         // number of triangles

	static MeshSegment read(std::FILE *fp);
};

struct MeshData
{
	FourCC      magic;          // "MESH"
	int32       version;        // must be 2
	int32       numVertices;    // number of vertices following
	VertexArray vertexArrays[15];
	std::vector<int32> indices; // indices[numIndices]
	std::vector<MeshSegment> segments; // segmenst[numSegments]
	Vec3        boundCenter;    // center of the bound sphere in model space
	float32     boundRadius;    // radius of the bound sphere in model space
	Vec3        boundMin;       // minimum extents of the bound box in model space
	Vec3        boundMax;       // maximum extents of the bound box in model space

	static MeshData read(std::FILE *fp);
	void dump(std::ostream &out) const;
};

struct Bone
{
	int32 boneNodeIndex;    // index of the node used to deform the object
	Mat4x3 invRestMatrix;   // transform from model space to bone space

	static Bone read(std::FILE *fp);
};

struct MeshEntity
{
	MeshData meshData;
	std::vector<Bone> bones;
	Vec3     emissiveColor;    // deprecated, should be set to 0,0,0
	byte     castShadow;       // 0 = shadow casting off, 1 = shadow casting on

	static MeshEntity read(std::FILE *fp);

	void dump(std::ostream &out) const;
};

struct Node
{
	std::string  name;
	Mat4x3  localToParent;
	int32   parent;        // index of the parent node or -1 if no parent
	int32   type;          // -1 = no entity data, 0 = MeshEntity follows
	std::optional<MeshEntity> meshEntity;

	static Node read(std::FILE *fp);

	void dump(std::ostream &out, size_t index) const;
};

struct ModelFile
{
	FourCC  magic;           // "MDL1"
	int32   version;         // always two
	std::vector<Node> nodes;          // nodes[numNodes]

	static ModelFile read(std::FILE *fp);

	void dump(std::ostream &out) const;
};

std::optional<ModelFile> read_model(std::string_view filename);
bool write_obj(std::string filename, const ModelFile &model);

int main(int argc, char *argv[])
{
	if(argc < 2)
	{
		std::cerr << "Usage: " << argv[0] << " <name.model>\n";
		std::cerr << "Will write mesh to name.obj\n";
		std::exit(1);
	}

	for(int idx = 1u; idx < argc; ++idx)
	{
		std::string_view filename{ argv[idx], std::strlen(argv[1]) };
		// TODO: replace extension
		auto dot = filename.rfind('.');
		if(dot == std::string_view::npos)
			dot = filename.size();

		std::string base_name(filename);
		base_name.resize(dot);

		const auto T0 = steady_clock::now();
		auto model_ = read_model(filename);

		if(model_.has_value())
		{
			fs::path path(filename);
			const auto T1 = steady_clock::now();

			const auto &model = model_.value();

			std::cout << "[" << path.filename().generic_string() << "] read " << model.nodes.size() << " nodes  (" << duration_cast<microseconds>(T1 - T0).count() << " µs):\n";
			model.dump(std::cout);

			const auto output_file = base_name + ".obj";
			write_obj(output_file, model);
		}
		else
			std::cerr << "reading model FAILED: " << filename << '\n';
	}

	return 0;
}

std::optional<ModelFile> read_model(std::string_view filename)
{
	auto *fp = std::fopen(filename.data(), "rb");
	if(not fp)
	{
		// TODO: return error
		// std::cerr << "[" << path.filename().generic_string() << "] failed to open file: " << std::strerror(errno) << '\n';
		return std::nullopt;
	}

	// make sure the file is closed in all cases
	struct Closer
	{
		~Closer() { std::fclose(fp); }
		std::FILE *fp;
	} closer{ fp };

	return ModelFile::read(fp);
}

bool write_obj(std::string filename, const ModelFile &model)
{
	(void)filename;
	(void)model;
	std::cout << "write_obj() NOT IMPLEMENTED\n";

	return false;
}


static byte read_byte(std::FILE *fp)
{
	byte b { 0 };
	auto nread = std::fread(&b, sizeof(byte), 1, fp);
	if(nread != 1)
		throw std::runtime_error("short read (byte)");

	return b;
}

static int32 read_int32(std::FILE *fp)
{
	int32 i32 { 0 };
	auto nread = std::fread(&i32, sizeof(int32), 1, fp);
	if(nread != 1)
		throw std::runtime_error("short read (int32)");

	return i32;
}

static std::vector<int32> read_int32_v(std::FILE *fp, size_t count)
{
	std::vector<int32> i32_v;
	i32_v.resize(count);
	auto nread = std::fread(i32_v.data(), i32_v.size()*sizeof(int32), 1, fp);
	if(nread != 1)
		throw std::runtime_error("short read (int32_vv)");

	return i32_v;
}
static float32 read_float32(std::FILE *fp)
{
	float32 f32 { 0 };
	auto nread = std::fread(&f32, sizeof(float32), 1, fp);
	if(nread != 1)
		throw std::runtime_error("short read (float32)");

	return f32;
}

static std::string read_string(std::FILE *fp)
{
	int32 length = read_int32(fp);
	if(not length)
		return {};

	std::string s;
	s.resize(size_t(length));

	auto nread = std::fread(s.data(), size_t(length), 1, fp);
	if(nread != 1)
		throw std::runtime_error("short read (String)");

	return s;
}


ModelFile ModelFile::read(FILE *fp)
{
	// FourCC  magic;           // "MDL1"
	// int32   version;         // always two
	// int32   numNodes;        // number of nodes following
	// Node    *nodes;          // nodes[numNodes]

	ModelFile mf;

	mf.magic = FourCC::read(fp);
	mf.version = read_int32(fp);
	auto numNodes = read_int32(fp);
	assert(numNodes > 0);
	mf.nodes.reserve(size_t(numNodes));
	for(auto idx = 0u; idx < size_t(numNodes); ++idx)
		mf.nodes.push_back(Node::read(fp));

	return mf;
}

void ModelFile::dump(std::ostream &out) const
{
	out << "  " << nodes.size() << "nodes\n";
	auto idx = 0u;
	for(const auto &node: nodes)
	{
		node.dump(out, idx);
		++idx;
	}
}

FourCC FourCC::read(FILE *fp)
{
	FourCC fcc;
	auto nread = std::fread(&fcc, sizeof(fcc), 1, fp);
	if(nread != 1)
		throw std::runtime_error("short read (FourCC)");

	return fcc;
}

Node Node::read(FILE *fp)
{
	// String  name;
	// Mat4x3  localToParent;
	// int32   parent;        // index of the parent node or -1 if no parent
	// int32   type;          // -1 = no entity data, 0 = MeshEntity follows
	// MeshEntity *meshEntity;

	Node n;
	n.name = read_string(fp);
	n.localToParent = Mat4x3::read(fp);
	n.parent = read_int32(fp);
	n.type = read_int32(fp);
	if(n.type == 0)
		n.meshEntity = MeshEntity::read(fp);

	return n;
}

void Node::dump(std::ostream &out, size_t index) const
{
	out << "  node:" << index << '\n';

	if(meshEntity.has_value())
	{
		out << " MeshEntity\n";
		meshEntity.value().dump(out);
	}
	else
		out << " <empty>\n";
}

MeshEntity MeshEntity::read(FILE *fp)
{
	// MeshData meshdata;
	// int32    numBones;
	// Bone     *bones;
	// Vec3     emissiveColor;    // deprecated, should be set to 0,0,0
	// byte     castShadow;       // 0 = shadow casting off, 1 = shadow casting on

	MeshEntity me;
	me.meshData = MeshData::read(fp);
	auto numBones = read_int32(fp);
	assert(numBones >= 0);
	if(numBones > 0)
	{
		me.bones.reserve(size_t(numBones));
		for(auto idx = 0u; idx < size_t(numBones); ++idx)
			me.bones.push_back(Bone::read(fp));
	}
	me.emissiveColor = Vec3::read(fp);
	me.castShadow = read_byte(fp);

	return me;
}

void MeshEntity::dump(std::ostream &out) const
{
	meshData.dump(out);
	out << "  bones: " << bones.size();
	auto idx = 0u;
	for(const auto &bone: bones)
	{
		(void)bone;
		(void)idx;
		// bone.dump(name, out, idx);
		++idx;
	}
}

Vec3 Vec3::read(FILE *fp)
{
	Vec3 v3;
	v3.x = read_float32(fp);
	v3.y = read_float32(fp);
	v3.z = read_float32(fp);

	return v3;
}

Bone Bone::read(FILE *fp)
{
	// int32 boneNodeIndex;    // index of the node used to deform the object
	// Mat4x3 invRestMatrix;   // transform from model space to bone space

	Bone bn;

	bn.boneNodeIndex = read_int32(fp);
	bn.invRestMatrix = Mat4x3::read(fp);

	return bn;
}

Mat4x3 Mat4x3::read(FILE *fp)
{
	Mat4x3 m43;

	m43.baseX = Vec3::read(fp);
	m43.baseY = Vec3::read(fp);
	m43.baseZ = Vec3::read(fp);
	m43.translation = Vec3::read(fp);

	return m43;
}

MeshData MeshData::read(FILE *fp)
{
	// FourCC      magic;          // "MESH"
	// int32       version;        // must be 2
	// int32       numVertices;    // number of vertices following
	// VertexArray vertexArrays[15];
	// int32       numIndices;     // number of triangle indices following
	// int32       *indices;       // indices[numIndices]
	// int32       numSegments;    // number of mesh segments following
	// MeshSegment *segments;      // segmenst[numSegments]
	// Vec3        boundCenter;    // center of the bound sphere in model space
	// float32     boundRadius;    // radius of the bound sphere in model space
	// Vec3        boundMin;       // minimum extents of the bound box in model space
	// Vec3        boundMax;       // maximum extents of the bound box in model space

	MeshData  md;
	md.magic = FourCC::read(fp);
	md.version = read_int32(fp);
	md.numVertices = read_int32(fp);
	for(auto idx = 0; idx < 15; ++idx)
		md.vertexArrays[idx] = VertexArray::read(fp, md.numVertices);

	auto numIndices = read_int32(fp);
	if(numIndices > 0)
		md.indices = read_int32_v(fp, size_t(numIndices));

	auto numSegments = read_int32(fp);
	if(numSegments > 0)
	{
		md.segments.reserve(size_t(numSegments));
		for(auto idx = 0u; idx < size_t(numSegments); ++idx)
			md.segments.push_back(MeshSegment::read(fp));
	}

	md.boundCenter = Vec3::read(fp);
	md.boundRadius = read_float32(fp);
	md.boundMin = Vec3::read(fp);
	md.boundMax = Vec3::read(fp);

	return md;
}

void MeshData::dump(std::ostream &out) const
{
	out << "  vertices: " << numVertices << '\n';
}

VertexArray VertexArray::read(FILE *fp, int32 numVertices)
{
	// int32 dataType;   // 0 = byte, 1 = int16, 2 = int32, 3 = float32   0 if array unused
	// int32 dim;        // dimensions of the data type (2-4)             0 if array unused
	// int32 stride;     // byte offset from vertex to vertex             0 if array unused
	// byte  *rawVertexData;// rawVertexData[numVertices * stride];

	VertexArray va;
	va.dataType = read_int32(fp);
	va.dim = read_int32(fp);
	va.stride = read_int32(fp);
	if(va.dataType >= 0 and va.dim > 0 and va.stride > 0)
	{
		const auto dataSize = size_t(numVertices * va.stride);
		va.rawVertexData.resize(dataSize);
		auto nread = std::fread(va.rawVertexData.data(), dataSize, 1, fp);
		if(nread != 1)
			throw std::runtime_error("short read (VertexArray)");
	}

	return va;
}

MeshSegment MeshSegment::read(FILE *fp)
{
	// String material;      // name of the material defined in Lua script
	// int32  primitiveType; // always 2
	// int32  firstIndex;    // starting location in the index list
	// int32  count;         // number of triangles

	MeshSegment ms;
	ms.material = read_string(fp);
	ms.primitiveType = read_int32(fp);
	assert(ms.primitiveType == 2);
	ms.firstIndex = read_int32(fp);
	assert(ms.firstIndex >= 0);
	ms.count = read_int32(fp);
	assert(ms.count > 0);

	return ms;
}
