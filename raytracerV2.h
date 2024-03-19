#include "vector.h"


class Rayon
{
public:
    Rayon(const Vector &origin, const Vector &direction) : origin(origin), direction(direction){}
    Vector origin; 
    Vector direction;
};


class Object
{
public:
    Object(){};
    virtual bool intersect(const Rayon &r, Vector &P, Vector &N, double &t, Vector &color) = 0;
    Vector albedo;
    bool Miroir;
    bool transparent;
    bool mapSphere_tri;
    bool map_sphere_sqr;
    // bool istexture;
};


class BoundingBox {
public:
    // si le rayon intersecte la boîte englbante
    bool intersect(const Rayon &ray) const {
        double tMin = std::numeric_limits<double>::lowest();
        double tMax = std::numeric_limits<double>::max();
        for (int i = 0; i < 3; ++i) {
            double t1 = (bbmin[i] - ray.origin[i]) / ray.direction[i];
            double t2 = (bbmax[i] - ray.origin[i]) / ray.direction[i];
            // tMin et tMax pour l'intersection avec les plans
            tMin = std::max(tMin, std::min(t1, t2));
            tMax = std::min(tMax, std::max(t1, t2));
        }
        return tMax >= tMin && tMax >= 0;
    }
    Vector bbmin, bbmax;
};


class IntersectionTriangleResult {
public:
    double alpha, beta, gamma, localt;
    bool hit;
    IntersectionTriangleResult(double alpha, double beta, double gamma, double localt, bool hit)
        : alpha(alpha), beta(beta), gamma(gamma), localt(localt), hit(hit) {}
};


IntersectionTriangleResult intersectTriangle(const Rayon &ray, const Vector &A, const Vector &B, const Vector &C) {
    Vector AB = B - A;
    Vector AC = C - A;
    Vector N = cross(AB, AC);
    Vector AO = ray.origin - A;
    Vector AOxdirection = cross(AO, ray.direction);
    double denominateur = 1. / dot(ray.direction, N);
    double beta = -dot(AC, AOxdirection) * denominateur;
    double gamma = dot(AB, AOxdirection) * denominateur;
    double alpha = 1 - beta - gamma;
    double localt = -dot(AO, N) * denominateur;
    bool hit = beta >= 0 && gamma >= 0 && beta <= 1 && gamma <= 1 && alpha >= 0 && localt > 0;
    return IntersectionTriangleResult(alpha, beta, gamma, localt, hit);
}


class BVHNode {
public:
    BVHNode* leftChild = nullptr; // pointeur vers le fils gauche
    BVHNode* rightChild = nullptr; // ^pointeur vers le fils droit
    BoundingBox bounds; // boîte englobante du node
    int start = 0; // Index de début pour les éléments contenus dans le node
    int end = 0; // Index de fin pour les éléments contenus dans le node
    BVHNode() = default;
};


class TriangleIndices
{
public:
    TriangleIndices(int vtxi = -1, int vtxj = -1, int vtxk = -1, int ni = -1, int nj = -1, int nk = -1, int uvi = -1, int uvj = -1, int uvk = -1, int group = -1, bool added = false) : vtxi(vtxi), vtxj(vtxj), vtxk(vtxk), uvi(uvi), uvj(uvj), uvk(uvk), ni(ni), nj(nj), nk(nk), group(group){};
    int vtxi, vtxj, vtxk; // indices within the vertex coordinates array
    int uvi, uvj, uvk;    // indices within the uv coordinates array
    int ni, nj, nk;       // indices within the normals array
    int group;            // face group
};

class TriangleMesh : public Object
{
public:
    ~TriangleMesh() {}
    TriangleMesh(const Vector &albedo, bool miroir = false, bool transp = false)
    {
        this->albedo = albedo;
        Miroir = miroir;
        transparent = transp;
        BVH = new BVHNode;
    };

