#include <iostream>
#include "stanford_dragon.h"
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>


pair_vectors сut_by_plane(
	Vertex*& vertices, const int* const faces,
	int& num_points, const int num_faces,
	const float x, const char coord = 'x')
{
	int type_coord;
	switch (coord) // выбор координаты сечения
	{
	case 'x':
		type_coord = 0;
		break;
	case 'y':
		type_coord = 1;
		break;
	case 'z':
		type_coord = 2;
		break;
	default:
		std::cout << "Свич не работает";
		break;
	}

	// множество точек для более быстрого определения общего количества пересечений
	std::unordered_set<std::string> set_of_lines;

	// определение общего количества точек пересечения
	size_t new_vertex_amount = count_new_points(vertices, faces, num_faces, x, type_coord, set_of_lines);

	// словарь, ключами которого являются отрезки пересечения плоскости, представленные в виде строки
	// из двух индексов точек, образующих отрезок, разделенных  символом '_'.
	// В качестве значения словаря является индекс новой точки пересечения
	std::unordered_map<std::string, int> new_coord_dict;
	new_coord_dict.reserve(new_vertex_amount);

	// добавление нужного количества элементов в массивы вершин и граней
	vertices = expand_vertices_memory(vertices, num_points, new_vertex_amount,
		set_of_lines, new_coord_dict,
		x, type_coord);

	std::vector<int> r_faces;
	std::vector<int> l_faces;

	// зарезервируем для каждого количество граней равное изначальному
	r_faces.reserve(num_faces);
	l_faces.reserve(num_faces);

	// деление массива граней
	add_faces(faces, r_faces, l_faces,
		new_coord_dict, vertices,
		num_faces, x, type_coord);
	return { r_faces , l_faces };
}

void print_face_vector(const std::vector<int>& v_faces)
{
	for (int i = 0; i < v_faces.size(); )
	{
		int size = v_faces[i];
		for (int j = 0; j <= size; ++j)
		{
			std::cout << v_faces[i + j] << '\t';
		}
		std::cout << '\n';
		i += size + 1;
	}
}


int main()
{
	const float* points;
	int num_points = getDragonPoints(points); 
	const int* faces;
	int num_faces = getDragonFaces(faces);
	Vertex* vertices = new Vertex[num_points]; // создание массива вершин
	
	// перевод из точек в вершины
	from_points_to_vertex(points, vertices, num_points);

	// Разрез граней на две части
	pair_vectors right_and_left = сut_by_plane(vertices, faces, num_points, num_faces, 0.0f, 'x');
	std::vector<int> right_faces = right_and_left.first;
	std::vector<int> left_faces = right_and_left.second;

	// тесты
	/*
	Vertex* test_vertices = new Vertex [12]{
		{0.0f,0.0f,0.0f,100,100,100}, // 0
		{0.0f,1.0f,0.0f,100,100,100}, // 1
		{0.0f,2.0f,0.0f,100,100,100}, // 2
		{0.0f,3.0f,0.0f,100,100,100}, // 3
		{1.0f,3.0f,0.0f,100,100,100}, // 4
		{2.0f,3.0f,0.0f,100,100,100}, // 5
		{1.0f,2.0f,0.0f,100,100,100}, // 6
		{2.0f,2.0f,0.0f,100,100,100}, // 7
		{1.0f,1.0f,0.0f,100,100,100}, // 8
		{2.0f,1.0f,0.0f,100,100,100}, // 9
		{1.0f,0.0f,0.0f,100,100,100}, // 10
		{2.0f,0.0f,0.0f,100,100,100}  // 11
	};
	int* test_faces = new int [30]{
		4, 0, 1, 8, 10,
		4, 1, 8, 6, 2,
		4, 3, 4, 6, 2,
		4, 7, 6, 4, 5,
		4, 6, 7, 9, 8,
		4, 9, 8, 10, 11
	};

	int test_num_points = 12;
	int test_num_faces = 6;
	float x = 0.5f;
	pair_vectors t_right_and_left = сut_by_plane(test_vertices, test_faces,
		test_num_points, test_num_faces, x, 'x');
	std::vector<int> test_r_faces = t_right_and_left.first;
	std::vector<int> test_l_faces = t_right_and_left.second;
	//print_face_vector(test_r_faces);
	

	// тест на многоугольнике

	Vertex* polygon_vertices = new Vertex[7]{
		{0.0f,5.0f,0.0f,0.5f,0.5f,0.5f}, // 0
		{2.0f,4.0f,0.0f,0.6f,0.6f,0.6f}, // 1
		{1.0f,3.0f,0.0f,0.4f,0.4f,0.4f}, // 2
		{2.0f,2.0f,0.0f,0.5f,0.3f,0.2f}, // 3
		{1.0f,1.0f,0.0f,0.1f,0.2f,0.3f}, // 4
		{2.0f,0.0f,0.0f,0.1f,0.6f,0.8f}, // 5
		{0.0f,0.0f,0.0f,0.4f,0.5f,0.7f}, // 6
	};
	int* polygon_faces = new int[8]{
		7, 0, 1, 2, 3, 4, 5, 6
	};
	int t_num_vert = 7;
	pair_vectors polygon_r_and_l = сut_by_plane(polygon_vertices, polygon_faces,
		t_num_vert, 1, 1.5f, 'x');
	std::vector<int> polygon_r_faces = polygon_r_and_l.first;
	std::vector<int> polygon_l_faces = polygon_r_and_l.second;
	//print_face_vector(polygon_r_faces);
	*/

	draw_model(vertices, num_points, right_faces);

	delete[] vertices;
	
	return 0;
}