#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include<iostream>
#include<fstream>
#include<sstream>
#include<string>

#include"color.cpp"
#include"linear.cpp"

class Shader {
public:
    Shader(GLenum type_) : id(glCreateShader(type_)) {}
    Shader(const Shader&) = delete;
    Shader(Shader&& other) : id(other.id) {
        other.free();
    }

    ~Shader() {
        free();
    }

    Shader& operator=(const Shader&) = delete;
    Shader& operator=(Shader&& other) {
        glDeleteShader(id);
        id = other.id;
        other.free();
        return *this;
    }

    void set_source(const std::string& source) {
        const char* c_source = source.c_str();
        glShaderSource(id, 1, &c_source, NULL);
        glCompileShader(id);
    }

    void load(const std::string& path) {
        std::ifstream file(path);
        std::stringstream buffer;
        buffer << file.rdbuf();
        set_source(buffer.str());
    }

    void free() {
        glDeleteShader(id);
        id = GL_ZERO;
    }

    friend class Program;
private:
    GLint id;
};

class Program {
public:
    Program() : id(glCreateProgram()) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_DEPTH_TEST);
    }

    ~Program() {
        glDeleteProgram(id);
    }

    void build(const std::initializer_list<Shader*>& shaders) {
        for(Shader* shader : shaders)
            glAttachShader(id, shader->id);
        glLinkProgram(id);
    }

    void bind() {
        glUseProgram(id);
    }
    static void unbind() {
        glUseProgram(GL_ZERO);
    }

    friend class Uniform;
private:
    GLint id;
};

class Uniform {
public:
    Uniform(const Program& prog, const std::string& name) : program(prog.id) {
        location = glGetUniformLocation(program, name.c_str());
    }
protected:
    GLint program;
    GLint location;
};

using Vector3 = Vector<float, 3>;

template<size_t size>
class UniformVector : public Uniform {
public:
    UniformVector(const Program& prog, const std::string& name) : Uniform(prog, name) {}

    Vector<float, size> get() const {
        std::array<float, size> buffer;
        glGetnUniformfv(program, location, sizeof(buffer), buffer.data());
        return Vector<float, size>(buffer);
    }
    void set(const Vector<float, size>& value) const {
        switch (size) {
        case 1: glUniform1fv(location, 1, value.vals.data()); break;
        case 2: glUniform2fv(location, 1, value.vals.data()); break;
        case 3: glUniform3fv(location, 1, value.vals.data()); break;
        case 4: glUniform4fv(location, 1, value.vals.data()); break;
        }
    }
};

using Uniform3f = UniformVector<3>;
using Uniform4f = UniformVector<4>;

using Matrix3 = Matrix<float, 3>;

class UniformMatrix3f : public Uniform {
public:
    UniformMatrix3f(const Program& prog, const std::string& name) : Uniform(prog, name) {}

    Matrix3 get() const {
        std::array<std::array<float, 3>, 3> buffer;
        glGetUniformfv(program, location, buffer[0].data());
        return Matrix3(buffer);
    }
    void set(const Matrix3& value) const {
        glUniformMatrix3fv(location, 1, false, (float*)value.vals);
    }
};

template<Linear T, size_t size>
void normalise_base(Matrix<T, size>& base) {
    for(size_t i = 0; i < size; i++) {
        Vector3 vec = base.row(i);
        for(size_t j = 0; j < i; j++) {
            vec -= base.row(j) * dot(base.row(i), base.row(j)) / dot(base.row(j), base.row(j));
            
        }
        vec /= vec.len();
        for(size_t j = 0; j < size; j++) {
            base[i, j] = vec[j];
        }
    }
}

class Camera {
public:
    Camera(const Program& program) :
        pos(program, "camera_pos"),
        rot(program, "camera_rot")
    {
        pos.set({0, 0, 0});
        rot.set(Matrix3::unit());
    }

    void move(Vector3 delta) {
        pos.set(pos.get() + delta);
    }
    void rotate(Vector3 axis, float angle) {
        axis = rot.get() * axis;
        Matrix3 rev_transform;
        if(axis[0] != 0) {
            rev_transform = {{
                axis,
                {{0, 1, 0}},
                {{0, 0, 1}}
            }};
        }
        else if(axis[1] != 0) {
            rev_transform = {{
                axis,
                {{1, 0, 0}},
                {{0, 0, 1}}
            }};
        }
        else if(axis[2] != 0) {
            rev_transform = {{
                axis,
                {{1, 0, 0}},
                {{0, 1, 0}}
            }};
        }
        else {
            return;
        }
        normalise_base(rev_transform);

        Matrix3 new_rot = {{
            Vector3{{1, 0          ,  0          }},
            Vector3{{0, cosf(angle), -sinf(angle)}},
            Vector3{{0, sinf(angle),  cosf(angle)}}
        }};
        Matrix3 transform = rev_transform.inverse();

        rot.set(rev_transform * new_rot * transform * rot.get());
    }
private:
    Uniform3f pos;
    UniformMatrix3f rot;
};

