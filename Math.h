#pragma once

#define PI 3.14159265359f

//Integer Types

typedef char int8;
typedef short int16;
typedef int int32;
typedef long long int64;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;


//Vector Types

template <typename T> class Vector2 {
public:
	T x, y;

	Vector2() : x(0), y(0) {}
	Vector2(T X, T Y) : x(X), y(Y) {}
	Vector2(const Vector2& o) : x(o.x), y(o.y) {}
	Vector2& operator = (const Vector2& o) { x = o.x; y = o.y; return *this; }

	//array access
	T operator [] (size_t i) { return (&x)[i]; }

	//dot product
	T dot(const Vector2& o) const { return x * o.x + y * o.y; }

	//equality
	bool operator == (const Vector2& o) const { return x == o.x && y == o.y; }
	bool operator != (const Vector2& o) const { return x != o.x || y != o.y; }

	//negate
	Vector2 operator - () const { return Vector2(-x, -y); }

	//add, sub
	void operator += (const Vector2& o) { x += o.x; y += o.y; }
	void operator -= (const Vector2& o) { x -= o.x; y -= o.y; }
	Vector2 operator + (const Vector2& o) const { return Vector2(x + o.x, y + o.y); }
	Vector2 operator - (const Vector2& o) const { return Vector2(x - o.x, y - o.y); }

	//mult, div
	void operator *= (T o) { x *= o; y *= o; }
	void operator /= (T o) { x /= o; y /= o; }
	Vector2 operator * (T o) const { return Vector2(x * o, y * o); }
	Vector2 operator / (T o) const { return Vector2(x / o, y / o); }

	//scale component-wise
	void operator *= (const Vector2& o) { x *= o.x; y *= o.y; }
	void operator /= (const Vector2& o) { x /= o.x; y /= o.y; }
	Vector2 operator * (const Vector2& o) const { return Vector2(x * o.x, y * o.y); }
	Vector2 operator / (const Vector2& o) const { return Vector2(x / o.x, y / o.y); }
};

template <typename T> class Vector3 {
public:
	T x, y, z;

	Vector3() : x(0), y(0), z(0) {}
	Vector3(T X, T Y, T Z) : x(X), y(Y), z(Z) {}
	Vector3(const Vector3& o) : x(o.x), y(o.y), z(o.z) {}
	Vector3& operator = (const Vector3& o) { x = o.x; y = o.y; z = o.z; return *this; }

	//array access
	T operator [] (size_t i) { return (&x)[i]; }

	//dot product
	T dot(const Vector3& o) const { return x * o.x + y * o.y + z * o.z; }

	//cross product (right handed coordinates)
	Vector3 cross(const Vector3& o) const { return Vector3(y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x); }

	//equality
	bool operator == (const Vector3& o) const { return x == o.x && y == o.y && z == o.z; }
	bool operator != (const Vector3& o) const { return x != 0.x || y != o.y || z != o.z; }

	//negate
	Vector3 operator - () const { return Vector3(-x, -y, -z); }

	//add, sub
	void operator += (const Vector3& o) { x += o.x; y += o.y; z += o.z; }
	void operator -= (const Vector3& o) { x -= o.x; y -= o.y; z -= o.z; }
	Vector3 operator + (const Vector3& o) const { return Vector3(x + o.x, y + o.y, z + o.z); }
	Vector3 operator - (const Vector3& o) const { return Vector3(x - o.x, y - o.y, z - o.z); }

	//mult, div
	void operator *= (T o) { x *= o; y *= o; z *= o; }
	void operator /= (T o) { x /= o; y /= o; z /= o; }
	Vector3 operator * (T o) const { return Vector3(x * o, y * o, z * o); }
	Vector3 operator / (T o) const { return Vector3(x / o, y / o, z / o); }

	//scale component-wise
	void operator *= (const Vector3& o) { x *= o.x; y *= o.y; z *= o.z; }
	void operator /= (const Vector3& o) { x /= o.x; y /= o.y; z /= o.z; }
	Vector3 operator * (const Vector3& o) const { return Vector3(x * o.x, y * o.y, z * o.z); }
	Vector3 operator / (const Vector3& o) const { return Vector3(x / o.x, y / o.y, z / o.z); }
};

