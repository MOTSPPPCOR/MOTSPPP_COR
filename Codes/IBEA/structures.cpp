#include "structures.h"
#include <cmath>
#include <iostream>
#include <limits>

using namespace std;

// Constante para comparações de ponto flutuante
const double EPS = 1e-9;

double getObj(const Solution& s, int k) {
    switch(k) {
        case 0: return s.cost;
        case 1: return s.time;
        case 2: return -s.total_bonus; // Negativo porque queremos maximizar bônus
        default: return 0.0;
    }
}

bool x_dominates_y(const Solution& x, const Solution& y) {
    // Verifica se x domina y (considerando maximização do bônus)
    bool leq = (fabs(x.cost - y.cost) < EPS || x.cost < y.cost) &&
               (fabs(x.time - y.time) < EPS || x.time < y.time) &&
               (fabs(x.total_bonus - y.total_bonus) < EPS || x.total_bonus > y.total_bonus);
               
    bool strict = (x.cost + EPS < y.cost) ||
                 (x.time + EPS < y.time) ||
                 (x.total_bonus > y.total_bonus + EPS);
                 
    return leq && strict;
}

// Verifica se duas soluções são iguais (considerando epsilon)
bool solutions_are_equal(const Solution& a, const Solution& b) {
    return fabs(a.cost - b.cost) < EPS &&
           fabs(a.time - b.time) < EPS &&
           fabs(a.total_bonus - b.total_bonus) < EPS;
}

double euclidean_distance(const Solution& a, const Solution& b) {
    double d1 = a.cost - b.cost;
    double d2 = a.time - b.time;
    double d3 = a.total_bonus - b.total_bonus;
    return sqrt(d1*d1 + d2*d2 + d3*d3);
}

double get_tchebycheff(const Solution& solution, vector<float> z, vector<double> weight_vector) {
    double max = -numeric_limits<double>::max();
    for (int i = 0; i < 3; i++) {
        double val = weight_vector[i] * abs(getObj(solution, i) - z[i]);
        if (val > max) max = val;
    }
    return max;
}

bool solution_validity(const Solution& solution) {
    // Implementação básica - você pode expandir conforme necessário
    return solution.route.size() >= 2 && solution.route[0] == 0;
} 