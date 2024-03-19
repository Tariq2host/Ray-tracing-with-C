#define _CRT_SECURE_NO_WARNINGS 1
#include <vector>

#include <algorithm>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <ctime>
#include <fstream>
#include <iostream>
#define M_PI 3.14159265358979323846

#include <random>
#include <chrono> 

#include <string>
#include <stdio.h>
#include <queue>

const int MAX_REBONDS = 5;

static std::default_random_engine engine(10);                
static std::uniform_real_distribution<double> uniform(0, 1); 

static inline double sqr(double x) { return x * x; } //  Retourne x^2


class Vector
{
public:
    explicit Vector(double x = 0, double y = 0, double z = 0)
    {
        coords[0] = x;
        coords[1] = y;
        coords[2] = z;
    }
    double operator[](int i) const { return coords[i]; };
    double &operator[](int i) { return coords[i]; };
    double norm2()
    {
        return sqr(coords[0]) + sqr(coords[1]) + sqr(coords[2]);
    }
    Vector getNormalized()
    {
        double n = sqrt(norm2());
        return Vector(coords[0] / n, coords[1] / n, coords[2] / n);
    }

    Vector &operator+=(const Vector &a)
    {
        coords[0] += a[0];
        coords[1] += a[1];
        coords[2] += a[2];
        return *this;
    }

private:
    double coords[3];
};

Vector operator+(const Vector &a, const Vector &b)
{
    return Vector(a[0] + b[0], a[1] + b[1], a[2] + b[2]);
}

Vector operator-(const Vector &a, const Vector &b)
{
    return Vector(a[0] - b[0], a[1] - b[1], a[2] - b[2]);
}

Vector operator-(const Vector &a)
{
    return Vector(-a[0], -a[1], -a[2]);
}

Vector operator*(double a, const Vector &b)
{
    return Vector(a * b[0], a * b[1], a * b[2]);
}

Vector operator*(const Vector &a, double b)
{
    return Vector(a[0] * b, a[1] * b, a[2] * b);
}

Vector operator*(const Vector &a, const Vector &b)
{
    return Vector(a[0] * b[0], a[1] * b[1], a[2] * b[2]);
}

Vector operator/(const Vector &a, double b)
{
    return Vector(a[0] / b, a[1] / b, a[2] / b);
}

Vector cross(const Vector &a, const Vector &b)
{
    return Vector(a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]);
}

double dot(const Vector &a, const Vector &b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}


Vector random_cos(const Vector &N) {
    // générer des coordonnées sphériques aléatoires
    double u1 = uniform(engine);
    double u2 = uniform(engine);
    double x = cos(2 * M_PI * u1) * sqrt(1 - u2);
    double y = sin(2 * M_PI * u1) * sqrt(1 - u2);
    double z = sqrt(u2);
    //vecteur orthogonal à N
    Vector notN = (fabs(N[0]) > 0.1) ? Vector(0, 1, 0) : Vector(1, 0, 0);
    Vector T1 = cross(N, notN).getNormalized();
    Vector T2 = cross(N, T1);
    return z * N + x * T1 + y * T2;
}


void adjustOrientationCamera(Vector &up, Vector &right, Vector &viewDirection,
                              double inclinaisonVerticale, double rotationHorizontale) {
    // convertion en rad
    double angleVerticalRadian = inclinaisonVerticale * M_PI / 180;
    double angleHorizontalRadian = rotationHorizontale * M_PI / 180;
    // vecteur up pour l'inclinaison vertical
    up = Vector(0, cos(angleVerticalRadian), sin(angleVerticalRadian));
    //vecteur right pour la rotation horizontal
    right = Vector(cos(angleHorizontalRadian), 0, sin(angleHorizontalRadian));
    viewDirection = cross(up, right);
    // viewDirection = viewDirection.getNormalized();
    // right = Vector::cross(viewDirection, up).getNormalized();
    // up = Vector::cross(right, viewDirection).getNormalized();
}

