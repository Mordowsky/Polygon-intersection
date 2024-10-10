#include <iostream>
#include "stanford_dragon.h"
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <vector>
#include <algorithm>
#include <crtdbg.h>


// удобная печать координат
void print_vertex(const Vertex& vertex)
{
	std::cout << "x = " << vertex.x
		<< "  y= " << vertex.y << "  z= " << vertex.z << std::endl;
}

// строка для более удобного поиска пересечений, прдставляет собой 2 индекса соседних точек грани
// по сути пересечение лежит на данной прямой между двумя этими точками, потому так однозначно
// можно определить, повторилась ли точка пересечения снова.
std::string line_in_string(const int index1, const int index2)
{
	if (index1 < index2)
	{
		return std::to_string(index1) + "_" + std::to_string(index2);
	}
	else
	{
		return std::to_string(index2) + "_" + std::to_string(index1);
	}
}

void indexes_from_str(std::string str, int& index1, int& index2)
{
	size_t separator_pos = str.find('_');
	std::string str1 = str.substr(0, separator_pos);
	std::string str2 = str.substr(separator_pos + 1);
	index1 = std::stoi(str1);
	index2 = std::stoi(str2);
}

// Линейная интерполяция параметров, где параметр interpolation есть относительное положение между точками
template <typename T> T linterp(const T first_arg, const T second_arg, const float interpolation)
{
	_ASSERT((interpolation >= 0.0f) && (interpolation <= 1.0f));
	return first_arg + static_cast<T>(interpolation * (second_arg - first_arg));
}

// Линейная интерполяция параметров у вершин
void linterp_vertices(const Vertex& fist_vertex,
	const Vertex& second_vertex,
	const float interpolation,
	Vertex& where_to_save)
{
	where_to_save.x = linterp(fist_vertex.x, second_vertex.x, interpolation);
	where_to_save.y = linterp(fist_vertex.y, second_vertex.y, interpolation);
	where_to_save.z = linterp(fist_vertex.z, second_vertex.z, interpolation);
	where_to_save.red = linterp(fist_vertex.red, second_vertex.red, interpolation);
	where_to_save.green = linterp(fist_vertex.green, second_vertex.green, interpolation);
	where_to_save.blue = linterp(fist_vertex.blue, second_vertex.blue, interpolation);
}


// взятие координаты по ее коду из ссылки на вершину
float get_coord(const Vertex* const p_vertex, const int type_coord)
{
	_ASSERT(type_coord >= 0 && type_coord < 3);
	return (&p_vertex->x)[type_coord];
	/*
	switch (type_coord)
	{
	case 0:
		return p_vertex->x;
	case 1:
		return p_vertex->y;
	case 2:
		return p_vertex->z;
	}
	*/
}


// выдача точки пересечения
void linterp_for_intersection(const Vertex* const p_vertex1,
	const Vertex* const p_vertex2,
	const int type_coord, const float x,
	Vertex& where_to_save)
{
	float interpolation = (x - get_coord(p_vertex1, type_coord));
	interpolation = interpolation / (get_coord(p_vertex2, type_coord) - get_coord(p_vertex1, type_coord));
	
	// сортировка по координате, чтобы не было зависимости от направления
	Vertex vertex1;
	Vertex vertex2;
	if (get_coord(p_vertex1, type_coord) > get_coord(p_vertex2, type_coord))
	{
		vertex1 = *p_vertex2;
		vertex2 = *p_vertex1;
	}
	else
	{
		vertex1 = *p_vertex1;
		vertex2 = *p_vertex2;
	}

	linterp_vertices(vertex1, vertex2, interpolation, where_to_save);
	
	switch (type_coord)
	{
	case 0:
		where_to_save.x = x;
		break;
	case 1:
		where_to_save.y = x;
		break;
	case 2:
		where_to_save.z = x;
		break;
	default:
		std::cout << "Wrong type_cord. x is defualt";
		where_to_save.x = x;
		break;
	}
}

