#include <iostream>
#include <vector>
#include <fstream>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <algorithm>
#include <set>

const size_t SPECIMENS = 300;
const int BUFFER_OFFSET = 30;

struct Triangle {
    sf::Vector2i v[3];
    sf::Color c;
};

struct Specimen {
    std::vector<Triangle> triangles;
    int64_t score = 0;
    sf::Image buffer;
};

struct SpecimenComp {
    bool operator()(const Specimen& a, const Specimen& b) const {
        return a.score < b.score;
    }
};

int orient2d(const sf::Vector2i& a, const sf::Vector2i& b, const sf::Vector2i& c) {
    // Source: https://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

void insertTriangle(Specimen& specimen, sf::Vector2i v[3], sf::Color color, const sf::Image & img) {
    // Source: https://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/
    sf::Vector2u size = img.getSize();
    int minX = std::min(std::min(v[0].x, v[1].x), v[2].x);
    int minY = std::min(std::min(v[0].y, v[1].y), v[2].y);
    int maxX = std::max(std::max(v[0].x, v[1].x), v[2].x);
    int maxY = std::max(std::max(v[0].y, v[1].y), v[2].y);

    minX = std::max(minX, 0);
    minY = std::max(minY, 0);
    maxX = std::min(maxX, int(size.x - 1));
    maxY = std::min(maxY, int(size.y - 1));

    sf::Vector2i p;
    for (p.y = minY; p.y <= maxY; p.y++) {
        for (p.x = minX; p.x <= maxX; p.x++) {
            int w0 = orient2d(v[1], v[2], p);
            int w1 = orient2d(v[2], v[0], p);
            int w2 = orient2d(v[0], v[1], p);

            if (w0 >= 0 && w1 >= 0 && w2 >= 0 && p.x > 0 && p.x < size.x && p.y > 0 && p.y < size.y) {
                sf::Color imgColor = img.getPixel(p.x, p.y);
                sf::Color oldColor = specimen.buffer.getPixel(p.x, p.y);
                sf::Color newColor = sf::Color(
                    uint8_t((color.r + oldColor.r) / 2),
                    uint8_t((color.g + oldColor.g) / 2),
                    uint8_t((color.b + oldColor.b) / 2)
                );
                
                specimen.buffer.setPixel(p.x, p.y, newColor);
                specimen.score += - (int64_t(imgColor.r) - oldColor.r) * (int64_t(imgColor.r) - oldColor.r) + (int64_t(imgColor.r) - newColor.r) * (int64_t(imgColor.r) - newColor.r);
                specimen.score += - (int64_t(imgColor.g) - oldColor.g) * (int64_t(imgColor.g) - oldColor.g) + (int64_t(imgColor.g) - newColor.g) * (int64_t(imgColor.g) - newColor.g);
                specimen.score += - (int64_t(imgColor.b) - oldColor.b) * (int64_t(imgColor.b) - oldColor.b) + (int64_t(imgColor.b) - newColor.b) * (int64_t(imgColor.b) - newColor.b);
            }
        }
    }
}

void mutate(Specimen& specimen, const sf::Image& img) {
    sf::Vector2u size = img.getSize();
    sf::Vector2i p[3];
    int det = 0;
    while (p[0] == p[1] || p[1] == p[2] || p[0] == p[2] || det == 0) {
        p[0].x = rand() % (size.x + BUFFER_OFFSET * 2) - BUFFER_OFFSET;
        p[0].y = rand() % (size.y + BUFFER_OFFSET * 2) - BUFFER_OFFSET;
        p[1].x = rand() % (size.x + BUFFER_OFFSET * 2) - BUFFER_OFFSET;
        p[1].y = rand() % (size.y + BUFFER_OFFSET * 2) - BUFFER_OFFSET;
        p[2].x = rand() % (size.x + BUFFER_OFFSET * 2) - BUFFER_OFFSET;
        p[2].y = rand() % (size.y + BUFFER_OFFSET * 2) - BUFFER_OFFSET;
        // Source: https://gamedev.stackexchange.com/a/13231/104538
        det = p[0].x * p[1].y + p[1].x * p[2].y + p[2].x * p[0].y - p[1].y * p[2].x - p[2].y * p[0].x - p[0].y * p[1].x;
    }

    if (det < 0) {
        sf::Vector2i tmp = p[1];
        p[1] = p[2];
        p[2] = tmp;
    }

    uint8_t c = rand();
    sf::Color color = sf::Color(c,c,c);

    Triangle t;
    t.c = color;
    t.v[0] = p[0];
    t.v[1] = p[1];
    t.v[2] = p[2];

    specimen.triangles.push_back(t);
    insertTriangle(specimen, p, color, img);
}

void create(std::set<Specimen, SpecimenComp>& specimens, const sf::Image& img) {
    auto size = img.getSize();

    Specimen root;
    root.buffer.create(size.x, size.y);
    root.score = 0;
    for (unsigned x = 0; x < size.x; x++) {
        for (unsigned y = 0; y < size.y; y++) {
            sf::Color rootColor = sf::Color::Black;
            sf::Color imgColor = img.getPixel(x, y);
            root.score += (int64_t(rootColor.r) - imgColor.r) * (int64_t(rootColor.r) - imgColor.r) + 
                (int64_t(rootColor.g) - imgColor.g) * (int64_t(rootColor.g) - imgColor.g) + 
                (int64_t(rootColor.b) - imgColor.b) * (int64_t(rootColor.b) - imgColor.b);
            root.buffer.setPixel(x, y, sf::Color::Black);
        }
    }

    for (int i = 0; i < SPECIMENS; i++) {
        Specimen spec = root;
        mutate(spec, img);
        specimens.insert(spec);
    }
}

void update(std::set<Specimen, SpecimenComp>& specimens, const sf::Image& img) {
    if (!specimens.size()) {
        return;
    }
    
    Specimen best = *specimens.begin();
    specimens.clear();
    
    for (int i = 0; i < SPECIMENS; i++) {
        Specimen spec = best;
        mutate(spec, img);
        specimens.insert(spec);
    }
    specimens.insert(best);
}

void openSVG(std::ofstream& file, const sf::Image & img) {
    file << "<?xml version='1.0' encoding='UTF-8' standalone='no'?>\n"; 
    file << "<svg width='" << img.getSize().x << "' height='" << img.getSize().y << "' xmlns = 'http://www.w3.org/2000/svg'>\n";
    file << "<rect fill='#000' width='" << img.getSize().x << "' height='" << img.getSize().y << "'/>\n";
}

void closeSVG(std::ofstream& file) {
    file << "</svg>";
    file.close();
}

void updateSVG(std::ofstream& file, const Triangle t) {
    file << "<polygon points='" << t.v[0].x << "," << t.v[0].y << " " << t.v[1].x << "," << t.v[1].y << " " << t.v[2].x << "," << t.v[2].y << "' style='fill:rgb(" << int(t.c.r) << "," << int(t.c.g) << "," << int(t.c.b) << "); opacity:0.5'/>\n";
}

void usage() {
    std::cout << "Usage:\n\tGeneticImage FILENAME" << std::endl;
}

int main(int argc, char ** argv)
{
    if (argc == 1) {
        usage();
        return 0;
    }

    srand(time(0));
    sf::Image img;
    if (argc > 1) {
        if (!img.loadFromFile(argv[1])) {
            std::cerr << "Unable to load file: " << argv[1] << std::endl;
            return 1;
        }
    }
    sf::Texture imgTxtr; 
    imgTxtr.loadFromImage(img);
    sf::Sprite imgSpr; 
    imgSpr.setTexture(imgTxtr);
    auto size = img.getSize();

    sf::Font font;
    if (!font.loadFromFile("font.otf")) {
        std::cerr << "Unable to load font file. " << std::endl;
    }

    sf::Texture genTxtr;
    genTxtr.create(size.x, size.y);
    sf::Sprite genSpr;
    genSpr.setTexture(genTxtr);
    genSpr.setPosition(float(size.x), 0);

    size_t iteration = 0;
    std::set<Specimen, SpecimenComp> specimens;
    create(specimens, img);

    sf::Clock clock;

    std::ofstream svg("output.svg");
    openSVG(svg, img);
    Triangle lastTriangle;

    sf::RenderWindow window(sf::VideoMode(size.x * 2, size.y), "Designer", sf::Style::Close | sf::Style::Titlebar);
    bool running = true;
    while (running) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                running = false;
            }
        }

        window.clear(sf::Color::White);
        window.draw(imgSpr);
        int seconds = int(clock.getElapsedTime().asSeconds()) ;
        int minutes = int(seconds / 60);
        int hours = int(minutes / 60);
        sf::Text txt("Iteration: " + std::to_string(iteration) + "\n" +
                     "Specimens: " + std::to_string(SPECIMENS) + "\n" +
                     "Elapsed time: " + std::to_string(hours) + "h " + std::to_string(minutes % 60) + "m " + std::to_string(seconds % 60) + "s\n" +
                     "Difference: " + std::to_string(specimens.begin()->score), font, 12);
        txt.setFillColor(sf::Color::White);
        txt.setOutlineColor(sf::Color::Black);
        txt.setOutlineThickness(1);
        window.draw(txt);
        window.draw(genSpr);

        genTxtr.update(specimens.begin()->buffer);

        window.display();

        Triangle newTriangle = specimens.begin()->triangles.back();
        if (!(newTriangle.v[0] == lastTriangle.v[0] && newTriangle.v[1] == lastTriangle.v[1] && newTriangle.v[2] == lastTriangle.v[2] && newTriangle.c == lastTriangle.c)) {
            updateSVG(svg, specimens.begin()->triangles.back());
            lastTriangle = newTriangle;
        }
        
        update(specimens, img);
        iteration++;
    }

    closeSVG(svg);
    return 0;
}
