#include "/public/read.h"
#include "/public/colors.h"
#include <cmath>
#include <limits>
#include <algorithm>
using namespace std;

vector<vector<double>> z_buffer{};
//Very time we're going to draw a new frame, call this function to clear the z-buffer
void clear_z_buffer() {
	auto [rows,cols] = get_terminal_size(); //Make sure we're returning an int in range
	z_buffer.clear();
	z_buffer.resize(rows);
	for (int i = 0; i < rows; i++) 
		z_buffer.at(i).resize(cols,numeric_limits<double>::max());
}

struct Point {
	int row=0,col=0; //Screen coordinates - z is how far it is from the camera
	double z=0; 
	uint8_t r=255,g=255,b=255;
	friend istream& operator>>(istream &ins,Point &rhs) {
		ins >> rhs.col >> rhs.row >> rhs.z;
		return ins;
	}
	friend ostream& operator<<(ostream &outs, const Point &rhs) {
		outs << '(' << rhs.col << ',' << rhs.row << ')' << " z: " << rhs.z;
		return outs;
	}
};
vector<Point> vertex_buffer; //A collection of all points in our models

struct LineSeg {
	//Point start,end; //Should be between 0,0 and our screen resolution
	size_t start_index = 0, end_index = 0; //Holds the index in the buffer to our start and end points

	//Returns the cross product of start->end and start->p
	double crossproduct(const Point &p) {
		Point start = vertex_buffer.at(start_index);
		Point end = vertex_buffer.at(end_index);
		int x1 = end.col - start.col;
		int y1 = end.row - start.row;
		int x2 = p.col - start.col;
		int y2 = p.row - start.row;
		return x1*y2-x2*y1;
	}

	//This could probably be cleaned up a bit to eliminate redundant code
	void raster() { //Draw all points between start and end
		Point start = vertex_buffer.at(start_index);
		Point end = vertex_buffer.at(end_index);
		int horiz_distance = end.col - start.col;
		int vert_distance = end.row - start.row;
		if (abs(horiz_distance) >= abs(vert_distance)) {
			if (start.col > end.col) {
				swap(start.col,end.col);
				swap(start.row,end.row);
			}
			double slope = double(vert_distance)/horiz_distance; //Slope of the line
			for (int i = 0; i <= abs(horiz_distance); i++) {
				int col = start.col + i;
				int row = start.row + i*slope;
				double alpha = double(i)/horiz_distance;
				if (horiz_distance < 0) alpha = 1-abs(alpha);
				uint8_t r = lerp(start.r,end.r,alpha);
				uint8_t g = lerp(start.g,end.g,alpha);
				uint8_t b = lerp(start.b,end.b,alpha);
				double z = lerp(start.z,end.z,alpha);
				double buffered_z = z_buffer.at(row).at(col); //See what depth is already there
				if (z < buffered_z) {
					movecursor(row,col);
					z_buffer[row][col] = z;
					setbgcolor(r,g,b);
					setcolor(0,0,0);
					cout << " ";
				}
			}
		}
		else {
			if (start.row > end.row) {
				swap(start.col,end.col);
				swap(start.row,end.row);
			}
			double slope = double(horiz_distance)/vert_distance; //Slope of the line
			for (int i = 0; i <= abs(vert_distance); i++) {
				int row = start.row + i;
				int col  = start.col + i*slope;
				double alpha = double(i)/vert_distance;
				if (vert_distance < 0) alpha = 1-abs(alpha);
				uint8_t r = lerp(start.r,end.r,alpha);
				uint8_t g = lerp(start.g,end.g,alpha);
				uint8_t b = lerp(start.b,end.b,alpha);
				double z = lerp(start.z,end.z,alpha);
				double buffered_z = z_buffer.at(row).at(col); //See what depth is already there
				if (z < buffered_z) {
					movecursor(row,col);
					z_buffer[row][col] = z;
					setbgcolor(r,g,b);
					setcolor(0,0,0);
					cout << " ";
				}
			}
		}
		auto [rows,cols] = get_terminal_size(); //Make sure we're returning an int in range
		movecursor(rows-1,0); //Move the cursor to the bottom of the screen
		cout << RESET;
		cout.flush();
	}
};
struct LineSeg2 {
	Point start;
	double m; //How to handle vertical?
	double length; //Should be >= 0
};