// условие пересечения
bool intersection_condition(const Vertex* const p_vertex1, const Vertex* const p_vertex2,
	const int type_coord, const float x)
{
	return (((get_coord(p_vertex1, type_coord) < x) and (get_coord(p_vertex2, type_coord) > x))
		or
		((get_coord(p_vertex1, type_coord) > x) and (get_coord(p_vertex2, type_coord) < x)));
}

// Расчет вектора нормали грани по ссылке на грань
std::vector<float> face_normal(const int* const face, const Vertex* const vertices)
{
	Vertex v0 = vertices[face[1]];
	Vertex v1 = vertices[face[2]];
	Vertex v2 = vertices[face[3]];
	std::vector<float> e10{ v0.x - v1.x,v0.y - v1.y, v0.z - v1.z };
	std::vector<float> e12{ v2.x - v1.x,v2.y - v1.y, v2.z - v1.z };
	return std::vector<float> {
		    e10[1] * e12[2] - e10[2] * e12[1],
			-(e10[0] * e12[2] - e10[2] * e12[0]),
			e10[0] * e12[1] - e10[1] * e12[0]
	};
}

// по какой из координат лучше проводить сравнение положения точек
int type_coord_to_compare(const int type_coord, const int* const face, const Vertex* const vertices)
{
	std::vector<float> normal = face_normal(face, vertices);
	switch (type_coord)
	{
	case 0:
		return (fabs(normal[1]) <= fabs(normal[2])) ? 1 : 2;
	case 1:
		return (fabs(normal[0]) <= fabs(normal[2])) ? 0 : 2;
	case 2:
		return (fabs(normal[1]) <= fabs(normal[0])) ? 1 : 0;
	default:
		break;
	}
}


// количество пересечений грани. face - указатель на начало подмассива грани, 
// type_cord - код для взятия нужной координаты
void num_of_face_intersect(const Vertex* const vertices, const int* const face,
	const float x, const int type_coord, std::unordered_set<std::string>& set_lines)
{
	int num_face_points = *face;
	// проверка пересечения ежду первой и последней вершиной
	if (intersection_condition(vertices + face[1],
		vertices + face[num_face_points], type_coord, x))
	{
		// добавление линии в множество
		set_lines.insert(line_in_string(face[1], face[num_face_points]));
	}

	// проход  до предпоследнего, с единицы, потому что первое число - число вершин
	for (int i = 1; i < num_face_points; ++i)
	{
		if (intersection_condition(vertices + face[i],
			vertices + face[i+1], type_coord, x))
		{
			// добавление линии в множество
			set_lines.insert(line_in_string(face[i], face[i+1]));
		}
	}
}

// новое количество граней: старые + поделенные
size_t count_new_points(const Vertex* const vertices, const int* const faces,
	const int num_faces, const float x, const int type_coord, std::unordered_set<std::string>& set_lines)
{
	int face_index = 0;
	for (int i = 0; i < num_faces; ++i)
	{
		num_of_face_intersect(vertices, faces + face_index, x, type_coord, set_lines);
		face_index += faces[face_index] + 1;
	}
	return set_lines.size();
}

// перевод из точек (points) в вершины (Vertex)
void from_points_to_vertex(const float* const points, Vertex* const vertices, const int num_points)
{
	for (int i = 0; i < num_points; ++i)
	{
		vertices[i] = { points[i * 3], points[i * 3 + 1], points[i * 3 + 2],
			0.5f + (points[i * 3]/250.0f), 0.5f + (points[i * 3 +1] / 250.0f), 0.5f + (points[i * 3 + 2] / 250.0f) };
		// для цвета пока используются дефолтные значения
	}
}

