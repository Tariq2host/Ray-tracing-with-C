#include "raytracer.h"

int main()
{
    auto start = std::chrono::high_resolution_clock::now();
    float ini_time = clock();
    int W = 256;
    int H = 256;
    double fov = 60 * M_PI / 180;
    int nbrays = 10;
    double focus_distance = 55;
    int progress_counter = 0;
    Vector C(0, 0, 70);
    Vector h_up(0, 1, 0); 
    Vector w_right(1, 0, 0); 
    Vector Z_view(0, 0, -1); 
    // double Vert = -20; 
    // double Hori = 5; 
    // adjustOrientationCamera(h_up, w_right, Z_view, Vert, Hori);
    Scene s;
    s.intensite_lumiere = 6E9;
    s.position_lumiere = Vector(-15, 40, -90);   //-15, 38, -90

    Vector lightBlue(0x3D / 255.0, 0xDD / 255.0, 0xD6 / 255.0);     // #3DDDD6
    Vector veryLightBlue(0xAD / 255.0, 0xDD / 255.0, 0x98 / 255.0); // #ADDD98


    // x c'est la largeur
    // y c'est la hauteur
    // z c'est la profondeur
    Sphere Slum(s.position_lumiere, 17, Vector(1., 1., 1.), false, false);
    Sphere s1(Vector(0, -2000 - 20, -55), 1990, Vector(0.5, 0.5, 0.5), false, false, false, false); // Sol de couleur rose rougeâtre
    // s1.loadTexture("Models/sol_tex/Carpet016.png");
    Sphere s2(Vector(0, 2000 + 100, -55), 2000, Vector(1, 1, 1)); // Plafond blanc
    // s2.loadTexture2("Models/sol_tex/Carpet016.png");
    Sphere s3(Vector(-2000 - 50, 0, -55), 1970, Vector(1, 0.2, 0.4)); // Mur gauche de couleur rose pâle
    Sphere s4(Vector(2000 + 50, 0, -55), 1970, Vector(1, 0.2, 0.4)); // Mur droit de couleur bleu pâle
    Sphere s5(Vector(0, 0, -2000 - 100), 1970, lightBlue, false, false, true); // Mur de fond blanc
    Sphere s6(Vector(0, 0, +2000 + 100), 1970,  lightBlue); // Mur de fond derriere la camera et blanc
    Sphere s7(Vector(10, -25, 17), 7, Vector(1., 1., 1.), false, true); // sphere transarente
    Sphere s8(Vector(-40, -19, -50), 13, Vector(1., 1., 1.), true, false); // sphere miroir
    Sphere s9(Vector(-37, -24, -15), 7, Vector(1., 1., 1.), false, false, false, true); // sphere mapper




    // chat
    TriangleMesh m1(Vector(0., 1., 1.), false, false);
    m1.readOBJ("./Models/cat.obj");
    m1.loadTexture("./Models/cat_diff.png");
    m1.rescale(0.3);
    Vector translation1(-17, -30, -1);
    m1.translate(translation1);
    m1.buildBVH(m1.BVH, 0, m1.indices.size());


    // Tapis
    TriangleMesh m2(Vector(1, 0.6, 0.8), false, false);
    m2.readOBJ("./Models/tapis.obj");
    m2.rescale(22);
    Vector translation_tapis(0, -30, -35);
    m2.translate(translation_tapis);
    m2.buildBVH(m2.BVH, 0, m2.indices.size());


    // table
    TriangleMesh m3(Vector(1, 1, 0.6), false, true);
    m3.readOBJ("./Models/round_tablev2.obj");
    m3.rescale(0.7);
    Vector translation_table(0, -43, -35);
    m3.translate(translation_table);
    // m3.rotate(1,90);
    // Vector translation_table2(-30, 0 , -20);
    // m3.translate(translation_table2);
    m3.buildBVH(m3.BVH, 0, m3.indices.size());


    // small bowl
    TriangleMesh m4(Vector(1, 1, 1), true);
    m4.readOBJ("./Models/small_Bowl.obj");
    m4.rescale(6);
    Vector translation_bowl(0, -20, -35);
    m4.translate(translation_bowl);
    m4.buildBVH(m4.BVH, 0, m4.indices.size());



    // sofa
    TriangleMesh m5(Vector(1., 1., 1.), false, false);
    m5.readOBJ("./Models/sofa.obj");
    m5.rescale(10);
    Vector translation_sofa(0, -30, -65); //15
    m5.translate(translation_sofa);
    m5.rotate(1,90);
    Vector translation_sofa2(-69, 0 , -65);
    m5.translate(translation_sofa2);
    m5.buildBVH(m5.BVH, 0, m5.indices.size());

    // scène
    s.addObject(Slum);
    s.addObject(s1);
    s.addObject(s2);
    s.addObject(s3);
    s.addObject(s4);
    s.addObject(s5);
    s.addObject(s6);
    s.addObject(s7);
    s.addObject(s8);
    s.addObject(s9);
    s.addObject(m1);
    s.addObject(m2);
    s.addObject(m2);
    s.addObject(m3);
    s.addObject(m4);
    s.addObject(m5);

    std::vector<unsigned char> image(W * H * 3, 0);
#pragma omp parallel for schedule(dynamic, 1)
    for (int i = 0; i < H; i++)
    {
        for (int j = 0; j < W; j++)
        {
            Vector color(0, 0, 0);
            for (int k = 0; k < nbrays; k++)
            {
                // methode de Box Muller 
                double r1 = uniform(engine), r2 = uniform(engine);
                double g1 = 0.25 * cos(2 * M_PI * r1) * sqrt(-2 * log(r2));
                double g2 = 0.25 * sin(2 * M_PI * r1) * sqrt(-2 * log(r2));
                // chaque rayon légèrement décalé par rapport au centre du pixek
                Vector u(j - W / 2 + g2 + 0.5, i - H / 2 + g1 + 0.5, W / (2. * tan(fov / 2)));
                u = u.getNormalized();
                // ajuster orientation camera
                u = u[0] * w_right + u[1] * h_up + u[2] * Z_view;

                // depth of field
                double r3 = uniform(engine), r4 = uniform(engine);
                // double r3 = (uniform(engine)-0.5)*5, r4 =(uniform(engine)-0.5)*5;
                double dx_aperture = 0.01 * cos(2 * M_PI * r3) * sqrt(-2 * log(r4));
                double dy_aperture = 0.01 * sin(2 * M_PI * r3) * sqrt(-2 * log(r4));
                Vector destination = C + focus_distance * u;
                // Vector new_C = C + Vector(r3, r4, 0);
                Vector new_C = C + Vector(dx_aperture, dy_aperture, 0);
                Vector new_u = (destination - new_C).getNormalized();
                Rayon r(new_C, new_u);
                color += s.getColor(r, 0);
            }

            color = color / nbrays;

            image[((H - i - 1) * W + j) * 3 + 0] = std::min(255., std::pow(color[0], 1/2.2));
            image[((H - i - 1) * W + j) * 3 + 1] = std::min(255., std::pow(color[1], 1/2.2));
            image[((H - i - 1) * W + j) * 3 + 2] = std::min(255., std::pow(color[2], 1/2.2));
        }
        #pragma omp atomic
        ++progress_counter;
        #pragma omp critical
        {
            std::cout << "\rRendering progress: " << (progress_counter * 100.0 / H) << "%" << std::flush;
        }
        // Termine la mesure du temps et calcule la durée
        auto finish = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = finish - start;
        std::cout << std::endl << "Time elapsed so far: " << elapsed.count() << " seconds" << std::endl;
    }

    // wwriting the image on a png file
    stbi_write_png("rendu_finale.png", W, H, 3, &image[0], 0);

    // writing execution time on a txt file
    std::ofstream execution_file;
    execution_file.open("runtime.txt");
    execution_file << (clock() - ini_time) / CLOCKS_PER_SEC;
    execution_file.close();

    return 0;
};

// g++ main.cpp -o main.exe -O3 -fopenmp