// Console3DEngine.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "olcConsoleGameEngine.h"
#include <strstream>
#include <fstream>
#include <algorithm>
using namespace std;

struct vec3d
{
	// Points x, y, and z. w for matrix calculations included.
	float x, y, z;
};

struct triangle
{
	// Groups of 3 points
	vec3d p[3];

	// Console specific info for color
	wchar_t sym;
	short col;
};

struct mesh
{
	// Groups of triangles
	vector<triangle> tris;

	
	
	// Open from OBJ
	bool LoadFromObjectFile(string sFilename)
	{
		ifstream f(sFilename);
		if (!f.is_open()) return false;

		// Locally store the vertices from the file
		vector<vec3d> verts;

		// v for vertices
		// f for face
		while (!f.eof())
		{
			char line[128]; // assume each line is at max 128 characters
			f.getline(line, 128);

			strstream s;
			s << line;

			char junk;

			if (line[0] == 'v') // Vertex
			{
				vec3d v;
				s >> junk >> v.x >> v.y >> v.z;
				verts.push_back(v);
			}

			if (line[0] == 'f') // Triangle face
			{
				int f[3];
				s >> junk >> f[0] >> f[1] >> f[2];
				// add a triangle with vert f 0, 1, and 2
				tris.push_back({ verts[f[0]-1], verts[f[1]-1], verts[f[2]-1] }); //-1 bcs OBJ file data starts at 1 not 0
			};
		}

		return true;
	};
};

struct mat4x4
{
	float m[4][4] = { 0 };
};

class cslEngine3D : public olcConsoleGameEngine
{
public:
	cslEngine3D() {
		m_sAppName = L"Demo";
	}

private:
	mesh meshCube;
	mat4x4 matProj;

	vec3d vCamera;

	float fTheta;

	void MultiplyMatrixVector(vec3d& i, vec3d& o, mat4x4& m) // Use references to directly change input with dereference
	{
		// i for input, o for output
		o.x = i.x * m.m[0][0] + i.y * m.m[1][0] + i.z * m.m[2][0] + m.m[3][0];
		o.y = i.x * m.m[0][1] + i.y * m.m[1][1] + i.z * m.m[2][1] + m.m[3][1];
		o.z = i.x * m.m[0][2] + i.y * m.m[1][2] + i.z * m.m[2][2] + m.m[3][2];
		float w = i.x * m.m[0][3] + i.y * m.m[1][3] + i.z * m.m[2][3] + m.m[3][3];

		if (w != 0.0f)
		{
			o.x /= w;
			o.y /= w;
			o.z /= w;
		}
	};