// Расширение выделенной памяти под хранение вершин и добавление туда новых точек
Vertex* expand_vertices_memory(Vertex* vertices,
	int& old_vertex_amount, const size_t new_vertex_amount,
	const std::unordered_set<std::string>& set_lines,
	std::unordered_map<std::string, int>& coord_dict,
	const float x, const int type_coord)
{
	Vertex* new_vertices = new Vertex[old_vertex_amount + new_vertex_amount];
	for (int i = 0; i < old_vertex_amount; ++i)
	{
		new_vertices[i] = vertices[i];
	}
	int index = old_vertex_amount;
	int ind1, ind2;
	for (std::unordered_set<std::string>::iterator itr = set_lines.begin(); itr != set_lines.end(); ++itr)
	{
		indexes_from_str(*itr, ind1, ind2);
		linterp_for_intersection(new_vertices + ind1, new_vertices + ind2,
			type_coord, x, new_vertices[index]);
		coord_dict[*itr] = index++;
	}
	old_vertex_amount += new_vertex_amount;
	delete[] vertices;
	vertices = nullptr;
	return new_vertices;
}

// Добавление точек в контур
void update_temp_contour(std::vector<std::vector<int>>& temp_faces, std::vector<int>& temp_contour,
	const std::unordered_map<std::string, int>& coord_dict, const Vertex* const vertices,
	const int num_face_points, const int* const face, const int i,
	const int type_coord, const float x)
{
	int i_previous = (i == 1) ? num_face_points : i - 1;

	// проверка пересечения текущей и предыдущей точки
	if (intersection_condition(vertices + face[i_previous],
		vertices + face[i], type_coord, x))
	{
		// Добавление точки пересечения в контур
		int new_vetex_index = coord_dict.at(line_in_string(face[i], face[i_previous]));
		temp_contour.push_back(new_vetex_index);

		// Добавление контура в массив контуров
		temp_faces.push_back(temp_contour);
		temp_contour.clear();

		// Добавление точки пересечения и текущей точки в начало следующего контура
		temp_contour.push_back(new_vetex_index);
		temp_contour.push_back(face[i]);
		if (i == 1) temp_faces.push_back(temp_contour);
	}

	// если текущая вершина находится в плоскости сечения
	else if ((get_coord(vertices + face[i], type_coord) == x) or 
		(get_coord(vertices + face[i_previous], type_coord) == x))
	{
		if (get_coord(vertices + face[i_previous], type_coord) != x)
		{
			// предыдущая точка контура находится вне плоскости сечения
			
			temp_contour.push_back(face[i]);
			
			// Добавление контура в массив контуров, т.к. точка для данных условий является конечной
			// для контура
			temp_faces.push_back(temp_contour);
			temp_contour.clear();
		}
		else
		{
			// предыдущая точка контура находится в плоскости сечения
			if (get_coord(vertices + face[i], type_coord) != x)
			{
				// текущая точка вне плоскости сечения
				temp_contour.push_back(face[i_previous]);
				temp_contour.push_back(face[i]);
				if (i == 1) temp_faces.push_back(temp_contour);
			}
			// 4-ое условие не смотрим (когда предыдущая и текущая точки контура лежат в плоскости),
			// т.к. в данном случае предыдущий контур закончился, а новый должен лежать вне плоскости
		}
	}
	else
	{
		// Если ни точек пересечения, ни находящихся в плоскости сечения вершин нет,
		// то просто добавляем точку в контур
		temp_contour.push_back(face[i]);
		if (i == 1) temp_faces.push_back(temp_contour);
	}
}

// компаратор для сортировки
bool compare(const std::pair<int, float>& ind1, const std::pair<int, float>& ind2)
{
	return ind1.second < ind2.second;
}

