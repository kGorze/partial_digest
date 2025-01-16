#include "map_solver.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <iomanip>

MapSolver::MapSolver(const std::vector<int>& inputDistances, int length) 
    : distances(inputDistances)
    , totalLength(length)
    , processedPaths(0) 
{
    std::sort(distances.begin(), distances.end());
    maxind = static_cast<int>((1 + std::sqrt(1 + 8.0 * distances.size())) / 2);
    currentMap.resize(maxind, 0);
    totalPaths = calculateTotalPaths();
    
    initializeRemainingDistances();
    
    stats = Statistics{
        totalPaths,
        0,
        0.0,
        {},
        inputDistances,
        false
    };
}

void MapSolver::initializeRemainingDistances() {
    remainingDistances.clear();
    for (int d : distances) {
        remainingDistances[d]++;
    }
}

uint64_t MapSolver::calculateTotalPaths() const {
    // Przykładowy, nieskomplikowany szacunek
    uint64_t total = 1;
    for (int i = 1; i < maxind - 1; i++) {
        total *= (totalLength - i);
    }
    return total;
}

void MapSolver::updateProgress() {
    processedPaths++;
    if (processedPaths % 1000 == 0) {
        displayProgress();
    }
}

bool MapSolver::updateDistanceUsage(int distance, bool add) {
    if (add) {
        remainingDistances[distance]++;
        return true;
    }
    auto it = remainingDistances.find(distance);
    if (it == remainingDistances.end() || it->second == 0) {
        return false;
    }
    it->second--;
    return true;
}

std::vector<int> MapSolver::getUnusedDistances() const {
    std::vector<int> unused;
    for (const auto& [distance, count] : remainingDistances) {
        for (int i = 0; i < count; i++) {
            unused.push_back(distance);
        }
    }
    return unused;
}

void MapSolver::printRemainingDistances() const {
    std::cout << "Remaining distances: ";
    for (const auto& [distance, count] : remainingDistances) {
        if (count > 0) {
            std::cout << distance << "(" << count << ") ";
        }
    }
    std::cout << "\n";
}

void MapSolver::displayProgress() const {
    auto currentTime = std::chrono::steady_clock::now();
    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        currentTime - startTime
    ).count();
    
    std::cout << "\rPaths processed: " << processedPaths 
              << " | Time: " << elapsedMs << "ms" << std::flush;
}

std::vector<int> MapSolver::calculateDistancesBetweenPoints(const std::vector<int>& points) const {
    std::vector<int> dists;
    for (size_t i = 1; i < points.size(); i++) {
        dists.push_back(points[i] - points[i-1]);
    }
    return dists;
}

bool MapSolver::isValidPartialSolution(int assignedCount) const {
    std::vector<int> currentDistances;
    currentDistances.reserve((assignedCount * (assignedCount - 1)) / 2);
    
    for (int i = 0; i < assignedCount; i++) {
        for (int j = 0; j < i; j++) {
            int distance = currentMap[i] - currentMap[j];
            if (distance < 0 || distance > totalLength) {
                return false;
            }
            currentDistances.push_back(distance);
        }
    }
    
    if (assignedCount <= 1) {
        return true;
    }
    
    std::sort(currentDistances.begin(), currentDistances.end());
    std::vector<int> tempDistances = distances;
    for (int dist : currentDistances) {
        auto it = std::find(tempDistances.begin(), tempDistances.end(), dist);
        if (it == tempDistances.end()) {
            return false;
        }
        tempDistances.erase(it);
    }
    return true;
}

void MapSolver::szukaj(int ind, bool* jest) {
    processedPaths++;
    if (*jest) return;
    
    if (ind == maxind) {
        updateProgress();
        if (isValidPartialSolution(maxind)) {
            *jest = true;
            stats.solution = currentMap;
            stats.solutionFound = true;
        }
        return;
    }
    
    int startVal, endVal;
    if (ind == 0) {
        startVal = endVal = 0;
    } else if (ind == maxind - 1) {
        startVal = endVal = totalLength;
    } else {
        startVal = currentMap[ind - 1] + 1;
        endVal = totalLength - (maxind - ind - 1);
    }
    
    for (int pos = startVal; pos <= endVal; pos++) {
        currentMap[ind] = pos;
        if (isValidPartialSolution(ind + 1)) {
            szukaj(ind + 1, jest);
        }
        if (*jest) break;
    }
}