    void readOBJ(const char *obj)
    {

        char matfile[255];
        char grp[255];

        FILE *f;
        f = fopen(obj, "r");
        int curGroup = -1;
        while (!feof(f))
        {
            char line[255];
            if (!fgets(line, 255, f))
                break;

            std::string linetrim(line);
            linetrim.erase(linetrim.find_last_not_of(" \r\t") + 1);
            strcpy(line, linetrim.c_str());

            if (line[0] == 'u' && line[1] == 's')
            {
                sscanf(line, "usemtl %[^\n]\n", grp);
                curGroup++;
            }

            if (line[0] == 'v' && line[1] == ' ')
            {
                Vector vec;

                Vector col;
                if (sscanf(line, "v %lf %lf %lf %lf %lf %lf\n", &vec[0], &vec[1], &vec[2], &col[0], &col[1], &col[2]) == 6)
                {
                    col[0] = std::min(1., std::max(0., col[0]));
                    col[1] = std::min(1., std::max(0., col[1]));
                    col[2] = std::min(1., std::max(0., col[2]));

                    vertices.push_back(vec);
                    vertexcolors.push_back(col);
                }
                else
                {
                    sscanf(line, "v %lf %lf %lf\n", &vec[0], &vec[1], &vec[2]);
                    vertices.push_back(vec);
                }
            }
            if (line[0] == 'v' && line[1] == 'n')
            {
                Vector vec;
                sscanf(line, "vn %lf %lf %lf\n", &vec[0], &vec[1], &vec[2]);
                normals.push_back(vec);
            }
            if (line[0] == 'v' && line[1] == 't')
            {
                Vector vec;
                sscanf(line, "vt %lf %lf\n", &vec[0], &vec[1]);
                uvs.push_back(vec);
            }
            if (line[0] == 'f')
            {
                TriangleIndices t;
                int i0, i1, i2, i3;
                int j0, j1, j2, j3;
                int k0, k1, k2, k3;
                int nn;
                t.group = curGroup;

                char *consumedline = line + 1;
                int offset;

                nn = sscanf(consumedline, "%u/%u/%u %u/%u/%u %u/%u/%u%n", &i0, &j0, &k0, &i1, &j1, &k1, &i2, &j2, &k2, &offset);
                if (nn == 9)
                {
                    if (i0 < 0)
                        t.vtxi = vertices.size() + i0;
                    else
                        t.vtxi = i0 - 1;
                    if (i1 < 0)
                        t.vtxj = vertices.size() + i1;
                    else
                        t.vtxj = i1 - 1;
                    if (i2 < 0)
                        t.vtxk = vertices.size() + i2;
                    else
                        t.vtxk = i2 - 1;
                    if (j0 < 0)
                        t.uvi = uvs.size() + j0;
                    else
                        t.uvi = j0 - 1;
                    if (j1 < 0)
                        t.uvj = uvs.size() + j1;
                    else
                        t.uvj = j1 - 1;
                    if (j2 < 0)
                        t.uvk = uvs.size() + j2;
                    else
                        t.uvk = j2 - 1;
                    if (k0 < 0)
                        t.ni = normals.size() + k0;
                    else
                        t.ni = k0 - 1;
                    if (k1 < 0)
                        t.nj = normals.size() + k1;
                    else
                        t.nj = k1 - 1;
                    if (k2 < 0)
                        t.nk = normals.size() + k2;
                    else
                        t.nk = k2 - 1;
                    indices.push_back(t);
                }
                else
                {
                    nn = sscanf(consumedline, "%u/%u %u/%u %u/%u%n", &i0, &j0, &i1, &j1, &i2, &j2, &offset);
                    if (nn == 6)
                    {
                        if (i0 < 0)
                            t.vtxi = vertices.size() + i0;
                        else
                            t.vtxi = i0 - 1;
                        if (i1 < 0)
                            t.vtxj = vertices.size() + i1;
                        else
                            t.vtxj = i1 - 1;
                        if (i2 < 0)
                            t.vtxk = vertices.size() + i2;
                        else
                            t.vtxk = i2 - 1;
                        if (j0 < 0)
                            t.uvi = uvs.size() + j0;
                        else
                            t.uvi = j0 - 1;
                        if (j1 < 0)
                            t.uvj = uvs.size() + j1;
                        else
                            t.uvj = j1 - 1;
                        if (j2 < 0)
                            t.uvk = uvs.size() + j2;
                        else
                            t.uvk = j2 - 1;
                        indices.push_back(t);
                    }
                    else
                    {
                        nn = sscanf(consumedline, "%u %u %u%n", &i0, &i1, &i2, &offset);
                        if (nn == 3)
                        {
                            if (i0 < 0)
                                t.vtxi = vertices.size() + i0;
                            else
                                t.vtxi = i0 - 1;
                            if (i1 < 0)
                                t.vtxj = vertices.size() + i1;
                            else
                                t.vtxj = i1 - 1;
                            if (i2 < 0)
                                t.vtxk = vertices.size() + i2;
                            else
                                t.vtxk = i2 - 1;
                            indices.push_back(t);
                        }
                        else
                        {
                            nn = sscanf(consumedline, "%u//%u %u//%u %u//%u%n", &i0, &k0, &i1, &k1, &i2, &k2, &offset);
                            if (i0 < 0)
                                t.vtxi = vertices.size() + i0;
                            else
                                t.vtxi = i0 - 1;
                            if (i1 < 0)
                                t.vtxj = vertices.size() + i1;
                            else
                                t.vtxj = i1 - 1;
                            if (i2 < 0)
                                t.vtxk = vertices.size() + i2;
                            else
                                t.vtxk = i2 - 1;
                            if (k0 < 0)
                                t.ni = normals.size() + k0;
                            else
                                t.ni = k0 - 1;
                            if (k1 < 0)
                                t.nj = normals.size() + k1;
                            else
                                t.nj = k1 - 1;
                            if (k2 < 0)
                                t.nk = normals.size() + k2;
                            else
                                t.nk = k2 - 1;
                            indices.push_back(t);
                        }
                    }
                }

                consumedline = consumedline + offset;

                while (true)
                {
                    if (consumedline[0] == '\n')
                        break;
                    if (consumedline[0] == '\0')
                        break;
                    nn = sscanf(consumedline, "%u/%u/%u%n", &i3, &j3, &k3, &offset);
                    TriangleIndices t2;
                    t2.group = curGroup;
                    if (nn == 3)
                    {
                        if (i0 < 0)
                            t2.vtxi = vertices.size() + i0;
                        else
                            t2.vtxi = i0 - 1;
                        if (i2 < 0)
                            t2.vtxj = vertices.size() + i2;
                        else
                            t2.vtxj = i2 - 1;
                        if (i3 < 0)
                            t2.vtxk = vertices.size() + i3;
                        else
                            t2.vtxk = i3 - 1;
                        if (j0 < 0)
                            t2.uvi = uvs.size() + j0;
                        else
                            t2.uvi = j0 - 1;
                        if (j2 < 0)
                            t2.uvj = uvs.size() + j2;
                        else
                            t2.uvj = j2 - 1;
                        if (j3 < 0)
                            t2.uvk = uvs.size() + j3;
                        else
                            t2.uvk = j3 - 1;
                        if (k0 < 0)
                            t2.ni = normals.size() + k0;
                        else
                            t2.ni = k0 - 1;
                        if (k2 < 0)
                            t2.nj = normals.size() + k2;
                        else
                            t2.nj = k2 - 1;
                        if (k3 < 0)
                            t2.nk = normals.size() + k3;
                        else
                            t2.nk = k3 - 1;
                        indices.push_back(t2);
                        consumedline = consumedline + offset;
                        i2 = i3;
                        j2 = j3;
                        k2 = k3;
                    }
                    else
                    {
                        nn = sscanf(consumedline, "%u/%u%n", &i3, &j3, &offset);
                        if (nn == 2)
                        {
                            if (i0 < 0)
                                t2.vtxi = vertices.size() + i0;
                            else
                                t2.vtxi = i0 - 1;
                            if (i2 < 0)
                                t2.vtxj = vertices.size() + i2;
                            else
                                t2.vtxj = i2 - 1;
                            if (i3 < 0)
                                t2.vtxk = vertices.size() + i3;
                            else
                                t2.vtxk = i3 - 1;
                            if (j0 < 0)
                                t2.uvi = uvs.size() + j0;
                            else
                                t2.uvi = j0 - 1;
                            if (j2 < 0)
                                t2.uvj = uvs.size() + j2;
                            else
                                t2.uvj = j2 - 1;
                            if (j3 < 0)
                                t2.uvk = uvs.size() + j3;
                            else
                                t2.uvk = j3 - 1;
                            consumedline = consumedline + offset;
                            i2 = i3;
                            j2 = j3;
                            indices.push_back(t2);
                        }
                        else
                        {
                            nn = sscanf(consumedline, "%u//%u%n", &i3, &k3, &offset);
                            if (nn == 2)
                            {
                                if (i0 < 0)
                                    t2.vtxi = vertices.size() + i0;
                                else
                                    t2.vtxi = i0 - 1;
                                if (i2 < 0)
                                    t2.vtxj = vertices.size() + i2;
                                else
                                    t2.vtxj = i2 - 1;
                                if (i3 < 0)
                                    t2.vtxk = vertices.size() + i3;
                                else
                                    t2.vtxk = i3 - 1;
                                if (k0 < 0)
                                    t2.ni = normals.size() + k0;
                                else
                                    t2.ni = k0 - 1;
                                if (k2 < 0)
                                    t2.nj = normals.size() + k2;
                                else
                                    t2.nj = k2 - 1;
                                if (k3 < 0)
                                    t2.nk = normals.size() + k3;
                                else
                                    t2.nk = k3 - 1;
                                consumedline = consumedline + offset;
                                i2 = i3;
                                k2 = k3;
                                indices.push_back(t2);
                            }
                            else
                            {
                                nn = sscanf(consumedline, "%u%n", &i3, &offset);
                                if (nn == 1)
                                {
                                    if (i0 < 0)
                                        t2.vtxi = vertices.size() + i0;
                                    else
                                        t2.vtxi = i0 - 1;
                                    if (i2 < 0)
                                        t2.vtxj = vertices.size() + i2;
                                    else
                                        t2.vtxj = i2 - 1;
                                    if (i3 < 0)
                                        t2.vtxk = vertices.size() + i3;
                                    else
                                        t2.vtxk = i3 - 1;
                                    consumedline = consumedline + offset;
                                    i2 = i3;
                                    indices.push_back(t2);
                                }
                                else
                                {
                                    consumedline = consumedline + 1;
                                }
                            }
                        }
                    }
                }
            }
        }
        fclose(f);
    }

