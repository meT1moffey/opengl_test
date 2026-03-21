#include<algorithm>
#include<array>
#include<cmath>
#include<vector>

template<typename T>
concept Linear = requires(T& a, const T& b) {
    {a += b} -> std::same_as<T&>;
    {a -= b} -> std::same_as<T&>;
    {a *= b} -> std::same_as<T&>;
    {a /= b} -> std::same_as<T&>;
};

template<Linear T, size_t size>
class Vector {
public:
    Vector() {}
    Vector(const std::array<T, size>& vals_) : vals(vals_) {}
    Vector(std::array<T, size>&& vals_) : vals(std::move(vals_)) {}
    Vector(const std::initializer_list<T>& vals_) {
        size_t i = 0;
        for(const T& val : vals_) {
            vals[i++] = val;
        }
    }
    std::array<T, size> vals;

    T& operator[](size_t i) {
        return vals[i];
    }
    const T& operator[](size_t i) const {
        return vals[i];
    }

    Vector& operator+=(const Vector& other) {
        for(size_t i = 0; i < size; i++) {
            vals[i] += other[i];
        }
        return *this;
    }
    Vector operator+(const Vector& other) const {
        return Vector(*this) += other;
    }
    Vector& operator-=(const Vector& other) {
        for(size_t i = 0; i < size; i++) {
            vals[i] -= other[i];
        }
        return *this;
    }
    Vector operator-(const Vector& other) const {
        return Vector(*this) -= other;
    }

    Vector& operator*=(const T& val) {
        for(size_t i = 0; i < size; i++) {
            vals[i] *= val;
        }
        return *this;   
    }
    Vector operator*(const T& val) const {
        return Vector(*this) *= val;
    }
    Vector& operator/=(const T& val) {
        for(size_t i = 0; i < size; i++) {
            vals[i] /= val;
        }
        return *this;   
    }
    Vector operator/(const T& val) const {
        return Vector(*this) /= val;
    }

    T len() const {
        return sqrt(dot(*this, *this));
    }

    Vector norm() const {
        return *this / len();
    }

    bool operator==(const Vector& other) const {
        return vals == other.vals;
    }
    bool operator!=(const Vector& other) const {
        return vals != other.vals;
    }
};

template<Linear T, size_t size>
Vector<T, size> operator*(const T& val, const Vector<T, size>& vec) {
    Vector<T, size> prod;
    for(int i = 0; i < size; i++) {
        prod[i] = val * vec.vals[i];
    }
    return prod;
}

template<Linear T, size_t size>
T dot(const Vector<T, size>& lhs, const Vector<T, size>& rhs) {
    T prod = lhs.vals[0];
    prod *= rhs.vals[0];
    for(size_t i = 1; i < size; i++) {
        T elem = lhs.vals[i];
        elem *= rhs.vals[i];
        prod += elem;
    }
    return prod;
}

template<Linear T, size_t size>
T distance(const Vector<T, size>& from, const Vector<T, size>& to) {
    return (from - to).len();
}

template<Linear T, size_t size>
T projection(const Vector<T, size>& from, const Vector<T, size>& to) {
    return dot(from, to) / to.len();
}

template<Linear T, size_t size>
struct Matrix {
    Matrix() {
        vals = new std::array<std::array<T, size>, size>;
    }
    Matrix(const std::array<std::array<T, size>, size>& vals_) : Matrix() {
        std::copy(vals_.begin(), vals_.end(), vals->begin());
    }
    Matrix(std::array<std::array<T, size>, size>&& vals_) : Matrix() {
        std::move(vals_.begin(), vals_.end(), vals->begin());
    }
    Matrix(const std::array<Vector<T, size>, size> vecs) : Matrix() {
        for(size_t i = 0; i < size; i++) {
            for(size_t j = 0; j < size; j++) {
                (*vals)[i][j] = vecs[i][j];
            }
        }
    }
    Matrix(const Matrix& other) : Matrix() {
        std::copy(other.vals->begin(), other.vals->end(), vals->begin());
    }
    Matrix(Matrix&& other) {
        vals = other.vals;
        other.vals = nullptr;
    }

    ~Matrix() {
        delete vals;
    }

    Matrix& operator=(const Matrix& other) {
        std::copy(other.vals->begin(), other.vals->end(), vals->begin());
        return *this;
    }
    Matrix& operator=(Matrix&& other) {
        delete vals;
        vals = other.vals;
        other.vals = nullptr;
        return *this;
    }

    std::array<std::array<T, size>, size>* vals;