struct Triangle {
	//Precondition: Points are held clockwise
	size_t a,b,c; //Indices in the vertex buffer
	//YOU: Make a transformation matrix?? Seems like a good idea
	// And maybe write a rotation() member function that updates the matrix
	// Then when you rasterize, all of the row/col/zed data will go through that transformation matrix
	// To get a final row and col
	//Maybe make a center point for the triangle to rotate around
	void raster() {
		auto [rows,cols] = get_terminal_size(); 
		LineSeg first{a,b};
		LineSeg second{b,c};
		LineSeg third{c,a};
		//For every row for every column on the entire damn screen because I lazy
		first.raster();
		second.raster();
		third.raster();
		setbgcolor(0,0,0);
		setcolor(255,255,255);
		return;
		for (int row = 0; row < rows; row++) {
			for (int col = 0; col < cols; col++) {
				//We will see if it is inside the triangle via crossproduct
				Point p{row,col};
				//Suppose first.start is all red, second.start is all blue, third.start is all green
				double cross1 = first.crossproduct(p); //This is the percentage of green
				double cross2 = second.crossproduct(p); //This is our red percentage
				double cross3 = third.crossproduct(p); //This is the relative amount of blue
				double cross_sum = cross1 + cross2 + cross3;
				if (cross1 < 0 or cross2 < 0 or cross3 < 0)
					;
				else {
					//TODO: Use barycentric coordinates to color the triangle properly
					//Convert to what percentage each cross product is
					cross1 /= cross_sum;
					cross2 /= cross_sum;
					cross3 /= cross_sum;
					uint8_t red =
						cross2*vertex_buffer.at(first.start_index).r +
						cross3*vertex_buffer.at(second.start_index).r +
						cross1*vertex_buffer.at(third.start_index).r;
					uint8_t green =
						cross2*vertex_buffer.at(first.start_index).g +
						cross3*vertex_buffer.at(second.start_index).g +
						cross1*vertex_buffer.at(third.start_index).g;
					uint8_t blue =
						cross2*vertex_buffer.at(first.start_index).b +
						cross3*vertex_buffer.at(second.start_index).b +
						cross1*vertex_buffer.at(third.start_index).b;
					double zed = 
						cross2*vertex_buffer.at(first.start_index).z +
						cross3*vertex_buffer.at(second.start_index).z +
						cross1*vertex_buffer.at(third.start_index).z;

					double buffered_z = z_buffer.at(row).at(col); //See what depth is already there
					if (zed < buffered_z) {
						movecursor(row,col);
						z_buffer[row][col] = zed;
						setbgcolor(red,blue,green);
						cout << ' ';
					}

				}
			}
		}
	}
};

int main() {
	clearscreen();
	vertex_buffer.emplace_back(0,10,10,255,0,0); //row,col,depth,r,g,b
	vertex_buffer.emplace_back(10,10,1,0,255,0);
	vertex_buffer.emplace_back(15,0,1,0,0,255);
	Triangle tri1{0,1,2};
	clear_z_buffer();
	tri1.raster();
	/*
	const Point p1 = read("Type in starting point (x y):\n");
	const Point p2 = read("Type in ending point (x y):\n");
	vertex_buffer.emplace_back(10,10,1,255,0,0);
	vertex_buffer.emplace_back(10,20,1,0,255,0);
	vertex_buffer.emplace_back(20,10,1,0,0,255);
	Triangle tri1{0,1,2};
	vertex_buffer.emplace_back(5,35,2,255,0,0);
	vertex_buffer.emplace_back(0,7,2,0,0,255);
	vertex_buffer.emplace_back(20,20,2,0,255,0);
	Triangle tri2{4,3,5};
	clear_z_buffer();
	tri1.raster();
	tri2.raster();
	LineSeg line1{0,1};
	LineSeg line2{1,2};
	LineSeg line3{2,0};
	line1.raster();
	line2.raster();
	line3.raster();
	*/
	cout << RESET << endl;
}
