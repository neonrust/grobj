#pragma once

#include <cstdint>
#include <iostream>
#include <vector>
#include <optional>


using int32 = std::int32_t;
using byte = std::uint8_t;
using float32 = float;

// https://www.grimrock.net/modding/model-and-animation-file-formats/


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
