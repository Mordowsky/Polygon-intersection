#pragma once
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <crtdbg.h>


using pair_vectors = std::pair<std::vector<int>, std::vector<int>>;

int getDragonFaces(const int*& );
int getDragonPoints(const float*&);

struct Vertex
{
	float x, y, z;
	float red, green, blue;
};

void from_points_to_vertex(const float* const, Vertex* const, const int);
size_t count_new_points(const Vertex* const, const int* const,
	int, const float, const int, std::unordered_set<std::string>&);
Vertex* expand_vertices_memory(Vertex*,
	int&, const size_t,
	const std::unordered_set<std::string>&,
	std::unordered_map<std::string, int>&,
	const float, const int);
void linterp_vertices(const Vertex&, const Vertex&, const float, Vertex&);
void indexes_from_str(std::string, int&, int&);
void print_vertex(const Vertex&);
void add_faces(const int* const, std::vector<int>&, std::vector<int>&,
	const std::unordered_map<std::string, int>&, const Vertex* const,
	const int, const float, const int);
void draw_model(const Vertex* const, const int, std::vector<int>);
