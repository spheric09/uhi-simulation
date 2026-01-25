#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <array>
#include <string>

constexpr int GRID_W = 250; //physics grid size
constexpr int GRID_H = 250;
constexpr float CELL_SIZE_PX = 4.0f; //pixels per cell edge
constexpr float phyCellSize = 4.0f; // physical edge length in meters
constexpr float PIXELS_PER_METER = CELL_SIZE_PX / phyCellSize;
constexpr float cellArea = phyCellSize * phyCellSize; // m^2 

constexpr float DT = 5.0f;     // no. of seconds per timestep
constexpr float DAY_LENGTH = 24 * 3600; //no. of seconds in a day
constexpr float TIME_SCALE = 10.0f; // speed up factor
constexpr float SOLAR = 850.0f;   // W/m^2 max solar insolation rate (incoming)
constexpr float scaledDT = DT * TIME_SCALE;
constexpr float T_MIN = 283.15f; //K
constexpr float T_MAX = 348.15f; //K
constexpr float PI = 3.14159265358979323846f;
constexpr float sigma = 5.67e-8f; //Steffan-Boltzmann constant


float clamp(float x, float a, float b) { //to bound the temp values for color mapping
    return std::max(a, std::min(x, b));
}

float curSolarInsolation(float t) {
    float curPhase = std::fmod(t, DAY_LENGTH) / DAY_LENGTH; // fmod is % for floats; returns smthg bw 0 and 1
    float curSun = std::sin(curPhase * 2 * PI - PI/2.0f);
    return std::max(0.0f, curSun);
}


enum class MaterialID {
    Asphalt,
    Concrete,
    Vegetation,
    Water,
    whiteRoof
};

class MaterialProps {
public:

    // Material constants:

    float albedo; //dimensionless
    float emissivity; //dimensionless
    float density; // kg/m^3
    float specificHeat; // J/(kg.K)
    float conductivity; // W/(m.K)
    

    // Derived units:

    float diffusivity() const {
        return conductivity / (density * specificHeat);
    }

    float penetrationDepth() const {
        return std::sqrt(diffusivity() * DAY_LENGTH);
    }

    float heatCapacity() const { //heat capacity of the cell = rho*Cp*h*A
        return density * specificHeat * penetrationDepth() * cellArea;
    }

    

};


class MaterialDB {
public:

    static const std::array<MaterialProps, 5> materials;

    static const MaterialProps& get(MaterialID id)
    {
        return materials[static_cast<int>(id)];
    }

};


const std::array<MaterialProps, 5> MaterialDB::materials =
{
    // Asphalt
    MaterialProps{
        0.07f, 0.95f, 1700.f, 1000.f, 0.50f
    },

    // Concrete (RCC)
    MaterialProps{
        0.30f, 0.90f, 2288.f, 880.f, 1.58f
    },

    // Vegetation
    MaterialProps{
        0.25f, 0.96f, 650.f, 1700.f, 0.38f
    },

    // Water
    MaterialProps{
        0.03f, 0.97f, 990.f, 4186.f, 0.63f
    },

    // White roof (I couldn't find trustworthy data for this)
    MaterialProps{
        0.75f, 0.90f, 2200.f, 900.f, 1.00f
    }
};

//function to make transition from one color to another smooth
sf::Color colorTransition(const sf::Color& ca, const sf::Color& cb, float n) {
    n = clamp(n, 0.0f, 1.0f);

    std::uint8_t r = static_cast<std::uint8_t>(ca.r + n * (cb.r - ca.r));
    std::uint8_t g = static_cast<std::uint8_t>(ca.g + n * (cb.g - ca.g));
    std::uint8_t b = static_cast<std::uint8_t>(ca.b + n * (cb.b - ca.b));

    return sf::Color(r, g, b);
}


//i might want to add Gamma correction somewhere