template <typename T> class Vector4 {
public:
	T x, y, z, w;

	Vector4() : x(0), y(0), z(0), w(0) {}
	Vector4(T X, T Y, T Z, T W) : x(X), y(Y), z(Z), w(W) {}
	Vector4(const Vector4& o) : x(o.x), y(o.y), z(o.z), w(o.w) {}
	Vector4& operator = (const Vector4& o) { x = o.x; y = o.y; z = o.z; w = o.w; return *this; }

	//array access
	T& operator [] (size_t i) { return (&x)[i]; }

	//dot product
	T dot(const Vector4& o) const { return x * o.x + y * o.y + z * o.z + w * o.w; }

	//equality
	bool operator == (const Vector4& o) const { return x == o.x && y == o.y && z == o.z && w == o.w; }
	bool operator != (const Vector4& o) const { return x != 0.x || y != o.y || z != o.z || w != o.w; }

	//negate
	Vector4 operator - () const { return Vector4(-x, -y, -z, -w); }

	//add, sub
	void operator += (const Vector4& o) { x += o.x; y += o.y; z += o.z; w += o.w; }
	void operator -= (const Vector4& o) { x -= o.x; y -= o.y; z -= o.z; w -= o.w; }
	Vector4 operator + (const Vector4& o) const { return Vector4(x + o.x, y + o.y, z + o.z, w + o.w); }
	Vector4 operator - (const Vector4& o) const { return Vector4(x - o.x, y - o.y, z - o.z, w - o.w); }

	//mult, div
	void operator *= (T o) { x *= o; y *= o; z *= o; w *= o; }
	void operator /= (T o) { x /= o; y /= o; z /= o; w /= o; }
	Vector4 operator * (T o) const { return Vector4(x * o, y * o, z * o, w * o); }
	Vector4 operator / (T o) const { return Vector4(x / o, y / o, z / o, w / o); }

	//scale component-wise
	void operator *= (const Vector4& o) { x *= o.x; y *= o.y; z *= o.z; w *= o.w; }
	void operator /= (const Vector4& o) { x /= o.x; y /= o.y; z /= o.z; w /= o.w; }
	Vector4 operator * (const Vector4& o) const { return Vector4(x * o.x, y * o.y, z * o.z, w * o.w); }
	Vector4 operator / (const Vector4& o) const { return Vector4(x / o.x, y / o.y, z / o.z, w * o.w); }
};

typedef Vector2<float> Float2;
typedef Vector3<float> Float3;
typedef Vector4<float> Float4;
typedef Vector2<int32> Int2;
typedef Vector3<int32> Int3;
typedef Vector4<int32> Int4;


//Quaternions

class Quaternion {
public:
	float w;
	Float3 u;

	Quaternion() : w(1.0f), u(Float3(0.0f, 0.0f, 0.0f)) {}
	Quaternion(float W, const Float3& U) : w(W), u(U) {}

	Quaternion inv() const { return Quaternion(w, -u); }
	Quaternion operator * (const Quaternion& q) const {
		return Quaternion(
			w * q.w - u.dot(q.u),
			q.u * w + Float3(q.w, -q.u.z, q.u.y) * u.x + Float3(q.u.z, q.w, -q.u.x) * u.y + Float3(-q.u.y, q.u.x, q.w) * u.z
		);
	}
};

//Matrices

#include <cmath>

class Matrix4 {
public:
	Float4 r1;
	Float4 r2;
	Float4 r3;
	Float4 r4;

	
	Matrix4(const Float4& R1, const Float4& R2, const Float4& R3, const Float4& R4) : r1(R1), r2(R2), r3(R3), r4(R4) {}
	Matrix4() : Matrix4(Float4(1.0f, 0.0f, 0.0f, 0.0f),
						Float4(0.0f, 1.0f, 0.0f, 0.0f),
						Float4(0.0f, 0.0f, 1.0f, 0.0f),
						Float4(0.0f, 0.0f, 0.0f, 1.0f)) {}

	Float4& operator [] (size_t i) { return (&r1)[i]; }

