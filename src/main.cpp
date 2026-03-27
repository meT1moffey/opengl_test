#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include<chrono>
#include<functional>

#include"graphics.cpp"
#include"window.cpp"

constexpr unsigned sphere_detail = 1;

double integrate(const std::function<double(double)>& func, double start, double end, int steps=10) {
    double result = 0;
    if(end == start) {
        return 0;
    }
    double step = (end - start) / steps;
    for(double x = start; x < end; x += step) {
        result += func(x) * step;
    }
    return result;
}

float area(const Triangle& tr) {
    float a = (tr.getVert(0) - tr.getVert(1)).len();
    float b = (tr.getVert(0) - tr.getVert(2)).len();
    float c = (tr.getVert(1) - tr.getVert(2)).len();
    float p = (a + b + c) / 2;
    return sqrtf(p * (p - a) * (p - b) * (p - c));
}

int main() {
    double potential_dif;
    std::cout << "Разница потенциалов (В):";
    std::cin >> potential_dif;
    float distance;
    std::cout << "Расстояние между сферами (м):"; 
    std::cin >> distance;
    float radius;
    std::cout << "Радиус сфер (м):";
    std::cin >> radius;
    std::cout << "Запуск вычислений...\n";

    GraphicsDrive drive;
    std::string title = "М1. Основная задача электростатики.";
    Window win(900, 900, title);
    win.set_background({0.1, 0.1, 0.1});

    Program program;
    {
        Shader vert_shader(GL_VERTEX_SHADER);
        vert_shader.load("../shaders/projective.vert");

        Shader frag_shader(GL_FRAGMENT_SHADER);
        frag_shader.load("../shaders/solid.frag");

        program.build({&vert_shader, &frag_shader});
    }

    program.bind();

    Camera cam(program);

    Vector3 center1{-distance / 2, 0, 0};
    IcoSphere ico1(program, {0.5, 0.5, 0.5}, center1, radius, sphere_detail);
    
    Vector3 center2{+distance / 2, 0, 0};
    IcoSphere ico2(program, {0.5, 0.5, 0.5}, center2, radius, sphere_detail);
    
    const double electric_const = 8.988e9;

    constexpr size_t eq_size = 40 << (2 * sphere_detail);
    Matrix<double, eq_size + 1> eq;
    for(size_t i = 0; i < eq_size; i++) {
        Triangle* tr1 = i < eq_size / 2 ? &ico1.faces[i] : &ico2.faces[i - eq_size / 2];
        Vector3 p1 = (tr1->getVert(0) + tr1->getVert(1) + tr1->getVert(2)) / 3;
        Vector3 norm = p1 - (i < eq_size / 2 ? center1 : center2);
        for(size_t j = 0; j < eq_size; j++) {
            if(i == j) {
                eq[i, i] = 2 * M_PI * electric_const * sqrtf(area(*tr1));
            }
            else {
                Triangle* tr2 = j < eq_size / 2 ? &ico1.faces[j] : &ico2.faces[j - eq_size / 2];
                Vector3 p2 = (tr2->getVert(0) + tr2->getVert(1) + tr2->getVert(2)) / 3;

                eq[i, j] = electric_const * area(*tr2) / (p1 - p2).len();
            }
        }
        eq[i, eq_size] = -1;
        eq[eq_size, i] = area(*tr1);
    }
    eq[eq_size, eq_size] = 0;
    Vector<double, eq_size + 1> consts_col;
    for(size_t i = 0; i < eq_size; i++) {
        consts_col[i] = i < eq_size / 2 ? 0 : -potential_dif;
    }
    consts_col[eq_size] = 0;

    Vector<double, eq_size + 1> charges;
    float max_charge;
    if(eq.det() == 0) {
        for(size_t i = 0; i < eq_size; i++) {
            charges[i] = 0;
        }
        max_charge = 1;
    }
    else {
        charges = eq.inverse() * consts_col;
        max_charge = std::max(
             *std::max_element(charges.vals.begin(), charges.vals.end() - 1),
            -*std::min_element(charges.vals.begin(), charges.vals.end() - 1)
        );
    }

    {
        double charge = 0;
        for(size_t i = 0; i < eq_size / 2; i++) {
            charge += charges[i] * area(ico1.faces[i]);
        }
        std::cout << "Электическая ёмкость системы: " << charge / potential_dif << " Ф\n";
    }

    std::vector<Line> lines;
    std::srand(std::time({}));
    for(int _ = 0; _ < 30; _++) {
        Vector3 start{
            2 * ((float)std::rand() / RAND_MAX) - 1,
            2 * ((float)std::rand() / RAND_MAX) - 1,
            2 * ((float)std::rand() / RAND_MAX) - 1,
        };
        std::vector<Vector3> points;
        auto voltage = [&](const Vector3& point) {
            Vector3 volt = {0, 0, 0};
            for(int i = 0; i < eq_size; i++) {
                Triangle* face = i < eq_size / 2 ? &ico1.faces[i] : &ico2.faces[i - eq_size / 2];
                Vector3 center = (face->getVert(0) + face->getVert(1) + face->getVert(2)) / 3;
                float value = electric_const * charges[i] * area(*face) / powf((point - center).len(), 2);
                Vector3 direction = point - center;
                volt += value * direction.norm();
            }
            return volt;
        };
        auto voltage_diff = [&](const Vector3& point) {
            Vector3 volt = {0, 0, 0};
            for(int i = 0; i < eq_size; i++) {
                Triangle* face = i < eq_size / 2 ? &ico1.faces[i] : &ico2.faces[i - eq_size / 2];
                Vector3 center = (face->getVert(0) + face->getVert(1) + face->getVert(2)) / 3;
                float value = -2 * electric_const * charges[i] * area(*face) / powf((point - center).len(), 3);
                Vector3 direction = point - center;
                volt += value * direction.norm();
            }
            return volt;
        };
        float eps = 0.05;

        Vector3 cur = start;
        while(points.size() < 250) {
            points.push_back(cur);
            cur -= eps * voltage(cur) / voltage_diff(cur).len();
        }

        std::reverse(points.begin(), points.end());
        points.pop_back();

        cur = start;
        while(points.size() < 500) {
            points.push_back(cur);
            cur += eps * voltage(cur) / voltage_diff(cur).len();
        }

        lines.emplace_back(program, Color(1, 1, 0), points);
    }

    {
        size_t i = 0;
        for(Triangle& face : ico1.faces) {
            Color col;
            if(charges[i] > 0) {
                float diff = charges[i] / max_charge / 2;
                col = Color(0.5 + diff, 0.5 - diff, 0.5 - diff);
            }
            else {
                float diff = -charges[i] / max_charge / 2;
                col = Color(0.5 - diff, 0.5 - diff, 0.5 + diff);
            }
            face.setColor(col);
            i++;
        }
        for(Triangle& face : ico2.faces) {
            Color col;
            if(charges[i] > 0) {
                float diff = charges[i] / max_charge / 2;
                col = Color(0.5 + diff, 0.5 - diff, 0.5 - diff);
            }
            else {
                float diff = -charges[i] / max_charge / 2;
                col = Color(0.5 - diff, 0.5 - diff, 0.5 + diff);
            }
            face.setColor(col);
            i++;
        }
    }

    float rot_speed = 2;
    std::chrono::time_point<std::chrono::system_clock> prev_time = std::chrono::system_clock::now();
    while(win.is_open()) {
        win.poll_events();

        std::chrono::time_point<std::chrono::system_clock> cur_time = std::chrono::system_clock::now();
        double delta_time = std::chrono::duration_cast<std::chrono::microseconds>(cur_time - prev_time).count() / 1e6;
        prev_time = cur_time;

        if(win.key_held(GLFW_KEY_LEFT))
            cam.rotate({{0, 1, 0}}, -rot_speed * delta_time);
        if(win.key_held(GLFW_KEY_RIGHT))
            cam.rotate({{0, 1, 0}}, +rot_speed * delta_time);
        if(win.key_held(GLFW_KEY_UP))
            cam.rotate({{1, 0, 0}}, +rot_speed * delta_time);
        if(win.key_held(GLFW_KEY_DOWN))
            cam.rotate({{1, 0, 0}}, -rot_speed * delta_time);
        
        if(win.key_held(GLFW_KEY_MINUS)) {
            UniformVector<1> zoom(program, "zoom");
            zoom.set(zoom.get() / 1.1);
        }
        if(win.key_held(GLFW_KEY_EQUAL)) {
            UniformVector<1> zoom(program, "zoom");
            zoom.set(zoom.get() * 1.1);
        }

        win.clear();
        ico1.draw();
        ico2.draw();
        
        for(const Line& line : lines)
            line.draw();

        win.display();
    }
    return 0;
}