// temp to color mapping
sf::Color heatColor(float T) {
    T = clamp(T, T_MIN, T_MAX); //apply bound
    float n = (T - T_MIN) / (T_MAX - T_MIN); //normalizatio
    n = clamp(n, 0.0f, 1.0f);

    const sf::Color white(255, 255, 255);
    const sf::Color yellow(255, 255, 0);
    const sf::Color orange(255, 128, 0);
    const sf::Color red(255, 0, 0);
    const sf::Color purple(128, 0, 255);

    if (n < 0.3) {
        return colorTransition(white, yellow, n / 0.3f);
    }
    else if (n < 0.6) {
        return colorTransition(yellow, orange, (n - 0.3f) / 0.3f);
    }
    else if (n < 0.85) {
        return colorTransition(orange, red, (n - 0.6f) / 0.25f);
    }
    else {
        return colorTransition(red, purple, (n - 0.85f) / 0.15f);
    }
}

// effective conductivity calculator (for conduction on boundaries)
float k_eff(int i, int j, const std::vector<MaterialID>& mat)
{
    float k1 = MaterialDB::get(mat[i]).conductivity;
    float k2 = MaterialDB::get(mat[j]).conductivity;
    return 2.0f * k1 * k2 / (k1 + k2);
}


std::string materialName(MaterialID m) {
    switch (m) {
    case MaterialID::Asphalt: return "Asphalt";
    case MaterialID::Concrete: return "Concrete";
    case MaterialID::Vegetation: return "Vegetation";
    case MaterialID::Water: return "Water";
    case MaterialID::whiteRoof: return "White Roof";
    default: return "Unknown";
    }
}

MaterialID pixelToMaterial(const sf::Color& c)
{
    int r = c.r;
    int g = c.g;
    int b = c.b;

    int brightness = (r + g + b) / 3;

    int dr = g - r;
    int db = g - b;

    int br = b - r;
    int bg = b - g;

    int maxc = std::max({ r, g, b });
    int minc = std::min({ r, g, b });
    int contrast = maxc - minc;

    // ---- WATER: strong blue dominance ----
    if (b > r + 20 && b > g + 20)
        return MaterialID::Water;

    // ---- VEGETATION: green wins even if dark ----
    if (g > r + 30 && g > b + 30)
        return MaterialID::Vegetation;

    // ---- DARK NEUTRAL = vegetation ----
    //if (brightness < 100 && contrast < 35 && g > r && g > b)
    //    return MaterialID::Vegetation;

    // ---- BRIGHT NEUTRAL = asphalt ----
    if (brightness > 130 && contrast < 35)
        return MaterialID::Asphalt;

    // ---- SHADOWED VEGETATION RECOVERY ----
    if (brightness < 100 && g > r && g > b)
        return MaterialID::Vegetation;

    // ---- FALLBACK ----
    if (brightness < 120)
        return MaterialID::Concrete;

    return MaterialID::Concrete;
}


std::string displayTime(float simTime)
{
    float t = std::fmod(simTime, DAY_LENGTH);

    int totalSeconds = static_cast<int>(t);

    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;

    std::string h = std::to_string(hours);
    std::string m = std::to_string(minutes);

    return h + ":" + m;
}