    BoundingBox buildBB(int start, int end) 
    {
        BoundingBox bb;
        bb.bbmin = Vector(1E9, 1E9, 1E9);
        bb.bbmax = Vector(-1E9, -1E9, -1E9);

        for (int i = start; i < end; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                // bb.bbmin[j] = std::min(bb.bbmin[j], vertices[i][j]);
                // bb.bbmax[j] = std::max(bb.bbmax[j], vertices[i][j]);
                bb.bbmin[j] = std::min(bb.bbmin[j], vertices[indices[i].vtxi][j]);
                bb.bbmax[j] = std::max(bb.bbmax[j], vertices[indices[i].vtxi][j]);
                bb.bbmin[j] = std::min(bb.bbmin[j], vertices[indices[i].vtxj][j]);
                bb.bbmax[j] = std::max(bb.bbmax[j], vertices[indices[i].vtxj][j]);
                bb.bbmin[j] = std::min(bb.bbmin[j], vertices[indices[i].vtxk][j]);
                bb.bbmax[j] = std::max(bb.bbmax[j], vertices[indices[i].vtxk][j]);
            }
        }
        return bb;
    }


    void buildBVH(BVHNode *node, int start, int end) {
        node->start = start;
        node->end = end;
        node->bounds = buildBB(node->start, node->end);
        Vector diag = node->bounds.bbmax - node->bounds.bbmin; 
        // dimension la plus longue de la boîte englobnte pour diviser
        int splitDim = maxDimension(diag);
        // calculer le point de division (milieu) dans la dimension choisie
        double splitPos = 0.5 * (node->bounds.bbmin[splitDim] + node->bounds.bbmax[splitDim]);
        // partitionner les éléments autour du point de division
        int pivotIndex = partitionElements(start, end, splitDim, splitPos);
        // initialiser les enfants à nullptr
        node->leftChild = nullptr;
        node->rightChild = nullptr;
        //condition d'arret: si le nombre d'éléments est faible ou aucun élément n'est déplacé
        if (pivotIndex == start || pivotIndex == end || end - start < 5) {
            return;
        }
        // créer des nœuds enfants et construire récursivement leur BVH
        node->leftChild = new BVHNode;
        buildBVH(node->leftChild, start, pivotIndex);
        node->rightChild = new BVHNode;
        buildBVH(node->rightChild, pivotIndex, end);
    }

    int maxDimension(const Vector &v) {
        return (v[0] > v[1]) ? ((v[0] > v[2]) ? 0 : 2) : ((v[1] > v[2]) ? 1 : 2);
    }

    int partitionElements(int start, int end, int dim, double splitPos) {
        int pivotIndex = start;
        for (int i = start; i < end; ++i) {
            double midPoint = (vertices[indices[i].vtxi][dim] + vertices[indices[i].vtxj][dim] + vertices[indices[i].vtxk][dim]) / 3.0;
            if (midPoint < splitPos) {
                std::swap(indices[i], indices[pivotIndex]);
                pivotIndex++;
            }
        }
        return pivotIndex;
    }

    bool intersect(const Rayon &r, Vector &P, Vector &N, double &t, Vector &color)
    {
        if (!BVH->bounds.intersect(r))
            return false;
        t = std::numeric_limits<double>::max();
        bool has_inter = false;
        std::queue<BVHNode *> nodesToVisit;
        nodesToVisit.push(BVH);
        double contrastFactor = 1;
        while (!nodesToVisit.empty())
        {
            BVHNode *currentNode = nodesToVisit.front();
            nodesToVisit.pop();
            if (currentNode->leftChild || currentNode->rightChild) {
                if (currentNode->leftChild && currentNode->leftChild->bounds.intersect(r)) {
                    nodesToVisit.push(currentNode->leftChild);
                }
                if (currentNode->rightChild && currentNode->rightChild->bounds.intersect(r)) {
                    nodesToVisit.push(currentNode->rightChild);
                }
            } 
            else
            {
                for (int i = currentNode->start; i < currentNode->end; i++)
                {
                    // calcul d'intersection de chaqie triangle de sommet A,B,C
                    const Vector &A = vertices[indices[i].vtxi];
                    const Vector &B = vertices[indices[i].vtxj];
                    const Vector &C = vertices[indices[i].vtxk];
                    IntersectionTriangleResult result = intersectTriangle(r, A, B, C);
                    if (result.hit && result.localt < t)
                        {
                            has_inter = true;
                            t = result.localt;

                            // lissage de phong pour les normales
                            N = result.alpha * normals[indices[i].ni] + result.beta * normals[indices[i].nj] + result.gamma * normals[indices[i].nk];
                            N = N.getNormalized();
                            P = r.origin + r.direction;

                            // gestion des textures
                            if (!textures.empty() && !uvs.empty() && !Htex.empty() && !Wtex.empty()) {
                                int H = Htex[indices[i].group];
                                int W = Wtex[indices[i].group];
                                Vector UV = result.alpha * uvs[indices[i].uvi] + result.beta * uvs[indices[i].uvj] + result.gamma * uvs[indices[i].uvk];
                                UV = UV * Vector(W, H, 0);
                                int uvx = static_cast<int>(UV[0] + 0.5) % W;
                                int uvy = static_cast<int>(UV[1] + 0.5) % H;
                                //  negative indices
                                // uvx = (uvx < 0) ? uvx + H : uvx;
                                // uvy = (uvy < 0) ? uvy + W : uvy;
                                uvx = (uvx < 0) ? uvx + W : uvx;
                                uvy = (uvy < 0) ? uvy + H : uvy;
                                uvy = H - uvy - 1;
                                int textureIndex = (uvy * W + uvx) * 3;
                                for (int channel = 0; channel < 3; channel++) {
                                    double colorValue = std::pow(textures[indices[i].group][textureIndex + channel] / 255.0, 2.2);
                                    //ajustement de contraste
                                    colorValue = contrastFactor * (colorValue - 0.5) + 0.5;
                                    // s'assure dans l'intervalle [0, 1]
                                    colorValue = std::min(1.0, std::max(0.0, colorValue));
                                    color[channel] = colorValue;
                                }
                                // color = Vector(std::pow(textures[indices[i].group][(uvy * W + uvx) * 3] / 255., 2.2),
                                //             std::pow(textures[indices[i].group][(uvy * W + uvx) * 3 + 1] / 255., 2.2),
                                //             std::pow(textures[indices[i].group][(uvy * W + uvx) * 3 + 2] / 255., 2.2));
                            }
                            else
                            {
                                color = albedo;
                            }
                        }
                }
            }
        }
        return has_inter;
    };

    void rescale(double scale) {
        for (int i = 0; i < vertices.size(); i++) {
            vertices[i] = vertices[i] * scale; // Multiplie chaque composante du vecteur par 'scale'.
        }
    }

    void translate(const Vector &translation) {
        for (int i = 0; i < vertices.size(); i++) {
            vertices[i] = vertices[i] + translation;
        }
    }


    // void rotate(int axis, double angle) {
    //     if (axis < 0 || axis > 2) {
    //         std::cerr << "Invalid axis for rotation: " << axis << std::endl;
    //         return;
    //     }
    //     int axis1 = (axis + 1) % 3;
    //     int axis2 = (axis + 2) % 3;
    //     double radAngle = angle * M_PI / 180.0;
    //     double cosAngle = cos(radAngle);
    //     double sinAngle = sin(radAngle);
    //     for (size_t i = 0; i < vertices.size(); i++) {
    //         double tempAxis1Value = vertices[i][axis1];
    //         double tempAxis2Value = vertices[i][axis2];
    //         vertices[i][axis1] = cosAngle * tempAxis1Value - sinAngle * tempAxis2Value;
    //         vertices[i][axis2] = sinAngle * tempAxis1Value + cosAngle * tempAxis2Value;
    //     }
    // }


    void rotate(int axis, double angle)
    {
        if (axis < 0 || axis > 2) {
            std::cerr << "Invalid axis for rotation: " << axis << std::endl;
            return;
        }
        int axis1, axis2;
        if (axis == 0)
            {
                axis1 = 1;
                axis2 = 2;
            }
        if (axis == 2)
            {
                axis1 = 0;
                axis2 = 1;
            }
        if (axis == 1)
            {
                axis1 = 0;
                axis2 = 2;
            }
        int axevalue;
        double radAngle = angle * M_PI / 180.0;
        double cosAngle = cos(radAngle);
        double sinAngle = sin(radAngle);
        for (int i = 0; i <= vertices.size(); i++)
        {
            axevalue = vertices[i][axis1];
            vertices[i][axis1] = cosAngle * axevalue - sinAngle * vertices[i][axis2];
            vertices[i][axis2] = sinAngle * axevalue + cosAngle * vertices[i][axis2];
        }
    }

    void loadTexture(const char *filename)
    {
        int W, H, C;
        unsigned char *texture = stbi_load(filename, &W, &H, &C, 3);
        Wtex.push_back(W);
        Htex.push_back(H);
        textures.push_back(texture);
    }

    std::vector<TriangleIndices> indices;
    std::vector<Vector> vertices;
    std::vector<Vector> normals;
    std::vector<Vector> uvs;
    std::vector<Vector> vertexcolors;
    std::vector<unsigned char *> textures;
    std::vector<int> Wtex, Htex;
    BoundingBox bb;
    BVHNode *BVH;
};



