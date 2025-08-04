#pragma once

#include <vector>
#include <functional>
#include <limits>
#include <cmath>

namespace parangonar {

/**
 * 2D matrix helper class
 */
template<typename T>
class Matrix2D {
public:
    std::vector<std::vector<T>> data;
    size_t rows, cols;
    
    Matrix2D(size_t rows, size_t cols, T init_val = T{}) 
        : rows(rows), cols(cols), data(rows, std::vector<T>(cols, init_val)) {}
    
    std::vector<T>& operator[](size_t row) { return data[row]; }
    const std::vector<T>& operator[](size_t row) const { return data[row]; }
    
    T& at(size_t row, size_t col) { return data[row][col]; }
    const T& at(size_t row, size_t col) const { return data[row][col]; }
};

/**
 * Path step for DTW backtracking
 */
struct PathStep {
    int row, col;
    PathStep(int r, int c) : row(r), col(c) {}
};

using DTWPath = std::vector<PathStep>;

/**
 * Distance metrics
 */
namespace metrics {
    
// Euclidean distance between two feature vectors
template<typename T>
double euclidean_distance(const std::vector<T>& a, const std::vector<T>& b) {
    if (a.size() != b.size()) return std::numeric_limits<double>::infinity();
    
    double sum = 0.0;
    for (size_t i = 0; i < a.size(); ++i) {
        double diff = static_cast<double>(a[i]) - static_cast<double>(b[i]);
        sum += diff * diff;
    }
    return std::sqrt(sum);
}

// Cosine distance
template<typename T>
double cosine_distance(const std::vector<T>& a, const std::vector<T>& b) {
    if (a.size() != b.size()) return std::numeric_limits<double>::infinity();
    
    double dot_product = 0.0, norm_a = 0.0, norm_b = 0.0;
    for (size_t i = 0; i < a.size(); ++i) {
        double va = static_cast<double>(a[i]);
        double vb = static_cast<double>(b[i]);
        dot_product += va * vb;
        norm_a += va * va;
        norm_b += vb * vb;
    }
    
    if (norm_a == 0.0 || norm_b == 0.0) return 1.0;
    return 1.0 - (dot_product / (std::sqrt(norm_a) * std::sqrt(norm_b)));
}

} // namespace metrics

/**
 * Dynamic Time Warping implementation
 */
class DynamicTimeWarping {
public:
    using DistanceFunction = std::function<double(const std::vector<float>&, const std::vector<float>&)>;
    
private:
    DistanceFunction distance_fn;
    
public:
    explicit DynamicTimeWarping(DistanceFunction dist_fn = metrics::euclidean_distance<float>)
        : distance_fn(std::move(dist_fn)) {}
    
    // Main DTW computation
    struct DTWResult {
        double distance = 0.0;
        DTWPath path;
        Matrix2D<double> cost_matrix{0, 0};
        
        DTWResult() = default;
        DTWResult(double d, DTWPath p) : distance(d), path(std::move(p)), cost_matrix(0, 0) {}
        DTWResult(double d, DTWPath p, Matrix2D<double> m) : distance(d), path(std::move(p)), cost_matrix(std::move(m)) {}
    };
    
    DTWResult compute(const std::vector<std::vector<float>>& X, 
                     const std::vector<std::vector<float>>& Y,
                     bool return_path = true,
                     bool return_cost_matrix = false) const;
    
private:
    Matrix2D<double> compute_pairwise_distances(const std::vector<std::vector<float>>& X,
                                               const std::vector<std::vector<float>>& Y) const;
    
    Matrix2D<double> compute_cost_matrix(const Matrix2D<double>& distance_matrix) const;
    
    DTWPath backtrack_path(const Matrix2D<double>& cost_matrix) const;
};

/**
 * Weighted Dynamic Time Warping with custom step patterns
 */
class WeightedDynamicTimeWarping {
public:
    struct Direction {
        int row_step, col_step;
        Direction(int r, int c) : row_step(r), col_step(c) {}
    };
    
private:
    std::vector<double> directional_weights;
    std::vector<Direction> directions;
    DynamicTimeWarping::DistanceFunction distance_fn;
    
public:
    WeightedDynamicTimeWarping(
        const std::vector<double>& weights = {1.0, 1.0, 1.0},
        const std::vector<Direction>& dirs = {{1, 0}, {1, 1}, {0, 1}},
        DynamicTimeWarping::DistanceFunction dist_fn = metrics::euclidean_distance<float>)
        : directional_weights(weights), directions(dirs), distance_fn(std::move(dist_fn)) {}
    
    DynamicTimeWarping::DTWResult compute(const std::vector<std::vector<float>>& X,
                                         const std::vector<std::vector<float>>& Y,
                                         bool return_matrices = false,
                                         bool return_cost = false) const;
    
private:
    std::pair<Matrix2D<double>, DTWPath> forward_and_backward(const Matrix2D<double>& distance_matrix) const;
};

} // namespace parangonar