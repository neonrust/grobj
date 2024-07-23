#include "grobj/dump.h"

// ----------------------------------------------------------------------------

void dump(const ModelFile &mf, std::ostream &out, Filter filter)
{
	out << "  nodes: " << mf.nodes.size() << '\n';
	auto idx = 0u;
	for(const auto &node: mf.nodes)
	{
		dump(node, out, filter, idx);
		++idx;
	}
}

// ----------------------------------------------------------------------------

void dump(const Node &node, std::ostream &out, Filter filter, size_t index)
{
	const auto is_empty = not node.meshEntity.has_value();

	if(not is_empty or (filter & includeEmptyNodes) > 0)
	{
		out << "    node." << index << ": '" << node.name << "'";
		if(node.parent != -1)
			out << " -> node." << node.parent;
		if(not is_empty)
		{
			out << "  MeshEntity\n";
			dump(node.meshEntity.value(), out, filter);
		}
		else
			out << '\n';
	}
}

// ----------------------------------------------------------------------------

void dump(const MeshEntity &me, std::ostream &out, Filter filter)
{
	dump(me.meshData, out, filter);

	if(not me.bones.empty() and (filter & includeBones) > 0)
	{
		out << "      bones: " << me.bones.size() << '\n';
		auto idx = 0u;
		for(const auto &bone: me.bones)
		{
			dump(bone, out, filter, idx);
			++idx;
		}
	}
}

// ----------------------------------------------------------------------------

void dump(const MeshData &md, std::ostream &out, Filter filter)
{
	out << "      vertices: " << md.numVertices << " indices: " << md.indices.size() << " segments: " << md.segments.size() << '\n';
	for(const auto &va: { md.positionArray, md.normalArray, md.tangentArray, md.bitangentArray, md.colorArray, md.texCoordArray[0], md.texCoordArray[1], md.texCoordArray[2], md.texCoordArray[3], md.texCoordArray[4], md.texCoordArray[5], md.texCoordArray[6], md.texCoordArray[7], md.boneArray, md.boneWeightArray})
	{
		if(va)
			dump(va, out, filter);
	}

	auto idx = 0u;
	for(const auto &seg: md.segments)
	{
		dump(seg, out, filter, idx);
		++idx;
	}

}

// ----------------------------------------------------------------------------

void dump(const VertexArray &va, std::ostream &out, [[maybe_unused]] Filter filter)
{
	const char *purposeName;
	switch(va.purpose)
	{
	case Position  : purposeName = "position   "; break;
	case Normal    : purposeName = "normal     "; break;
	case Tangent   : purposeName = "tangent    "; break;
	case Bitangent : purposeName = "bitangent  "; break;
	case Color     : purposeName = "color      "; break;
	case TexCoord0 : purposeName = "uv0        "; break;
	case TexCoord1 : purposeName = "uv1        "; break;
	case TexCoord2 : purposeName = "uv2        "; break;
	case TexCoord3 : purposeName = "uv3        "; break;
	case TexCoord4 : purposeName = "uv4        "; break;
	case TexCoord5 : purposeName = "uv5        "; break;
	case TexCoord6 : purposeName = "uv6        "; break;
	case TexCoord7 : purposeName = "uv7        "; break;
	case BoneIndex : purposeName = "bone       "; break;
	case BoneWeight: purposeName = "bone-weight"; break;
	default: purposeName = "unknown"; break;
	}

	const char *typeName;
	switch(va.dataType)
	{
	case Byte:    typeName = "byte"; break;
	case Int16:   typeName = "int16"; break;
	case Int32:   typeName = "int32"; break;
	case Float32: typeName = "float32"; break;
	default: typeName = "unknown"; break;
	}
	out << "        " << purposeName << " (" << va.dim << "x " << typeName << ")\n";
}

// ----------------------------------------------------------------------------

void dump(const MeshSegment &ms, std::ostream &out, [[maybe_unused]] Filter filter, size_t index)
{
	out << "        segment." << index << ": '" << ms.material << "' " << ms.count << " tris\n";
}

// ----------------------------------------------------------------------------

void dump(const Bone &bone, std::ostream &out, [[maybe_unused]] Filter filter, size_t index)
{
	out << "        bone." << index << " -> node." << bone.nodeIndex << '\n';
}