template<GLenum mode>
class Shape {
public:
    Shape(const Program& program, const std::vector<Vector3>& points) :
        verts_count(points.size()),
        color_uniform(program, "fill_color")
    {
        glGenBuffers(1, &verts_buf);
        glBindBuffer(GL_ARRAY_BUFFER, verts_buf);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3) * verts_count, points.data(), GL_STATIC_DRAW);

        init_vao();
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    Shape(const Shape&) = delete;
    Shape(Shape&& other) :
        verts_count(other.verts_count),
        verts_buf(other.verts_buf),
        verts_attr(other.verts_attr),
        color(other.color),
        color_uniform(other.color_uniform)
    {
        other.verts_buf = 0;
        other.verts_attr = 0;
    }

    ~Shape() {
        glDeleteVertexArrays(1, &verts_attr);
        glDeleteBuffers(1, &verts_buf);
    }

    Shape& operator=(const Shape&) = delete;
    Shape& operator=(Shape&& other) {
        glDeleteVertexArrays(1, &verts_attr);
        glDeleteBuffers(1, &verts_buf);

        verts_count = other.verts_count;
        verts_buf = other.verts_buf;
        verts_attr = other.verts_attr;
        color = other.color;
        color_uniform = other.color_uniform;

        other.verts_buf = 0;
        other.verts_attr = 0;

        return *this;
    }

    Vector3 getVert(size_t id) const {
        Vector3 vert;
        glBindBuffer(GL_ARRAY_BUFFER, verts_buf);
        glGetBufferSubData(GL_ARRAY_BUFFER, id * sizeof(Vector3), sizeof(Vector3), &vert);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        return vert;
    }

    void setColor(const Color& color_) {
        color = color_;
    }

    void draw() const {
        glBindVertexArray(verts_attr);
        color_uniform.set({color.red, color.green, color.blue, color.alpha});
        glDrawArrays(mode, 0, verts_count);
        glBindVertexArray(0);
    }
private:
    void init_vao() {
        glGenVertexArrays(1, &verts_attr);
        glBindVertexArray(verts_attr);
        glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vector3), 0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }
    unsigned int verts_buf;
    unsigned int verts_attr;
    size_t verts_count;
    Color color = Color(1, 1, 1);
    Uniform4f color_uniform;
};

class Triangle : public Shape<GL_TRIANGLES> {
public:
    Triangle(const Program& program, Vector3 v1, Vector3 v2, Vector3 v3) : Shape(program, {v1, v2, v3}) {}
};

class Line : public Shape<GL_LINE_STRIP> {
public:
    Line(const Program& program, const std::vector<Vector3>& points) : Shape(program, points) {}
};

class IcoSphere {
public:
    IcoSphere(const Program& program, Vector3 center, float rad, unsigned int detailing = 0) {
        float phi = (1 + sqrtf(5)) / 2;
        float scale = rad / sqrtf(1 + phi * phi);

        Vector3 v[12] = {
            {{0, +1, +phi}},
            {{0, +1, -phi}},
            {{0, -1, +phi}},
            {{0, -1, -phi}},

            {{+1, +phi, 0}},
            {{+1, -phi, 0}},
            {{-1, +phi, 0}},
            {{-1, -phi, 0}},

            {{+phi, 0, +1}},
            {{+phi, 0, -1}},
            {{-phi, 0, +1}},
            {{-phi, 0, -1}},
        };
        for(Vector3& vec : v) {
            vec = vec * scale + center;
        }

        faces.emplace_back(program, v[0], v[2], v[8]);
        faces.emplace_back(program, v[0], v[8], v[4]);
        faces.emplace_back(program, v[0], v[4], v[6]);
        faces.emplace_back(program, v[0], v[6], v[10]);
        faces.emplace_back(program, v[0], v[10], v[2]);

        faces.emplace_back(program, v[3], v[11], v[1]);
        faces.emplace_back(program, v[3], v[1], v[9]);
        faces.emplace_back(program, v[3], v[9], v[5]);
        faces.emplace_back(program, v[3], v[5], v[7]);
        faces.emplace_back(program, v[3], v[7], v[11]);

        faces.emplace_back(program, v[1],  v[4],  v[9]);
        faces.emplace_back(program, v[6],  v[1],  v[4]);
        faces.emplace_back(program, v[11], v[6],  v[1]);
        faces.emplace_back(program, v[10], v[11], v[6]);
        faces.emplace_back(program, v[7],  v[10], v[11]);
        faces.emplace_back(program, v[2],  v[7],  v[10]);
        faces.emplace_back(program, v[5],  v[2],  v[7]);
        faces.emplace_back(program, v[8],  v[5],  v[2]);
        faces.emplace_back(program, v[9],  v[8],  v[5]);
        faces.emplace_back(program, v[4],  v[9],  v[8]);

        for(int i = 0; i < detailing; i++) {
            std::vector<Triangle> more_faces;
            for(const Triangle& face : faces) {
                Vector3 a = face.getVert(0);
                Vector3 b = face.getVert(1);
                Vector3 c = face.getVert(2);

                Vector3 ab = (a + b) / 2 - center;
                Vector3 ac = (a + c) / 2 - center;
                Vector3 bc = (b + c) / 2 - center;

                ab *= rad / ab.len();
                ac *= rad / ac.len();
                bc *= rad / bc.len();

                ab += center;
                ac += center;
                bc += center;

                more_faces.emplace_back(program, a, ab, ac);
                more_faces.emplace_back(program, ab, b, bc);
                more_faces.emplace_back(program, ac, bc, c);
                more_faces.emplace_back(program, ab, ac, bc);
            }
            faces = std::move(more_faces);
        }
    }

    void draw() const {
        for(const Triangle& face : faces) {
            face.draw();
        }
    }

    std::vector<Triangle> faces;
};