int make_points_in_plane_vector(const std::vector<std::vector <int>>& temp_faces,
	const int coord_to_compare, const Vertex* const vertices, 
	std::vector< std::pair<int, float>>& points_in_plane)
{
	std::pair<int, float > pair_index_value;
	int size_vector = temp_faces.size();
	int first_index;
	int last_index;
	for (int i = 0; i < size_vector; ++i)
	{
		first_index = temp_faces[i].front();
		last_index = temp_faces[i].back();
		if (i == 0)
		{
			pair_index_value = { first_index, get_coord(vertices + first_index, coord_to_compare) };
			points_in_plane.push_back(pair_index_value);
		}	
		else if (temp_faces[i - 1].back() != first_index)
		{
			pair_index_value = { first_index, get_coord(vertices + first_index, coord_to_compare) };
			points_in_plane.push_back(pair_index_value);
		}
		pair_index_value = { last_index, get_coord(vertices + last_index, coord_to_compare) };
		points_in_plane.push_back(pair_index_value);
	}
	if (points_in_plane.front().first == points_in_plane.back().first)
		points_in_plane.pop_back();
	return points_in_plane.size();
}

// Добавление в общий массив граней с распределением на правый и левый
// правая - с права от плоскости счеения, т.е. с большей координатой, по которой идет сечение
// левая - меньше координаты плоскости сечения
void add_to_r_or_l(const std::vector<int>& contour, std::vector<int>& r_faces, std::vector<int>& l_faces, 
	const Vertex* const vertices, const int type_coord, const float x)
{
	int size = contour.size();
	if (get_coord(vertices + contour[1], type_coord) > x)
	{
		r_faces.push_back(size);
		r_faces.insert(r_faces.end(), contour.begin(), contour.end());
	}
	else
	{
		l_faces.push_back(size);
		l_faces.insert(l_faces.end(), contour.begin(), contour.end());
	}
}

