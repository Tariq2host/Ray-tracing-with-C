// Désactive les avertissements sur les fonctions non sécurisées (spécifique à MSVC)
#define _CRT_SECURE_NO_WARNINGS 1

// Inclut la bibliothèque vector de C++ pour utiliser le type de données vector
#include <vector>

// Définit les macros pour intégrer les implémentations de stb_image_write et stb_image
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define M_PI 3.141592653589


// Définit une fonction inline pour calculer le carré d'un nombre (utilisé dans la norme)
static inline double sqr(double x) { return x * x; } //  Retourne x^2


// Définit une classe Vector pour représenter un vecteur 3D
class Vector {
public: // 
    // Constructeur de la classe Vector avec des valeurs par défaut
    explicit Vector(double x = 0, double y = 0, double z = 0) {
        coord[0] = x;
        coord[1] = y;
        coord[2] = z;
    }

    // Surcharge de l'opérateur [] pour accéder/modifier les coordonnées
    double& operator[](int i) { return coord[i]; }
    double operator[](int i) const { return coord[i]; }

    // Surcharge de l'opérateur += pour ajouter un autre vecteur à ce vecteur
    Vector& operator+=(const Vector& v) {
        coord[0] += v[0];
        coord[1] += v[1];
        coord[2] += v[2];
        return *this;
    }

    // Calcule la norme au carré du vecteur (somme des carrés des coordonnées)
    double norm2() const {
        return sqr(coord[0]) + sqr(coord[1]) + sqr(coord[2]); 
    }

    void normalize() {
        double norm = sqrt(norm2()); //  Calcul de la norme
        coord[0] /= norm; //   Normalisation en utilisant la formule : V/||V||
        coord[1] /= norm;
        coord[2] /= norm; 
    }

    Vector getNormalized() { //  Version qui ne modifie pas le vecteur original
        Vector result(*this); //    Crée une copie du vecteur original
        result.normalize(); //    Normalise la copie et renvoie cette copie
        return result;
    }

    double coord[3]; // Tableau pour stocker les trois coordonnées du vecteur
};

// Définit des opérateurs pour effectuer des opérations vectorielles
Vector operator+(const Vector& a, const Vector& b) {
    return Vector(a[0] + b[0], a[1] + b[1], a[2] + b[2]);
}
Vector operator-(const Vector& a, const Vector& b) {
    return Vector(a[0] - b[0], a[1] - b[1], a[2] - b[2]);
}
Vector operator*(const Vector& a, double b) {
    return Vector(a[0]*b, a[1]*b, a[2]*b);
}
Vector operator*(double a, const Vector& b) {
    return Vector(a*b[0], a*b[1], a*b[2]);
}

// Fonction pour calculer le produit scalaire de deux vecteurs
double dot(const Vector& a, const Vector& b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}



// class rayon qui définit O l'origine et u sa direction
class Rayon {
    public:
    Rayon(const Vector& origin, const Vector& direction): O(origin), u(direction) {} ;
    Vector O,u;
};

// on définit une classe lumiére qui representra notre source de lumiére dans l'image
class lumiere {
    public:
    lumiere(const Vector& position, const double intensite): P(position), I(intensite) {} ;
    Vector P;
    double I;
};


// class sphere de centre C et de rayon R avec la fonction intersect qui vérifie si la droite intersect la sphére
class sphere {
    public :
    sphere(const Vector& origin, double rayon, Vector couleur) : C(origin), R(rayon), albedo(couleur) {};
    Vector C;
    double R;
    Vector albedo; // facteur qui represente à chaque elle reflete une couleur 
    bool intersect(const Rayon& r, Vector& P, Vector &N) { 
        // resout a*t*2 + b*t +c =c0
        // on cherche la position (le plus proche) de l'intersection avec la sphere (c'est le P)
        // et la normal par rapport à la sphere sur cette position p (c'est N)
        double a = 1; 
        double b = 2 * dot(r.u, r.O-C);  
        double c = (r.O-C).norm2() - R*R;
        double delta = b * b - 4 * a * c; 
        if (delta < 0) return false; // 
        double sqrtdelta = sqrt(delta);
        double t1 = (-b - sqrtdelta) / (2 * a);
        double t2 = (-b + sqrtdelta) / (2 * a);
        if (t2 < 0) return false;
        double t;
        if (t1>0) 
            t = t1;
        else
            t = t2;
        P = r.O + t*r.u;
        N = (P-C).getNormalized();
        return true; 
    }
};


// La fonction principale où le programme commence
int main() {
    // Définit la largeur et la hauteur de l'image
    int W = 512;                  // Largeur de l'image
    int H = 512;                  // Hauteur de l'image

    double fov = 60 * M_PI / 100; // Angle de champ de la caméra (en radians)
    sphere s(Vector(0, 0, -55), 20, Vector(1,0,0)); // Crée une sphère avec un centre et un rayon

    lumiere l(Vector(10,6,-20),100000);

    // Crée un vecteur pour stocker l'image (3 canaux par pixel)
    std::vector<unsigned char> image(W * H * 3, 0);

    // Boucle sur chaque pixel de l'image (en parallèle si OpenMP est activé)
#pragma omp parallel for
    for (int i = 0; i < H; i++) {
        for (int j = 0; j < W; j++) {

            // Crée un vecteur pour le pixel actuel
            Vector u(j - W / 2 + 0.5, -i + H / 2 + 0.5, -W / (2 * tan(fov / 2)));

            u.normalize();
            // Crée un rayon depuis le centre de la caméra vers ce pixel
            Rayon r(Vector(0, 0, 0), u);
            Vector P, N;
            bool has_inter = s.intersect(r, P, N);

            Vector intensite_pix(0,0,0);
            if (has_inter){
                intensite_pix = s.albedo * (l.I * std::max(0.,dot((l.P - P).getNormalized(), N)) / (l.P - P).norm2());
            }
            // Attribution de la couleur au pixel en fonction de l'intersection avec la sphère
            image[(i * W + j) * 3 + 0] = std::min(255., std::max(0.,intensite_pix[0]));  // Composante RED
            image[(i * W + j) * 3 + 1] = std::min(255., std::max(0.,intensite_pix[1]));   // Composante GREEN
            image[(i * W + j) * 3 + 2] = std::min(255., std::max(0.,intensite_pix[2]));   // Composante BLUE
        }
    }
    // Enregistre l'image résultante au format PNG
    stbi_write_png("sphere_eclaire_albedo.png", W, H, 3, &image[0], 0);

    // Termine le programme
    return 0;
}