std::optional<std::vector<int>> MapSolver::solve() {
    bool jest = false;
    startTime = std::chrono::steady_clock::now();
    
    szukaj(0, &jest);
    
    auto endTime = std::chrono::steady_clock::now();
    stats.searchTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime
    ).count();
    stats.processedPaths = processedPaths;
    
    if (jest) {
        return stats.solution;
    }
    return std::nullopt;
}

std::optional<std::vector<int>> MapSolver::solveWithCondition() {
    bool jest = false;
    startTime = std::chrono::steady_clock::now();
    
    std::fill(currentMap.begin(), currentMap.end(), -1);
    currentMap[0] = 0;
    currentMap[maxind - 1] = totalLength;
    
    initializeRemainingDistances();
    if (!updateDistanceUsage(totalLength, false)) {
        return std::nullopt;
    }
    
    szukajWithCondition(1, &jest);
    
    auto endTime = std::chrono::steady_clock::now();
    stats.searchTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime
    ).count();
    stats.processedPaths = processedPaths;
    
    if (jest) {
        return stats.solution;
    }
    return std::nullopt;
}

void MapSolver::szukajWithCondition(int ind, bool* jest) {
    if (*jest) return;
    processedPaths++;
    
    if (ind == maxind - 1) {
        updateProgress();
        bool allUsed = true;
        for (const auto& [distance, cnt] : remainingDistances) {
            if (cnt != 0) {
                allUsed = false;
                break;
            }
        }
        if (allUsed) {
            *jest = true;
            stats.solution = currentMap;
            stats.solutionFound = true;
        }
        return;
    }
    
    int startVal = currentMap[ind - 1] + 1;
    int endVal = totalLength - (maxind - ind - 1);
    
    for (int pos = startVal; pos <= endVal; pos++) {
        bool canPlace = true;
        std::vector<int> usedDistances;
        
        for (int j = 0; j < ind; j++) {
            if (currentMap[j] == -1) continue;
            int dist = std::abs(pos - currentMap[j]);
            if (!updateDistanceUsage(dist, false)) {
                canPlace = false;
                break;
            }
            usedDistances.push_back(dist);
        }
        if (canPlace) {
            int distToEnd = totalLength - pos;
            if (!updateDistanceUsage(distToEnd, false)) {
                canPlace = false;
            } else {
                usedDistances.push_back(distToEnd);
            }
        }
        
        if (canPlace) {
            currentMap[ind] = pos;
            szukajWithCondition(ind + 1, jest);
            if (*jest) return;
            currentMap[ind] = -1;
        }

        for (int d : usedDistances) {
            updateDistanceUsage(d, true);
        }
    }
}

bool MapSolver::canAddPosition(int pos, int ind, std::vector<int>& usedDistances) {
    for (int i = 0; i < ind; i++) {
        int dist = std::abs(currentMap[i] - pos);
        auto it = distanceCounter.find(dist);
        if (it == distanceCounter.end() || it->second == 0) {
            return false;
        }
        it->second--;
        usedDistances.push_back(dist);
    }
    int dist0 = std::abs(pos);
    if (distanceCounter[dist0] > 0) {
        distanceCounter[dist0]--;
        usedDistances.push_back(dist0);
    } else {
        return false;
    }
    int distEnd = std::abs(totalLength - pos);
    if (distanceCounter[distEnd] > 0) {
        distanceCounter[distEnd]--;
        usedDistances.push_back(distEnd);
    } else {
        return false;
    }
    return true;
}

void MapSolver::revertPosition(const std::vector<int>& usedDistances) {
    for (int d : usedDistances) {
        distanceCounter[d]++;
    }
}

bool MapSolver::allDistancesUsedUp() const {
    for (const auto& [distance, count] : distanceCounter) {
        if (count != 0) {
            return false;
        }
    }
    return true;
}