    T& operator[](size_t i, size_t j) {
        return (*vals)[i][j];
    }
    const T& operator[](size_t i, size_t j) const {
        return (*vals)[i][j];
    }

    Vector<T, size> row(size_t i) const {
        return Vector((*vals)[i]);
    }
    Vector<T, size> column(size_t j) const {
        Vector<T, size> col;
        for(size_t i = 0; i < size; i++) {
            col[i] = (*vals)[i][j];
        }
        return col;
    }

    Matrix& operator+=(const Matrix& other) {
        for(size_t i = 0; i < size; i++) {
            for(size_t j = 0; j < size; j++) {
                (*vals)[i][j] += other[i, j];
            }
        }
        return *this;
    }
    Matrix operator+(const Matrix& other) const {
        return Matrix(*this) += other;
    }

    Matrix& operator-=(const Matrix& other) {
        for(size_t i = 0; i < size; i++) {
            for(size_t j = 0; j < size; j++) {
                (*vals)[i][j] -= other[i, j];
            }
        }
        return *this;
    }
    Matrix operator-(const Matrix& other) const {
        return Matrix(*this) -= other;
    }

    Matrix operator*(const Matrix& other) const {
        Matrix prod;
        for(size_t i = 0; i < size; i++) {
            for(size_t j = 0; j < size; j++) {
                prod[i, j] = dot(row(i), other.column(j));
            }
        }
        return prod;
    }
    Matrix& operator*=(const Matrix& other) {
        return *this = *this * other;
    }

    Matrix& operator*=(const T& val) {
        for(size_t i = 0; i < size; i++) {
            for(size_t j = 0; j < size; j++) {
                (*vals)[i][j] *= val;
            }
        }
    }
    Matrix operator*(const T& val) const {
        return Matrix(*this) *= val;
    }

    T trace() const {
        T res = (*vals)[0][0];
        for(size_t i = 1; i < size; i++) {
            res *= (*vals)[i][i];
        }
        return res;
    }

    T det() const {
        Matrix copy = *this;
        for(size_t t = 0; t < size; t++) {
            if(copy[t, t] == 0) {
                size_t t_ = size;
                for(size_t s = t + 1; s < size; s++) {
                    if(copy[s, t] != 0) {
                        t_ = s;
                        break;
                    }
                }
                if(t_ == size) {
                    return T(0);
                }
                std::swap((*copy.vals)[t], (*copy.vals)[t_]);
            }

            T mult = copy[t, t];
            for(size_t j = t; j < size; j++) {
                copy[t, j] /= mult;
            }

            for(size_t i = t + 1; i < size; i++) {
                T mult = copy[i, t];
                for(size_t j = t; j < size; j++) {
                    copy[i, j] -= mult * copy[t, j];
                }
            }
        }
        return copy.trace();
    }

    Matrix inverse() const {
        // assuming det != 0
        Matrix inv = unit();
        Matrix copy = *this;
        for(size_t t = 0; t < size; t++) {
            if(copy[t, t] == 0) {
                size_t t_ = size;
                for(size_t s = t + 1; s < size; s++) {
                    if(copy[s, t] != 0) {
                        t_ = s;
                        break;
                    }
                }
                std::swap((*copy.vals)[t], (*copy.vals)[t_]);
                std::swap((*inv.vals)[t], (*inv.vals)[t_]);
            }

            T mult = copy[t, t];
            for(size_t j = t; j < size; j++) {
                copy[t, j] /= mult;
            }
            for(size_t j = 0; j < size; j++) {
                inv[t, j] /= mult;
            }

            for(size_t i = 0; i < size; i++) {
                if(i == t) {
                    continue;
                }
                T mult = copy[i, t];
                for(size_t j = t; j < size; j++) {
                    copy[i, j] -= mult * copy[t, j];
                }
                for(size_t j = 0; j < size; j++) {
                    inv[i, j] -= mult * inv[t, j];
                }
            }
        }
        return inv;
    }

    Vector<T, size> operator*(const Vector<T, size>& vec) const {
        Vector<T, size> prod;
        for(size_t i = 0; i < size; i++) {
            prod[i] = dot(row(i), vec);
        }
        return prod;
    }

    bool operator==(const Matrix& other) const {
        return std::equal(vals->front().begin(), vals->back().end(), other.vals->front().begin());
    }
    bool operator!=(const Matrix& other) const {
        return !(*this == other);
    }

    static Matrix unit() {
        Matrix mtx;
        for(size_t i = 0; i < size; i++) {
            for(size_t j = 0; j < size; j++) {
                mtx[i, j] = i == j ? T(1) : T(0);
            }
        }
        return mtx;
    }
};