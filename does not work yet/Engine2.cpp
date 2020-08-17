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
	float x = 0;
	float y = 0;
	float z = 0;
	float w = 1;
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

			if (line[0] == 'v') // Vertex/Vector
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
				tris.push_back({ verts[f[0] - 1], verts[f[1] - 1], verts[f[2] - 1] }); //-1 bcs OBJ file data starts at 1 not 0
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
	/*
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
	};*/

	vec3d Matrix_MultiplyVector(mat4x4& m, vec3d& i)
	{
		vec3d v;
		v.x = i.x * m.m[0][0] + i.y * m.m[1][0] + i.z * m.m[2][0] + i.w * m.m[3][0];
		v.y = i.x * m.m[0][1] + i.y * m.m[1][1] + i.z * m.m[2][1] + i.w * m.m[3][1];
		v.z = i.x * m.m[0][2] + i.y * m.m[1][2] + i.z * m.m[2][2] + i.w * m.m[3][2];
		v.w = i.x * m.m[0][3] + i.y * m.m[1][3] + i.z * m.m[2][3] + i.w * m.m[3][3];

		return v;
	}

	mat4x4 Matrix_MakeIdentity()
	{
		mat4x4 matrix;
		matrix.m[0][0] = 1.0f;
		matrix.m[1][1] = 1.0f;
		matrix.m[2][2] = 1.0f;
		matrix.m[3][3] = 1.0f;
		return matrix;
	}

	mat4x4 Matrix_MakeRotationY(float fAngleRad)
	{
		// Rotation Y. Search up rotation matrix
		mat4x4 matRotY;
		matRotY.m[0][0] = cosf(fTheta * fAngleRad);
		matRotY.m[1][1] = 1;
		matRotY.m[0][2] = sinf(fTheta * fAngleRad);
		matRotY.m[2][0] = -sinf(fTheta * fAngleRad);
		matRotY.m[2][2] = cosf(fTheta * fAngleRad);
		matRotY.m[3][3] = 1;
		return matRotY;
	}

	mat4x4 Matrix_MakeRotationX(float rad)
	{
		// Rotation X
		mat4x4 matRotX;
		matRotX.m[0][0] = 1;
		matRotX.m[1][1] = cosf(fTheta * rad);
		matRotX.m[1][2] = sinf(fTheta * rad);
		matRotX.m[2][1] = -sinf(fTheta * rad);
		matRotX.m[2][2] = cosf(fTheta * rad);
		matRotX.m[3][3] = 1;
		return matRotX;
	}

	mat4x4 Matrix_MakeRotationZ(float rad)
	{
		// Rotation Z
		mat4x4 matRotZ;
		matRotZ.m[0][0] = cosf(rad);
		matRotZ.m[0][1] = sinf(rad);
		matRotZ.m[1][0] = -sinf(rad);
		matRotZ.m[1][1] = cosf(rad);
		matRotZ.m[2][2] = 1;
		matRotZ.m[3][3] = 1;
		return matRotZ;
	}

	mat4x4 Matrix_MakeTranslation(float x, float y, float z)
	{
		mat4x4 matrix;
		matrix.m[0][0] = 1.0f;
		matrix.m[1][1] = 1.0f;
		matrix.m[2][2] = 1.0f;
		matrix.m[3][3] = 1.0f;
		matrix.m[3][0] = x;
		matrix.m[3][1] = y;
		matrix.m[3][2] = z;

		return matrix;
	}

	mat4x4 Matrix_MakeProjection(float fFOV, float fAspectRatio, float fNear, float fFar)
	{
		/*
		float fNear = 0.1f; // Distance from camera point to 2d projection plane
		float fFar = 1000.0f; // Maximum view distance
		float fFOV = 90.0f; // Field of View (degrees)
		float fAspectRatio = (float)ScreenHeight() / (float)ScreenWidth();
		*/

		float fFOVRad = 1.0f / tanf(fFOV * 0.5f / 180.0f * 3.14159f); // Splits projection plane in half, scale translation of vec by 1/tan(theta). See #1 at 19:00
		mat4x4 matrix;
		matrix.m[0][0] = fAspectRatio * fFOV;
		matrix.m[1][1] = fFOV;
		matrix.m[2][2] = fFar / (fFar - fNear);
		matrix.m[3][2] = (-fFar * fNear) / (fFar - fNear);
		matrix.m[2][3] = 1.0f;
		matrix.m[3][3] = 0.0f;

		return matrix;
	}

	mat4x4 Matrix_MultiplyMatrix(mat4x4& m1, mat4x4& m2)
	{
		mat4x4 matrix;
		for (int c = 0; c < 4; c++)
		{
			for (int r = 0; r < 4; r++)
			{
				matrix.m[r][c] = m1.m[r][0] * m2.m[0][c] + m1.m[r][1] * m2.m[1][c] + m1.m[r][2] * m2.m[2][c] + m1.m[r][3] * m2.m[3][c];
			}
		}

		return matrix;
	}

	// Adds Vectors
	vec3d Vector_Add(vec3d& v1, vec3d& v2)
	{
		return { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
	}

	vec3d Vector_Sub(vec3d& v1, vec3d& v2)
	{
		return { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
	}

	vec3d Vector_Mul(vec3d& v1, float k)
	{
		return { v1.x * k, v1.y * k, v1.z * k };
	}

	vec3d Vector_Div(vec3d& v1, float k)
	{
		return { v1.x / k, v1.y / k, v1.z / k };
	}

	float Vector_DotProduct(vec3d& v1, vec3d& v2)
	{
		return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
	}

	float Vector_Length(vec3d& v)
	{
		return sqrtf(Vector_DotProduct(v, v));
	}

	vec3d Vector_Normalise(vec3d& v)
	{
		float l = Vector_Length(v);
		return{ v.x / l, v.y / l, v.z / l };
	}

	vec3d Vector_CrossProduct(vec3d& v1, vec3d& v2)
	{
		vec3d v;
		v.x = v1.y * v2.z - v1.z * v2.y;
		v.y = v1.z * v2.x - v1.x * v2.z;
		v.z = v1.x * v2.y - v1.y * v2.x;

		return v;
	}

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

		mat4x4 matProj = Matrix_MakeProjection(fFOV, fAspectRatio, fNear, fFar);

		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		Fill(0, 0, ScreenWidth(), ScreenHeight(), PIXEL_SOLID, FG_BLACK);

		// Set up rotation matrices
		mat4x4 matRotZ, matRotX, matRotY;
		fTheta += 1.0f * fElapsedTime;

		matRotZ = Matrix_MakeRotationZ(3.14f);
		matRotY = Matrix_MakeRotationY(fTheta);

		mat4x4 matTrans;
		matTrans = Matrix_MakeTranslation(0.0f, 0.0f, 16.0f);

		mat4x4 matWorld;
		matWorld = Matrix_MakeIdentity();
		// Rotate then Translate
		matWorld = Matrix_MultiplyMatrix(matRotZ, matRotY);
		matWorld = Matrix_MultiplyMatrix(matWorld, matTrans);

		// Save all triangles in a vector before rasterizing
		vector<triangle> vecTrianglesToRaster;

		// Drawing Triangles
		for (auto tri : meshCube.tris)
		{
			// Processes 1 triangle
			triangle triProjected, triTransformed;

			// Translate and rotate each point of the mesh
			triTransformed.p[0] = Matrix_MultiplyVector(matWorld, tri.p[0]);
			triTransformed.p[1] = Matrix_MultiplyVector(matWorld, tri.p[1]);
			triTransformed.p[2] = Matrix_MultiplyVector(matWorld, tri.p[2]);

			// Get the normals for the current triangle
			// Uses cross multiplication of vectors, line1 X line2
			vec3d normal, line1, line2;

			line1 = Vector_Sub(triTransformed.p[1], triTransformed.p[0]);
			line2 = Vector_Sub(triTransformed.p[2], triTransformed.p[0]);

			normal = Vector_CrossProduct(line1, line2);
			normal = Vector_Normalise(normal);

			// Normalize the normal (turn into value -1 to 1)
			// Find length of vector with pythagorean
			float l = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);

			// Divide vector by length
			normal.x /= l;
			normal.y /= l;
			normal.z /= l;

			vec3d vCameraRay = Vector_Sub(triTransformed.p[0], vCamera);

			if (Vector_DotProduct(normal, vCameraRay) < 0.0f)
			{

				/*
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
				triTransformed.col = c.Attributes;
				triTransformed.sym = c.Char.UnicodeChar;
				*/

				// Beyond this point is 2D projected

				// Projecting the vectors
				triProjected.p[0] = Matrix_MultiplyVector(matProj, triTransformed.p[0]);
				triProjected.p[1] = Matrix_MultiplyVector(matProj, triTransformed.p[1]);
				triProjected.p[2] = Matrix_MultiplyVector(matProj, triTransformed.p[2]);

				triProjected.p[0] = Vector_Div(triProjected.p[0], triProjected.p[0].w);
				triProjected.p[1] = Vector_Div(triProjected.p[1], triProjected.p[1].w);
				triProjected.p[2] = Vector_Div(triProjected.p[2], triProjected.p[2].w);

				// Scaling to view
				vec3d vOffsetView{ 1, 1, 0 };
				triProjected.p[0] = Vector_Add(triProjected.p[0], vOffsetView);
				triProjected.p[1] = Vector_Add(triProjected.p[1], vOffsetView);
				triProjected.p[2] = Vector_Add(triProjected.p[2], vOffsetView);

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
