#pragma once

#include <iostream>

#include "grobj/grimrock.h"

using Filter = std::uint32_t;
static constexpr Filter includeEmptyNodes  { 1 << 0 };
static constexpr Filter includeBones       { 1 << 1 };
static constexpr Filter includeTransforms  { 1 << 2 };


void dump(const ModelFile &mf, std::ostream &out, Filter filter);
void dump(const Node &node, std::ostream &out, Filter filter, size_t index) ;
void dump(const Bone &bone, std::ostream &out, Filter filter, size_t index);
void dump(const VertexArray &va, std::ostream &out, Filter filter);
void dump(const MeshSegment &ms, std::ostream &out, Filter filter, size_t index);
void dump(const MeshData &md, std::ostream &out, Filter filter);
void dump(const MeshEntity &me, std::ostream &out, Filter filter);
