#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include<string>
#include<unordered_set>

#include"color.cpp"

class GraphicsDrive {
public:
    GraphicsDrive() {
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    }
    ~GraphicsDrive() {
        glfwTerminate();
    }
};

class Window {
public:
    Window(int width, int height, const std::string& title_) : title(title_) {
        gl_win = glfwCreateWindow(width, height, title.data(), NULL, NULL);
        bind();
        gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

        glfwSetFramebufferSizeCallback(gl_win, global_framebuffer_size_callback);
        glfwSetKeyCallback(gl_win, global_key_callback);
        glfwSetCursorPosCallback(gl_win, global_cursor_callback);
        glfwSetMouseButtonCallback(gl_win, global_button_callback);
        glfwSetScrollCallback(gl_win, global_scroll_callback);
    }

    ~Window() {
        if(current == this)
            current = nullptr;
        glfwSetWindowShouldClose(gl_win, 1);
    }

    void bind() {
        glfwMakeContextCurrent(gl_win);
        current = this;
    }
    static void unbind() {
        glfwMakeContextCurrent(nullptr);
        current = nullptr;
    }

    void set_background(const Color& col) {
        glClearColor(col.red, col.green, col.blue, col.alpha);
    }

    bool is_open() const {
        return !glfwWindowShouldClose(gl_win);
    }

    void clear() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void display() {
        glfwSwapBuffers(gl_win);
    }

    void poll_events() {
        glfwPollEvents();
    }

    bool key_held(int key) const {
        return held_keys.contains(key);
    }
    using Cursor = Vector<double, 2>;
    Cursor cursor_pos() const {
        return cursor;
    }
    bool button_held(int button) const {
        return held_buttons.contains(button);
    }
    double get_scroll() const {
        return scroll;
    }
private:
    virtual void framebuffer_size_callback(int width, int height) {
        glViewport(0, 0, width, height);
    }
    static void global_framebuffer_size_callback(GLFWwindow* window, int width, int height) {
        current->framebuffer_size_callback(width, height);
    }

    void key_callback(int key, int scancode, int action, int mods) {
        if (action == GLFW_PRESS || action == GLFW_REPEAT)
            held_keys.emplace(key);
        else if (action == GLFW_RELEASE)
            held_keys.erase(key);
    }
    static void global_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        current->key_callback(key, scancode, action, mods);
    }

    void cursor_callback(const Cursor& new_pos) {
        cursor = new_pos;
    }
    static void global_cursor_callback(GLFWwindow* window, double x, double y) {
        current->cursor_callback(Cursor{x, y});
    }

    void button_callback(int button, int action, int mods) {
        if (action == GLFW_PRESS || action == GLFW_REPEAT)
            held_buttons.emplace(button);
        else if (action == GLFW_RELEASE)
            held_buttons.erase(button);
    }
    static void global_button_callback(GLFWwindow* window, int button, int action, int mods) {
        current->button_callback(button, action, mods);
    }

    void scroll_callback(double offset) {
        scroll += offset;
    }
    static void global_scroll_callback(GLFWwindow* window, double x, double y) {
        current->scroll_callback(y);
    }

    std::string title;
    GLFWwindow* gl_win;
    std::unordered_set<int> held_keys;
    std::unordered_set<int> held_buttons;
    Cursor cursor{0, 0};
    double scroll = 0;

    static thread_local Window* current;
};

thread_local Window* Window::current = nullptr;