	// Taken From Command Line Webcam Video
	// Boilerplate
	// Use whatever function gets the color you want
	// Basically shaders handle how we handle rendering meshes (color shadow etc)
	// Currently not used bcs it doesn't work and honestly I don't understand how this works. Not important though bcs it's proprietary.   
	CHAR_INFO GetColour(float lum)
	{
		short bg_col, fg_col;
		wchar_t sym;
		int pixel_bw = (int)(13.0f * lum);
		switch (pixel_bw)
		{
		case 0: bg_col = BG_BLACK; fg_col = FG_BLACK; sym = PIXEL_SOLID; break;

		case 1: bg_col = BG_BLACK; fg_col = FG_DARK_GREY; sym = PIXEL_QUARTER; break;
		case 2: bg_col = BG_BLACK; fg_col = FG_DARK_GREY; sym = PIXEL_HALF; break;
		case 3: bg_col = BG_BLACK; fg_col = FG_DARK_GREY; sym = PIXEL_THREEQUARTERS; break;
		case 4: bg_col = BG_BLACK; fg_col = FG_DARK_GREY; sym = PIXEL_SOLID; break;

		case 5: bg_col = BG_DARK_GREY; fg_col = FG_GREY; sym = PIXEL_QUARTER; break;
		case 6: bg_col = BG_DARK_GREY; fg_col = FG_GREY; sym = PIXEL_HALF; break;
		case 7: bg_col = BG_DARK_GREY; fg_col = FG_GREY; sym = PIXEL_THREEQUARTERS; break;
		case 8: bg_col = BG_DARK_GREY; fg_col = FG_GREY; sym = PIXEL_SOLID; break;

		case 9:  bg_col = BG_GREY; fg_col = FG_WHITE; sym = PIXEL_QUARTER; break;
		case 10: bg_col = BG_GREY; fg_col = FG_WHITE; sym = PIXEL_HALF; break;
		case 11: bg_col = BG_GREY; fg_col = FG_WHITE; sym = PIXEL_THREEQUARTERS; break;
		case 12: bg_col = BG_GREY; fg_col = FG_WHITE; sym = PIXEL_SOLID; break;
		default:
			bg_col = BG_BLACK; fg_col = FG_BLACK; sym = PIXEL_SOLID;
		}

		CHAR_INFO c;
		c.Attributes = bg_col | fg_col;
		c.Char.UnicodeChar = sym;
		return c;
	}



public:
	bool OnUserCreate() override
	{
		meshCube.LoadFromObjectFile("./teapot.obj");

		// Projection Matrix stuff
		float fNear = 0.1f; // Distance from camera point to 2d projection plane
		float fFar = 1000.0f; // Maximum view distance
		float fFOV = 90.0f; // Field of View (degrees)
		float fAspectRatio = (float)ScreenHeight() / (float)ScreenWidth(); 
		float fFOVRad = 1.0f / tanf(fFOV * 0.5f / 180.0f * 3.14159f); // Splits projection plane in half, scale translation of vec by 1/tan(theta). See #1 at 19:00
		// For reference, search projection matrix.
		
		matProj.m[0][0] = fAspectRatio * fFOVRad;
		matProj.m[1][1] = fFOVRad;
		matProj.m[2][2] = fFar / (fFar - fNear);
		matProj.m[3][2] = (-fFar * fNear) / (fFar - fNear);
		matProj.m[2][3] = 1.0f;
		matProj.m[3][3] = 0.0f;


		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		Fill(0, 0, ScreenWidth(), ScreenHeight(), PIXEL_SOLID, FG_BLACK);

		// Set up rotation matrices
		mat4x4 matRotZ, matRotX, matRotY;
		fTheta += 1.0f * fElapsedTime;

		// Search on google: Rotation Matrix
		// Rotation Z
		matRotZ.m[0][0] = cosf(3.14159f);
		matRotZ.m[0][1] = sinf(3.14159f);
		matRotZ.m[1][0] = -sinf(3.14159f);
		matRotZ.m[1][1] = cosf(3.14159f);
		matRotZ.m[2][2] = 1;
		matRotZ.m[3][3] = 1;

		// Rotation X
		matRotX.m[0][0] = 1;
		matRotX.m[1][1] = cosf(fTheta * 0.5f);
		matRotX.m[1][2] = sinf(fTheta * 0.5f);
		matRotX.m[2][1] = -sinf(fTheta * 0.5f);
		matRotX.m[2][2] = cosf(fTheta * 0.5f);
		matRotX.m[3][3] = 1;

		// Rotation Y
		matRotY.m[0][0] = cosf(fTheta * 0.5f);
		matRotY.m[1][1] = 1;
		matRotY.m[0][2] = sinf(fTheta * 0.5f);
		matRotY.m[2][0] = -sinf(fTheta * 0.5f);
		matRotY.m[2][2] = cosf(fTheta * 0.5f);
		matRotY.m[3][3] = 1;

		// Save all triangles in a vector before rasterizing
		vector<triangle> vecTrianglesToRaster;

		// Drawing Triangles
		for (auto tri : meshCube.tris)
		{
			// Processes 1 triangle
			triangle triProjected, triTranslated, triRotatedZ, triRotatedZX;

			// Rotated Y
			MultiplyMatrixVector(tri.p[0], triRotatedZ.p[0], matRotY);
			MultiplyMatrixVector(tri.p[1], triRotatedZ.p[1], matRotY);
			MultiplyMatrixVector(tri.p[2], triRotatedZ.p[2], matRotY);
		

			// RotatedZ then Z
			MultiplyMatrixVector(triRotatedZ.p[0], triRotatedZX.p[0], matRotZ);
			MultiplyMatrixVector(triRotatedZ.p[1], triRotatedZX.p[1], matRotZ);
			MultiplyMatrixVector(triRotatedZ.p[2], triRotatedZX.p[2], matRotZ);

			// Translate triangle away from screen
			triTranslated = triRotatedZX;
			triTranslated.p[0].z = triRotatedZX.p[0].z + 5.0f;
			triTranslated.p[1].z = triRotatedZX.p[1].z + 5.0f;
			triTranslated.p[2].z = triRotatedZX.p[2].z + 5.0f;

			triTranslated.p[0].y = triRotatedZX.p[0].y + 1.3f;
			triTranslated.p[1].y = triRotatedZX.p[1].y + 1.3f;
			triTranslated.p[2].y = triRotatedZX.p[2].y + 1.3f;

			// Get the normals for the current triangle
			// Uses cross multiplication of vectors, line1 X line2
			vec3d normal, line1, line2;

			// Subtracting here so that we can get the local vectors (from 0,0) 
			line1.x = triTranslated.p[1].x - triTranslated.p[0].x;
			line1.y = triTranslated.p[1].y - triTranslated.p[0].y;
			line1.z = triTranslated.p[1].z - triTranslated.p[0].z;

			line2.x = triTranslated.p[2].x - triTranslated.p[0].x;
			line2.y = triTranslated.p[2].y - triTranslated.p[0].y;
			line2.z = triTranslated.p[2].z - triTranslated.p[0].z;

			normal.x = line1.y * line2.z - line1.z * line2.y;
			normal.y = line1.z * line2.x - line1.x * line2.z;
			normal.z = line1.x * line2.y - line1.y * line2.x;

			// Normalize the normal (turn into value -1 to 1)
			// Find length of vector with pythagorean
			float l = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);

			// Divide vector by length
			normal.x /= l;
			normal.y /= l;
			normal.z /= l;

			if (
				normal.x * (triTranslated.p[0].x - vCamera.x) +
				normal.y * (triTranslated.p[0].y - vCamera.y) +
				normal.z * (triTranslated.p[0].z - vCamera.z) < 0.0f) 

			{
				// Beyond this point is 2D projected

				// This is a shader
				// Basic Illumination
				vec3d light_direction = { 0.0f, 0.0f, -1.0f };
				float l = sqrtf(light_direction.x * light_direction.x + light_direction.y * light_direction.y + light_direction.z * light_direction.z);
				light_direction.x /= l; light_direction.y /= l; light_direction.z /= l;

				// How similar are the normals?
				float dp = normal.x * light_direction.x + normal.y * light_direction.y + normal.z * light_direction.z;

				// Color based on how similar are the normals of the light and the mesh
				// Implementation doesn't work for now but concept is there. 
				CHAR_INFO c = GetColour(dp);
				triTranslated.col = c.Attributes;
				triTranslated.sym = c.Char.UnicodeChar;

				// Projecting the vertices
				MultiplyMatrixVector(triTranslated.p[0], triProjected.p[0], matProj);
				MultiplyMatrixVector(triTranslated.p[1], triProjected.p[1], matProj);
				MultiplyMatrixVector(triTranslated.p[2], triProjected.p[2], matProj);

				// Scaling to view
				triProjected.p[0].x += 1.0f; triProjected.p[0].y += 1.0f;
				triProjected.p[1].x += 1.0f; triProjected.p[1].y += 1.0f;
				triProjected.p[2].x += 1.0f; triProjected.p[2].y += 1.0f;

				triProjected.p[0].x *= 0.5f * (float)ScreenWidth();
				triProjected.p[0].y *= 0.5f * (float)ScreenHeight();
				triProjected.p[1].x *= 0.5f * (float)ScreenWidth();
				triProjected.p[1].y *= 0.5f * (float)ScreenHeight();
				triProjected.p[2].x *= 0.5f * (float)ScreenWidth();
				triProjected.p[2].y *= 0.5f * (float)ScreenHeight();

				vecTrianglesToRaster.push_back(triProjected);
			}
		}

		sort(vecTrianglesToRaster.begin(), vecTrianglesToRaster.end(), [](triangle& t1, triangle& t2)
			{
				float z1 = (t1.p[0].z + t1.p[1].z + t1.p[2].z) / 3.0f;
				float z2 = (t2.p[0].z + t2.p[1].z + t2.p[2].z) / 3.0f;
				return z1 > z2;
			});

		for (auto& triProjected : vecTrianglesToRaster)
		{
			FillTriangle(
				triProjected.p[0].x, triProjected.p[0].y,
				triProjected.p[1].x, triProjected.p[1].y,
				triProjected.p[2].x, triProjected.p[2].y,
				PIXEL_SOLID, FG_GREY);

			DrawTriangle(
				triProjected.p[0].x, triProjected.p[0].y,
				triProjected.p[1].x, triProjected.p[1].y,
				triProjected.p[2].x, triProjected.p[2].y,
				PIXEL_SOLID, FG_WHITE);
		}
		
		return true;
	}
};

int main() {
	cslEngine3D demo;
	if (demo.ConstructConsole(256, 240, 4, 4))
	{
		demo.Start();
	}

	return 0;
}