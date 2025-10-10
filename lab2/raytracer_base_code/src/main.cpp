#include <memory>
#include <iostream>
#include <filesystem>
#include "camera.h"
#include "plane.h"
#include "scene.h"
#include "sphere.h"
#include "triangle.h"
#include "obj_loader.h"

namespace fs = std::filesystem;
namespace rt { void render_scene_ppm(const Scene&, const Camera&, int, const std::string&); }

int main(int argc, char** argv){
    using namespace rt;
    const int W=800, H=600, SPP=4; // Width, Height, Samples Per Pixel
    Camera cam({0,1,4}, {0,1,0}, {0,1,0}, 45.0, W, H);

    Scene sc;
    // Define the material for ground plane and add it to the scene
    int matGrey  = sc.addMaterial({{0.8,0.8,0.8}});
    sc.add(std::make_unique<Plane>(Vec3{0,0,0}, Vec3{0,1,0}, matGrey));

    // Add a point light to the scene
    sc.lights.push_back({Vec3{2,3,2}, Vec3{30,30,30}});

    // Loads an object; Store .obj files in assets folder!
    std::string objPath = (argc>1)? std::string(argv[1]) : std::string("../assets/dragon_res3.obj");
    std::vector<Vec3> V; 
    std::vector<uint32_t> I;
    bool loaded = fs::exists(objPath) && load_obj_positions_indices(objPath, V, I) && !V.empty() && !I.empty();
    if (loaded) {
        std::cerr << "Loaded OBJ: " << objPath << "  V=" << V.size() << "  T=" << I.size()/3 << "\n";
        int matBunny = sc.addMaterial({{0.8,0.8,0.9}});
        add_mesh(sc, V, I, matBunny, /*scale*/{3,3,3}, /*translate*/{0,0.6,0});
    } else {
        std::cerr << "OBJ not found or failed to load. Proceeding without mesh.\n";
    }

    // Define Red, Green, and Blue Materials
    int matRed = sc.addMaterial({{0.8,0.2,0.2}, true});
    int matGreen = sc.addMaterial({{0.2,0.8,0.2}});
    int matBlue = sc.addMaterial({{0.2,0.2,0.8}});
    //add three Spheres, each with its own unique material
    sc.add(std::make_unique<Sphere>(Vec3{-1.2,2.0,0.0}, 0.5, matRed));
    sc.add(std::make_unique<Sphere>(Vec3{1.2,1.0,0.0}, 1.0, matGreen));
    sc.add(std::make_unique<Sphere>(Vec3{0.0,1.0,-2.0}, 0.75, matBlue));

    // Render the scene
    render_scene_ppm(sc, cam, SPP, "out.ppm");
    return 0;
}