int main() {
    const int WIN_W = GRID_W * CELL_SIZE_PX; // Decalre window size
    const int WIN_H = GRID_H * CELL_SIZE_PX;

    sf::RenderWindow window(            // Render window
        sf::VideoMode({ WIN_W, WIN_H }),
        "Urban Heat Island - Phase 3"
    );

    sf::Font font;
    if (!font.openFromFile("Roboto-Regular.ttf"))
    {
        std::cerr << "Failed to load font\n";
    }

    sf::Text overlayText(font);     //declare overlay text
    overlayText.setCharacterSize(18);
    overlayText.setFillColor(sf::Color::Black);
    overlayText.setPosition(sf::Vector2f(10.f, 10.f));


    sf::RectangleShape overlayBg;   //declare bg rectangle for visibility
    overlayBg.setFillColor(sf::Color(120, 120, 120, 160)); // grey + transparency
    overlayBg.setPosition(sf::Vector2f(5.f, 5.f));
    overlayBg.setSize(sf::Vector2f(180.f, 140.f));

    sf::Image baseImg;
    if (!baseImg.loadFromFile("delhi_base1.png")) {
        std::cerr << "Failed to load base map\n";
    }



    // Temp grid
    std::vector<float> T(GRID_W * GRID_H, 0.f); //representing 2d grid through 1d indexing
    std::vector<float> T_new = T;

    // Material grid (city map)
    std::vector<MaterialID> mat(GRID_W * GRID_H, MaterialID::Vegetation);
    for (int y = 0; y < GRID_H; y++) {
        for (int x = 0; x < GRID_W; x++) {

            int i = y * GRID_W + x;

            unsigned int px = x * baseImg.getSize().x / GRID_W;
            unsigned int py = y * baseImg.getSize().y / GRID_H;

            sf::Color c = baseImg.getPixel({ px, py });

            mat[i] = pixelToMaterial(c);

            T[i] = 288.0f;
        }
    }

    sf::VertexArray cells(sf::PrimitiveType::Triangles, GRID_W * GRID_H * 6);
    // sending a draw call to the GPU (a list of points (vertices) that the GPU draws in one go)
    // each grid cell is 1 square, each square is 2 triangles, meaning 6 vertices
    // total cells = GRID_W * GRID_H

    //precompute the static grid
    // RENDER
    for (int y = 0; y < GRID_H; y++) {
        for (int x = 0; x < GRID_W; x++) {
            int i = y * GRID_W + x;
            int v = 6 * i; //starting vertex of cell i

            float px = x * CELL_SIZE_PX; // convert grid coords into vertex coords
            float py = y * CELL_SIZE_PX;

            // Triangle 1
            cells[v + 0].position = { px, py };
            cells[v + 1].position = { px + CELL_SIZE_PX, py };
            cells[v + 2].position = { px + CELL_SIZE_PX, py + CELL_SIZE_PX };

            // Triangle 2
            cells[v + 3].position = { px, py };
            cells[v + 4].position = { px + CELL_SIZE_PX, py + CELL_SIZE_PX };
            cells[v + 5].position = { px, py + CELL_SIZE_PX };
            
        }
    }

    MaterialID brushMaterial = MaterialID::Asphalt;
    int brushRadius = 0; // 0 = single cell

    float simTime = 0.0f;

    while (window.isOpen()) {
        while (auto event = window.pollEvent()) { // Ask the operating system: did anything happen?
            //pollEvent() checks the OS event queue
            
            if (event->is<sf::Event::Closed>()) {   // is this even a 'window close' event
                window.close();
            }

            if (event->is<sf::Event::KeyPressed>()) {

                auto key = event->getIf<sf::Event::KeyPressed>()->code;

                if (key == sf::Keyboard::Key::Num1)
                    brushMaterial = MaterialID::Asphalt;

                if (key == sf::Keyboard::Key::Num2)
                    brushMaterial = MaterialID::Concrete;

                if (key == sf::Keyboard::Key::Num3)
                    brushMaterial = MaterialID::Vegetation;

                if (key == sf::Keyboard::Key::Num4)
                    brushMaterial = MaterialID::Water;

                if (key == sf::Keyboard::Key::Num5)
                    brushMaterial = MaterialID::whiteRoof;

                // Brush size
                if (key == sf::Keyboard::Key::Num9)
                    brushRadius++;

                if (key == sf::Keyboard::Key::Num0)
                    brushRadius = std::max(0, brushRadius - 1);
            }
        }

        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {

            auto pos = sf::Mouse::getPosition(window);

            int cx = pos.x / CELL_SIZE_PX;
            int cy = pos.y / CELL_SIZE_PX;

            for (int dy = -brushRadius; dy <= brushRadius; dy++) {
                for (int dx = -brushRadius; dx <= brushRadius; dx++) {

                    int x = cx + dx;
                    int y = cy + dy;

                    if (x >= 0 && x < GRID_W &&
                        y >= 0 && y < GRID_H) {

                        int i = y * GRID_W + x;
                        
                        mat[i] = brushMaterial;
                    }
                }
            }
        }


        simTime += scaledDT;

        // PHYSICS

        float curSolar = curSolarInsolation(simTime);

        for (int y = 1; y < GRID_H - 1; y++) { //skip boundary cells (for now) since they dont have all 4 neighbours
            for (int x = 1; x < GRID_W - 1; x++) {
                int i = y * GRID_W + x; //convert 2D grid to 1D indexing

                const MaterialProps& p = MaterialDB::get(mat[i]);

                float C = p.heatCapacity();

                // Diffusion
                float Qcond = //assuming mostly surface transfer, considering the depth only in Heat Capacity, not in conduction
                    k_eff(i-1, i, mat) * (T[i - 1] - T[i]) +
                    k_eff(i+1, i, mat) * (T[i + 1] - T[i]) +
                    k_eff(i-GRID_W, i, mat) * (T[i - GRID_W] - T[i]) +
                    k_eff(i+GRID_W, i, mat) * (T[i + GRID_W] - T[i]);

                // Incoming solar
                float Qsolar =
                    (1.0f - p.albedo) * curSolar * SOLAR * cellArea;

                // steffan boltzmann radiative cooling
                float T_k = T[i];
                float Tsky = 283.0f; //K

                float Qrad = p.emissivity * sigma * cellArea * (pow(T_k, 4) - pow(Tsky, 4));

                // Evaporation
                float Qevap = 0.0f;

                if (mat[i] == MaterialID::Vegetation) {

                    float evapCoeff = 0.00004f;

                    float evap = evapCoeff * curSolar * std::max(0.f, T[i] - 285.f);

                    Qevap = evap * 2.45e6f * cellArea;
                }

                else if (mat[i] == MaterialID::Water) {

                    float evapCoeff = 0.00009f; // stronger

                    float evap = evapCoeff * (curSolar) * std::max(0.f, T[i] - 285.f);

                    Qevap = evap * 2.45e6f * cellArea;
                }


                // Update (master equation)
                float Qnet = Qcond + Qsolar - Qrad - Qevap;

                T_new[i] = T[i] + scaledDT * Qnet / C;
            }
        }

        std::swap(T, T_new);

        float minTemp = 1e9f;
        float maxTemp = -1e9f;
        float sumTemp = 0.0f;

        for (int y = 1; y < GRID_H - 1; y++) {      //note: ignoring boundary cells for these stats as well
            for (int x = 1; x < GRID_W - 1; x++) {

                int i = y * GRID_W + x;
                float temp = T[i];

                minTemp = std::min(minTemp, temp);
                maxTemp = std::max(maxTemp, temp);
                sumTemp += temp;
            }
        }

        float meanTemp = sumTemp / ((GRID_H-2) * (GRID_W-2));

        

        overlayText.setString(
            "Time: " + displayTime(simTime) + "\n" +
            "Min:  " + std::to_string(minTemp) + " K\n" +
            "Max:  " + std::to_string(maxTemp) + " K\n" +
            "Mean: " + std::to_string(meanTemp) + " K\n" +
            "Brush: " + materialName(brushMaterial) + "\n" +
            "Radius: " + std::to_string(brushRadius)
            
        );

        //render the new colors(temp update)
        for (int i = 0; i < GRID_W * GRID_H; i++) {
            sf::Color c = heatColor(T[i]); //determine temperature color of the cell
            int v = 6 * i;
            for (int k = 0; k < 6; k++) //apply color to all 6 vertices
                cells[v + k].color = c;
        }


        // DEBUG: show materials
        /*for (int i = 0; i < GRID_W * GRID_H; i++) {

            sf::Color c;

            switch (mat[i]) {
            case MaterialID::Water:
                c = sf::Color::Blue;
                break;

            case MaterialID::Vegetation:
                c = sf::Color::Green;
                break;

            case MaterialID::Asphalt:
                c = sf::Color(50, 50, 50);
                break;

            case MaterialID::Concrete:
                c = sf::Color(200, 200, 200);
                break;
            }

            int v = 6 * i;

            for (int k = 0; k < 6; k++)
                cells[v + k].color = c;
        }*/

        window.clear();
        window.draw(cells);
        window.draw(overlayBg);
        window.draw(overlayText);
        window.display();

    }

    return 0;
}
