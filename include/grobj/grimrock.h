#pragma once

#include <cstdint>
#include <cstdio>
#include <vector>
#include <optional>
#include <string>


using byte = std::uint8_t;
using int16 = std::int16_t;
using int32 = std::int32_t;
using float32 = float;


// https://www.grimrock.net/modding/model-and-animation-file-formats/

// A FourCC is a four character code string used for headers
struct FourCC
{
	union {
		byte data[4];
		std::uint32_t raw;
	};

	inline FourCC() : raw(0) {}
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

enum ArrayDataType
{
	Byte = 0,
	Int16 = 1,
	Int32 = 2,
	Float32 = 3,
};

enum ArrayPurpose
{
	Position    = 0,
	Normal      = 1,
	Tangent     = 2,
	Bitangent   = 3,
	Color       = 4,
	TexCoord0   = 5,
	TexCoord1   = 6,
	TexCoord2   = 7,
	TexCoord3   = 8,
	TexCoord4   = 9,
	TexCoord5   = 10,
	TexCoord6   = 11,
	TexCoord7   = 12,
	BoneIndex   = 13,
	BoneWeight  = 14,
};
static constexpr auto ArrayCount = BoneWeight + 1; // must always be <last enum> + 1

struct VertexArray
{
	ArrayPurpose      purpose;
	ArrayDataType     dataType;   //                                               Byte if array unused
	int32             dim;        // dimensions of the data type (2-4)             0 if array unused
	int32             stride;     // byte offset from vertex to vertex             0 if array unused
	std::vector<byte> rawVertexData;// rawVertexData[numVertices * stride];

	inline VertexArray() : purpose(Position), dataType(Byte), dim(0), stride(0) {}

	inline operator bool () const { return dataType >= 0 and dim > 0 and stride > 0; }

	static VertexArray read(std::FILE *fp, int32 numVertices);
};

struct MeshSegment
{
	std::string material;      // name of the material defined in Lua script
	int32       primitiveType; // always 2
	int32       firstIndex;    // starting location in the index list
	int32       count;         // number of triangles

	static MeshSegment read(std::FILE *fp);
};

struct MeshData
{
	FourCC         magic;          // "MESH"
	int32          version;        // must be 2
	int32          numVertices;    // number of vertices following
	// TODO: use std::optional ?
	VertexArray    positionArray;
	VertexArray    normalArray;
	VertexArray    tangentArray;
	VertexArray    bitangentArray;
	VertexArray    colorArray;
	VertexArray    texCoordArray[8];
	VertexArray    boneArray;
	VertexArray    boneWeightArray;
	std::vector<int32>       indices; // indices[numIndices]
	std::vector<MeshSegment> segments; // segmenst[numSegments]
	Vec3           boundCenter;    // center of the bound sphere in model space
	float32        boundRadius;    // radius of the bound sphere in model space
	Vec3           boundMin;       // minimum extents of the bound box in model space
	Vec3           boundMax;       // maximum extents of the bound box in model space

	inline MeshData() : version(0), numVertices(0), boundRadius(0) {}
	static MeshData read(std::FILE *fp);
};

struct Bone
{
	int32  nodeIndex;       // index of the node used to deform the object
	Mat4x3 invRestMatrix;   // transform from model space to bone space

	static Bone read(std::FILE *fp);
};

struct MeshEntity
{
	MeshData          meshData;
	std::vector<Bone> bones;
	Vec3              emissiveColor;    // deprecated, should be set to 0,0,0
	byte              castShadow;       // 0 = shadow casting off, 1 = shadow casting on

	static MeshEntity read(std::FILE *fp);
};

enum NodeType
{
	TypeEmpty      = -1,
	TypeMeshEntity = 0
};

struct Node
{
	std::string               name;
	Mat4x3                    localToParent;
	int32                     parent;        // index of the parent node or -1 if no parent
	NodeType                  type;
	std::optional<MeshEntity> meshEntity;

	inline Node() : parent(-1), type(TypeEmpty) {}
	static Node read(std::FILE *fp);
};

struct ModelFile
{
	FourCC            magic;           // "MDL1"
	int32             version;         // always two
	std::vector<Node> nodes;          // nodes[numNodes]

	inline ModelFile() : version(0) {}
	static ModelFile read(std::FILE *fp);
};

