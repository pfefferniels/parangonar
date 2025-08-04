#include <parangonar/dtw.hpp>
#include <algorithm>
#include <limits>

namespace parangonar {

DynamicTimeWarping::DTWResult DynamicTimeWarping::compute(
    const std::vector<std::vector<float>>& X,
    const std::vector<std::vector<float>>& Y,
    bool return_path,
    bool return_cost_matrix) const {
    
    // Compute pairwise distances
    auto distance_matrix = compute_pairwise_distances(X, Y);
    
    // Compute cost matrix
    auto cost_matrix = compute_cost_matrix(distance_matrix);
    
    DTWResult result;
    result.distance = cost_matrix[cost_matrix.rows - 1][cost_matrix.cols - 1];
    
    if (return_path) {
        result.path = backtrack_path(cost_matrix);
    }
    
    if (return_cost_matrix) {
        result.cost_matrix = std::move(cost_matrix);
    }
    
    return result;
}

Matrix2D<double> DynamicTimeWarping::compute_pairwise_distances(
    const std::vector<std::vector<float>>& X,
    const std::vector<std::vector<float>>& Y) const {
    
    Matrix2D<double> distances(X.size(), Y.size());
    
    for (size_t i = 0; i < X.size(); ++i) {
        for (size_t j = 0; j < Y.size(); ++j) {
            distances[i][j] = distance_fn(X[i], Y[j]);
        }
    }
    
    return distances;
}

Matrix2D<double> DynamicTimeWarping::compute_cost_matrix(const Matrix2D<double>& distance_matrix) const {
    const size_t M = distance_matrix.rows;
    const size_t N = distance_matrix.cols;
    
    Matrix2D<double> cost_matrix(M + 1, N + 1, std::numeric_limits<double>::infinity());
    
    // Initialize
    cost_matrix[0][0] = 0.0;
    
    // Fill cost matrix
    for (size_t i = 1; i <= M; ++i) {
        for (size_t j = 1; j <= N; ++j) {
            double cost = distance_matrix[i-1][j-1];
            
            double insertion = cost_matrix[i-1][j];
            double deletion = cost_matrix[i][j-1];
            double match = cost_matrix[i-1][j-1];
            
            cost_matrix[i][j] = cost + std::min({insertion, deletion, match});
        }
    }
    
    // Remove padding
    Matrix2D<double> result(M, N);
    for (size_t i = 0; i < M; ++i) {
        for (size_t j = 0; j < N; ++j) {
            result[i][j] = cost_matrix[i+1][j+1];
        }
    }
    
    return result;
}

DTWPath DynamicTimeWarping::backtrack_path(const Matrix2D<double>& cost_matrix) const {
    DTWPath path;
    
    int i = static_cast<int>(cost_matrix.rows) - 1;
    int j = static_cast<int>(cost_matrix.cols) - 1;
    
    path.emplace_back(i, j);
    
    while (i > 0 || j > 0) {
        if (i == 0) {
            j -= 1;
        } else if (j == 0) {
            i -= 1;
        } else {
            // Find minimum of three predecessors
            double match = cost_matrix[i-1][j-1];
            double insertion = cost_matrix[i-1][j];
            double deletion = cost_matrix[i][j-1];
            
            if (match <= insertion && match <= deletion) {
                i -= 1;
                j -= 1;
            } else if (insertion <= deletion) {
                i -= 1;
            } else {
                j -= 1;
            }
        }
        
        path.emplace_back(i, j);
    }
    
    // Reverse path to get forward direction
    std::reverse(path.begin(), path.end());
    
    return path;
}

// Weighted DTW implementation
DynamicTimeWarping::DTWResult WeightedDynamicTimeWarping::compute(
    const std::vector<std::vector<float>>& X,
    const std::vector<std::vector<float>>& Y,
    bool return_matrices,
    bool return_cost) const {
    
    const size_t M = X.size();
    const size_t N = Y.size();
    
    // Compute pairwise distances
    Matrix2D<double> distance_matrix(M, N);
    for (size_t i = 0; i < M; ++i) {
        for (size_t j = 0; j < N; ++j) {
            distance_matrix[i][j] = distance_fn(X[i], Y[j]);
        }
    }
    
    auto [cost_matrix, path] = forward_and_backward(distance_matrix);
    
    DynamicTimeWarping::DTWResult result;
    result.path = std::move(path);
    result.distance = cost_matrix[M-1][N-1];
    
    if (return_matrices) {
        result.cost_matrix = std::move(cost_matrix);
    }
    
    return result;
}

std::pair<Matrix2D<double>, DTWPath> WeightedDynamicTimeWarping::forward_and_backward(
    const Matrix2D<double>& distance_matrix) const {
    
    const size_t M = distance_matrix.rows;
    const size_t N = distance_matrix.cols;
    
    Matrix2D<double> cost_matrix(M + 1, N + 1, std::numeric_limits<double>::infinity());
    Matrix2D<int> backtrack(M, N, -1);
    
    // Initialize
    cost_matrix[0][0] = 0.0;
    
    // Forward pass
    for (size_t i = 1; i <= M; ++i) {
        for (size_t j = 1; j <= N; ++j) {
            double min_cost = std::numeric_limits<double>::infinity();
            int best_direction = -1;
            
            // Try all directions
            for (size_t d = 0; d < directions.size(); ++d) {
                int prev_i = static_cast<int>(i) - directions[d].row_step;
                int prev_j = static_cast<int>(j) - directions[d].col_step;
                
                if (prev_i >= 0 && prev_j >= 0) {
                    double cost = cost_matrix[prev_i][prev_j] + 
                                 distance_matrix[i-1][j-1] * directional_weights[d];
                    
                    if (cost < min_cost) {
                        min_cost = cost;
                        best_direction = static_cast<int>(d);
                    }
                }
            }
            
            cost_matrix[i][j] = min_cost;
            backtrack[i-1][j-1] = best_direction;
        }
    }
    
    // Backward pass - reconstruct path
    DTWPath path;
    int i = static_cast<int>(M) - 1;
    int j = static_cast<int>(N) - 1;
    
    path.emplace_back(i, j);
    
    while (i > 0 || j > 0) {
        int direction_idx = backtrack[i][j];
        if (direction_idx >= 0 && direction_idx < static_cast<int>(directions.size())) {
            i -= directions[direction_idx].row_step;
            j -= directions[direction_idx].col_step;
            path.emplace_back(i, j);
        } else {
            break;
        }
    }
    
    // Reverse path
    std::reverse(path.begin(), path.end());
    
    // Remove padding from cost matrix
    Matrix2D<double> result_cost(M, N);
    for (size_t i = 0; i < M; ++i) {
        for (size_t j = 0; j < N; ++j) {
            result_cost[i][j] = cost_matrix[i+1][j+1];
        }
    }
    
    return {std::move(result_cost), std::move(path)};
}

} // namespace parangonar