// разрез одной грани и добавка новых в массивы левых и правых граней
void add_face(const int* const face, std::vector<int>& r_faces, std::vector<int>& l_faces,
	const std::unordered_map<std::string, int>& coord_dict, const Vertex* const vertices,
	const float x, const int type_coord, std::vector<std::vector <int>>& temp_faces)
{
	int num_face_points = *face;
	int max_intersections = (num_face_points % 2 == 0) ? num_face_points : num_face_points - 1;
	int max_face_points = num_face_points + max_intersections / 2;

	std::vector<int> temp_contour;
	bool first_not_in_plane = (get_coord(vertices + face[1], type_coord) != x);
	if (first_not_in_plane) temp_contour.push_back(face[1]);
	temp_contour.reserve(max_face_points);

	for (int i = 2; i <= num_face_points; ++i)
	{
		update_temp_contour(temp_faces, temp_contour, coord_dict, vertices,
			num_face_points, face, i, type_coord, x);
	}
	update_temp_contour(temp_faces, temp_contour, coord_dict, vertices,
		num_face_points, face, 1, type_coord, x);

	// если первая точка не в плоскости, то 1-ый и последний контур это один большой контур, их необходимо
	// соединить воединио

	if ((first_not_in_plane) and (temp_faces.size() != 1))
	{
		temp_faces.back().insert(temp_faces.back().end(),
			temp_faces[0].begin() + 1, temp_faces[0].end());
		temp_faces.erase(temp_faces.begin());
	}

	// for testing
	/*
	for (auto contour : temp_faces)
	{
		for (auto point : contour)
		{
			std::cout << point << '\t';
		}
		std::cout << '\n';
	}
	*/
	
	int contours_amount = temp_faces.size();
	// крайние точки контуров лежат в плоскости сечения. Некоторые контуры можно соединить,
	// если один находится под другим

	if (contours_amount == 1)
	{
		temp_faces.front().pop_back();
		add_to_r_or_l(temp_faces.front(), r_faces, l_faces, vertices, type_coord, x);
		return;
	}
	// ось координат по которой будет сортироваться список точек лежащих в плоскости сечения
	int coord_to_compare = type_coord_to_compare(type_coord, face, vertices);

	// вектор пар из индекса точки и ее координаты (на выбранной ранее оси)
	std::vector<std::pair<int, float>> points_in_plane;
	int size_points_in_plane = make_points_in_plane_vector(temp_faces,
		coord_to_compare, vertices, points_in_plane);

	// отсортированный по возрастанию выбранной координаты вектор точек лежащих в плоскости
	std::sort(points_in_plane.begin(), points_in_plane.end(), compare);

	int in_plane_index;
	int index_to_connect;
	int max_edge;
	int min_edge;
	int max_edge_compare;
	int min_edge_compare;
	for (int i = 0; i < contours_amount; ++i)
	{
		// Выявляем минимальную и максимальную грань контура
		max_edge = temp_faces[i].front();
		min_edge = temp_faces[i].back();
		if (get_coord(vertices + min_edge, coord_to_compare) > 
			get_coord(vertices + max_edge, coord_to_compare))
			std::swap(max_edge, min_edge);

		// Поиск у выбранной грани индекса точки самой низкой вершины в отсортированном массиве 
	    // точек лежащих в плсоксти
		for (int j = 0; j < size_points_in_plane; ++j)
		{
			if (points_in_plane[j].first == min_edge)
			{
				in_plane_index = j;
				break;
			}
		}
	
		// если в отсортированном массиве между самой высокой точкой лежащей в плоскости сечения
		// и самой низкой нет никакх точек, то контур цельный, к нему добавлять ничего не нужно.
		if (points_in_plane[in_plane_index + 1].first != max_edge)
		{
			while (points_in_plane[in_plane_index + 1].first != max_edge)
			{
				for (int j = 0; j < contours_amount; ++j)
				{
					// нужно найти такой контур, у которого минимальный индекс следующий от рассматриваемого
					// Поиск контура имеющего на краях points_in_plane[in_plane_index + 1] или
					// points_in_plane[in_plane_index + 2]
					max_edge_compare = temp_faces[j].front();
					min_edge_compare = temp_faces[j].back();
					if (get_coord(vertices + min_edge_compare, coord_to_compare) >
						get_coord(vertices + max_edge_compare, coord_to_compare))
						std::swap(max_edge_compare, min_edge_compare);
					if (points_in_plane[in_plane_index + 1].first == min_edge_compare)
					{
						index_to_connect = j;

						// переход к следующей паре краев контуров
						while (points_in_plane[in_plane_index].first != max_edge_compare)
							++in_plane_index;
					}	
				}
				if (index_to_connect > i)
				{
					// если добавляемый подконтур находится после основного, то добавляем в конец
					temp_faces[i].insert(temp_faces[i].end(),
						temp_faces[index_to_connect].begin(), temp_faces[index_to_connect].end());
					temp_faces.erase(temp_faces.begin() + index_to_connect);

					// Т.к. вырезал элмемент, то его длина теперь меньше
					--contours_amount;
				}
				else
				{
					// иначе в начало
					temp_faces[i].insert(temp_faces[i].begin(),
						temp_faces[index_to_connect].begin(), temp_faces[index_to_connect].end());
					temp_faces.erase(temp_faces.begin() + index_to_connect);
					// Т.к. вырезал элмемент, то его длина теперь меньше, а т.к. index_to_connect < i,
					// то надо сместить значение i на единицу вниз
					--contours_amount;
					--i;
				}
			}
		}
	}
	for (int i = 0; i < temp_faces.size(); ++i)
	{
		add_to_r_or_l(temp_faces[i], r_faces, l_faces, vertices, type_coord, x);
	}
}

// разрез всех граней
void add_faces(const int* const faces, std::vector<int>& r_faces, std::vector<int>& l_faces,
	const std::unordered_map<std::string, int>& coord_dict, const Vertex* const vertices, 
	const int num_faces, const float x, const int type_coord)
{
	std::vector<std::vector <int>> temp_faces;
	temp_faces.reserve(3 + 2/1);
	int face_index = 0;
	for (int i = 0; i < num_faces; ++i)
	{
		add_face(faces + face_index, r_faces, l_faces, coord_dict, vertices,
			x, type_coord, temp_faces);
		face_index += faces[face_index] + 1;
		temp_faces.clear();
	}
}