	Matrix4 T() const {
		return Matrix4(
			Float4(r1.x, r2.x, r3.x, r4.x),
			Float4(r1.y, r2.y, r3.y, r4.y),
			Float4(r1.z, r2.z, r3.z, r4.z),
			Float4(r1.w, r2.w, r3.w, r4.w));
	}
	Float3 mul(const Float3& x) const {
		Float4 x4 = Float4(x.x, x.y, x.z, 1.0f);
		return Float3(r1.dot(x4), r2.dot(x4), r3.dot(x4));
	}
	Float4 mul(const Float4& x) const {
		return Float4(r1.dot(x), r2.dot(x), r3.dot(x), r4.dot(x));
	}
	Matrix4 mul(const Matrix4& m) const {
		const Matrix4 mT = m.T();
		return Matrix4(
			Float4(r1.dot(mT.r1), r1.dot(mT.r2), r1.dot(mT.r3), r1.dot(mT.r4)),
			Float4(r2.dot(mT.r1), r2.dot(mT.r2), r2.dot(mT.r3), r2.dot(mT.r4)),
			Float4(r3.dot(mT.r1), r3.dot(mT.r2), r3.dot(mT.r3), r3.dot(mT.r4)),
			Float4(r4.dot(mT.r1), r4.dot(mT.r2), r4.dot(mT.r3), r4.dot(mT.r4)));
	}

	static Matrix4 Translation(Float3 t) {
		return Matrix4(
			Float4(1.0f, 0.0f, 0.0f, t.x),
			Float4(0.0f, 1.0f, 0.0f, t.y),
			Float4(0.0f, 0.0f, 1.0f, t.z),
			Float4(0.0f, 0.0f, 0.0f, 1.0f));
	}
	static Matrix4 Scaling(Float3 s) {
		return Matrix4(
			Float4(s.x, 0.0f, 0.0f, 0.0f),
			Float4(0.0f, s.y, 0.0f, 0.0f),
			Float4(0.0f, 0.0f, s.z, 0.0f),
			Float4(0.0f, 0.0f, 0.0f, 1.0f));
	}
	static Matrix4 RotationX(float ang) {
		return Matrix4(
			Float4(1.0f, 0.0f, 0.0f, 0.0f),
			Float4(0.0f, cos(ang), -sin(ang), 0.0f),
			Float4(0.0f, sin(ang), cos(ang), 0.0f),
			Float4(0.0f, 0.0f, 0.0f, 1.0f));
	}
	static Matrix4 RotationY(float ang) {
		return Matrix4(
			Float4(cos(ang), 0.0f, sin(ang), 0.0f),
			Float4(0.0f, 1.0f, 0.0f, 0.0f),
			Float4(-sin(ang), 0.0f, cos(ang), 0.0f),
			Float4(0.0f, 0.0f, 0.0f, 1.0f));
	}
	static Matrix4 RotationZ(float ang) {
		return Matrix4(
			Float4(cos(ang), -sin(ang), 0.0f, 0.0f),
			Float4(sin(ang), cos(ang), 0.0f, 0.0f),
			Float4(0.0f, 0.0f, 1.0f, 0.0f),
			Float4(0.0f, 0.0f, 0.0f, 1.0f));
	}
	static Matrix4 RotationQuat(const Quaternion& q) {
		return Matrix4(
			Float4(1.0f - 2.0f * (q.u.y * q.u.y + q.u.z * q.u.z), 2.0f * (q.u.x * q.u.y - q.u.z * q.w), 2.0f * (q.u.x * q.u.z + q.u.y * q.w), 0.0f),
			Float4(2.0f * (q.u.x * q.u.y + q.u.z * q.w), 1.0f - 2.0f * (q.u.x * q.u.x + q.u.z * q.u.z), 2.0f * (q.u.y * q.u.z - q.u.x * q.w), 0.0f),
			Float4(2.0f * (q.u.x * q.u.z - q.u.y * q.w), 2.0f * (q.u.y * q.u.z + q.u.x * q.w), 1.0f - 2.0f * (q.u.x * q.u.x + q.u.y * q.u.y), 0.0f),
			Float4(0.0f, 0.0f, 0.0f, 1.0f));
	}

	Float4 operator * (const Float4& v) const { return mul(v); }
	Float3 operator * (const Float3& v) const { return mul(v); }
	Matrix4 operator * (const Matrix4& m) const { return mul(m); }
};

class Matrix3 {
public:
	Float3 r1;
	Float3 r2;
	Float3 r3;

	Matrix3(const Matrix4& o) :
		r1(o.r1.x, o.r1.y, o.r1.z),
		r2(o.r2.x, o.r2.y, o.r2.z),
		r3(o.r3.x, o.r3.y, o.r3.z)
	{}
	Matrix3() {}
};