class Sphere : public Object {
public:
    bool board;
    bool istexture;
    int textureWidth, textureHeight;
    unsigned char* texture;


    Sphere(const Vector &centre, double R, const Vector &albedo, bool isMiroir = false, bool transp = false, bool mapSphere_tri = false, bool mapSphere_sqr = false ,char* texturePath = nullptr ) : centre(centre), R(R) {
        this->albedo = albedo;
        this->Miroir = isMiroir;
        this->transparent = transp;
        this->mapSphere_tri = mapSphere_tri;
        this->map_sphere_sqr = mapSphere_sqr;
        // this->istexture = istexture;
        texture = nullptr;
        if (texturePath) {
            loadTexture2(texturePath);
        }
    }

    ~Sphere() {
        if (texture) stbi_image_free(texture);
    }

    bool intersect(const Rayon &r, Vector &P, Vector &N, double &t, Vector &color) override {
        // a*t^2 + b*t + c = 0
        double a = 1;
        double b = 2 * dot(r.direction, r.origin - centre);
        double c = (r.origin - centre).norm2() - R * R;
        double delta = b * b - 4 * a * c;
        if (delta < 0) return false;

        double t1 = (-b - sqrt(delta)) / (2 * a);
        double t2 = (-b + sqrt(delta)) / (2 * a);
        if (t2 < 0) return false;
        t = (t1 > 0) ? t1 : t2;

        P = r.origin + t * r.direction;
        N = (P - centre).getNormalized();
        if(map_sphere_sqr)
        {
            Vector Pc = P - centre;
            double phi = atan2(Pc[2], Pc[0]);
            double theta = acos(Pc[1]/ R);
            double u = (phi + M_PI) / (2 * M_PI);
            double v = (M_PI - theta) / M_PI;
            int checkWidth = 20; 
            int checkHeight = 20;
            int i = (int)(u * checkWidth);
            int j = (int)(v * checkHeight);
            if ((i + j) % 2 == 0) {
                color = Vector(1, 1, 1); 
                // color = Vector(1, 0.2, 0.4); 
            } else {
                color = Vector(0,0,0);
                // Vector lightBlue(0x3D / 255.0, 0xDD / 255.0, 0xD6 / 255.0); 
                // color = lightBlue; 
            }
        }
        else {
            color = this->albedo; 
        }

        return true;
    }

