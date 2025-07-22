#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <vector>
#include <list>
#include <chrono>

// ==================== ESTRUTURAS DE DADOS ====================
struct Passenger
{
    int wk; // custo máximo que aceita pagar
    int ok; // origem
    int dk; // destino
    int tk; // tempo máximo que aceita
};

struct Solution
{
    double cost, time, total_bonus;
    std::vector<int> route;
    std::vector<bool> cities_colected;
    std::vector<bool> passengers_riding;
    double fitness;
};

// Estrutura para armazenar conjuntos de soluções por geração
struct Pareto_objectives {
    std::vector<Solution> solutions;
    std::vector<Solution> pareto_set;
};

struct Generations {
    std::vector<Pareto_objectives> generations;
    std::vector<std::chrono::duration<double>> durations;
};

// Funções auxiliares para manipulação de Solution
double getObj(const Solution& s, int k);
bool x_dominates_y(const Solution& x, const Solution& y);
bool solutions_are_equal(const Solution& a, const Solution& b);
double euclidean_distance(const Solution& a, const Solution& b);
double get_tchebycheff(const Solution& solution, std::vector<float> z, std::vector<double> weight_vector);
void print_solution(const Solution& solution);
bool solution_validity(const Solution& solution);

#endif // STRUCTURES_H 