    void loadTexture2(const char* texturePath) {
        int n; 
        texture = stbi_load(texturePath, &textureWidth, &textureHeight, &n, 3);
        if (!texture) {
            std::cerr << "Texture loading failed for file: " << texturePath << std::endl;
        }
    }

    Vector centre;
    double R;
};

class Scene
{
public:
    Scene(){};
    void addObject(Object& o) { objects.push_back(&o); };
    std::vector<Object *> objects;
    Vector position_lumiere;
    double intensite_lumiere;
    bool intersect(const Rayon &r, Vector &P, Vector &N, Vector &albedo, bool &mirror, bool &transp, double &t, int &objectId)
    {
        t = std::numeric_limits<double>::max();
        bool has_inter = false;
        for (int i = 0; i < objects.size(); i++)
        {
            Vector localP, localN, localAlbedo;
            double localt;
            if (objects[i]->intersect(r, localP, localN, localt, localAlbedo) && localt < t)
            {
                t = localt;
                has_inter = true;
                albedo = localAlbedo;
                P = localP;
                N = localN;
                mirror = objects[i]->Miroir;
                transp = objects[i]->transparent;
                objectId = i;
            }
        }
        return has_inter;
    };

    Vector getColor(const Rayon &r, int rebondRecursive, bool b=false)
    {
        if (rebondRecursive > MAX_REBONDS) return Vector(0., 0., 0.);
        
        Vector P, N, albedo;
        double t;
        bool miroir, transp;
        int objectId;
        bool has_inter = intersect(r, P, N, albedo, miroir, transp, t, objectId);
        Vector color(0, 0, 0);

        if (has_inter)
        {
            if (objectId == 0)
            {
                if (rebondRecursive == 0 || !b)
                {
                    // approche naive
                    Vector I = Vector(intensite_lumiere, intensite_lumiere, intensite_lumiere);
                    double R2 = sqr(dynamic_cast<Sphere *>(objects[0])->R);
                    return I / (4 * sqr(M_PI) * R2); 
                }
                return Vector(0., 0., 0.);
            }
            if (miroir)
            {
                // R = i - 2*dot(N,i)*N
                Vector direction_miroir = r.direction - 2 * dot(r.direction, N) * N;
                // Rayon Rayon_miroir(P, direction_miroir);
                Rayon Rayon_miroir(P + 0.00001 * N, direction_miroir);
                return getColor(Rayon_miroir, rebondRecursive + 1);
            }

            if (transp) {
                // calcul de la tarnsparence
                // double n1 = 1.0;
                // double n2 = 1.3;
                // Vector N_refract(N);
                // double cosi = dot(r.direction, N);
                // double ti = 1-sqr(n1/n2)*(1-sqr(cosi));
                // // check direction du rayon par rapport à la normale de la sphere
                // if (cosi > 0){
                //     n1 = 1.3;
                //     n2 = 1;
                //     N_refract = - N;
                // }
                // if (ti > 0){
                //     Vector T_normal = - sqrt(ti) * N_refract;
                //     Vector T_tangent = n1/n2 * (r.direction - cosi*N_refract);
                //     Vector T_transmis = T_normal + T_tangent; 
                //     Rayon rayon_transp (P-0.0001*N_refract, T_transmis);
                //     intensite_pix = getColor(rayon_transp, nbrebonds + 1);
                // } 
                double n1 = 1;
                double n2 = 1.3;
                Vector N2 = N;
                if (dot(r.direction, N) > 0) { 
                    n1 = 1.3;
                    n2 = 1;
                    N2 = -N;
                }

                double cosTheta = -dot(N2, r.direction);
                double r0 = sqr((n1 - n2) / (n1 + n2));
                double R = r0 + (1 - r0) * pow(1 - cosTheta, 5);  //approximation de Schlick
                double T = 1 - R;
                double reflectProb = R;
                double rand= uniform(engine); 

                if (rand < reflectProb) {
                    Vector dirReflechi = r.direction - 2 * dot(r.direction, N) * N;
                    Rayon RayReflechi(P + 0.0001 * N, dirReflechi);
                    // Rayon RayReflechit(P, directionReflechit);
                    return getColor(RayReflechi, rebondRecursive + 1);
                } 
                else 
                {
                    double ti = 1 - sqr(n1 / n2) * (1 - sqr(cosTheta));
                    if (ti < 0) {
                            // reflection Total interne
                            Vector dirReflechi = r.direction - 2 * dot(r.direction, N) * N;
                            Rayon RayReflechi(P + 0.0001 * N, dirReflechi);
                            return getColor(RayReflechi, rebondRecursive + 1);
                    }
                    Vector transmitted_ray = n1 / n2 * (r.direction + cosTheta * N2) -sqrt(1 - (n1 / n2 * (r.direction + cosTheta * N2)).norm2()) * N2;
                    return getColor(Rayon(P - 0.0001 * N2, transmitted_ray), rebondRecursive + 1);
                }
            }
            else 
            {
                // eclairage direct

                // Ray ray_light(P + 0.0001 * N, (s.position_lumiere - P).getNormalized());
                // Vector P_light, N_light;
                // int sphere_id_light;
                // double t_light;
                // bool has_inter_light = s.intersection(ray_light, P_light, N_light, sphere_id_light, t_light);
                // double d_light2 = (s.position_lumiere - P).norm2();
                // if (has_inter_light && t_light * t_light < d_light2 - 0.0001) {
                //     intensite_pix = Vector(1, 1, 1);
                // }
                // else {
                //     double d_light2_inv = 1 / d_light2;
                //     Vector light_dir_normalized = (s.position_lumiere - P).getNormalized();
                //     double light_intensity = s.intensite_lumiere * std::max(0., dot(light_dir_normalized, N)) * d_light2_inv;
                //     intensite_pix = s.spheres[sphere_id].albedo * light_intensity + ambient_light; 
                // }

                // eclairage direct

                // ovecteur LP allant de P à l'origine de la source lumineuse
                Vector LP = (P - position_lumiere).getNormalized() ;
                // on génère un vecteur w qui pointe dans une direction aléatoire partant de P vers la demi-sphère centrée sur la source lumineuse
                Vector w = random_cos(LP);
                //On choisit aléatoirement un point xprime sur la surface de la sphère lumineuse
                Vector xprime = w * dynamic_cast<Sphere *>(objects[0])->R + dynamic_cast<Sphere *>(objects[0])->centre;
                Vector Pxprime = xprime - P;
                double d_xprime = sqrt(Pxprime.norm2()); // recupere la  distance entre P et xprime
                Pxprime = Pxprime / d_xprime; // normlise
                Vector Pprime, N_prime, albedo_prime;
                double tprime;
                int objectId;
                bool MiroirPrime, transpPrime;
                Rayon RayonPrime(P + 0.00001 * N, Pxprime);
                bool has_interPrime = intersect(RayonPrime, Pprime, N_prime, albedo_prime, MiroirPrime, transpPrime, tprime, objectId);
                // facteur de visibilité
                if (has_interPrime && tprime < d_xprime*0.99)
                    {
                        color = Vector(0., 0., 0.);
                    }
                else
                    {
                        // calcul de l'éclairage direct en tenant compte de la source étendue avec la formule du cours
                        double R_sqr = sqr(dynamic_cast<Sphere *>(objects[0])->R);
                        double proba_xprime = std::max(1E-8, dot(LP, w)) / (M_PI * R_sqr);
                        double Jacobien = std::max(0., dot(w, -Pxprime)) / sqr(d_xprime);
                        Vector BRDF = albedo / M_PI;
                        double cos_theta_prime = std::max(0., dot(N, Pxprime));
                        color = (intensite_lumiere / (4 * sqr(M_PI)* R_sqr)) * BRDF * cos_theta_prime * Jacobien / proba_xprime;
                    }

                // eclairage indirect
                Vector wi = random_cos(N);
                Rayon randomRay(P + 0.00001 * N, wi);
                color += albedo * getColor(randomRay, rebondRecursive + 1, true);
            }
        }
        return color;
    }
};



