#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <tuple>
#include <vector>
#include <chrono>
#include <array>
#include <filesystem>
#include "structures.h"
#include "BoundedParetoSet.h"

using namespace std;

// Constante para comparações de ponto flutuante
const double EPS = 1e-9;

// ==================== ESTRUTURAS DE DADOS ====================
struct InputData
{
    int n, l, vehicle_capacity;
    vector<vector<int>> c, y; // matrizes de custo e tempo
    vector<int> b, g;         // bônus e tempo de coleta
    vector<Passenger> passengers;
};

// ==== VAR GLOBAIS PARA IBEA ====
struct Range
{
    double min, max;
};
static Range boundsArr[3];

using NormSol = std::array<double,3>; 

// ==================== LEITURA DE ARQUIVO ====================
InputData readInput(const string &filename)
{
    ifstream file(filename);
    if (!file.is_open())
    {
        cerr << "Erro ao abrir instância: " << filename << endl;
        exit(EXIT_FAILURE);
    }
    InputData data;
    int unused;
    file >> data.n >> data.l >> data.vehicle_capacity;
    data.c.assign(data.n, vector<int>(data.n));
    data.y.assign(data.n, vector<int>(data.n));
    for (int i = 0; i < data.n; ++i)
        for (int j = 0; j < data.n; ++j)
            file >> data.c[i][j];
    for (int i = 0; i < data.n; ++i)
        for (int j = 0; j < data.n; ++j)
            file >> data.y[i][j];
    data.passengers.resize(data.l);
    for (int i = 0; i < data.l; ++i)
        file >> data.passengers[i].wk >> data.passengers[i].ok >> data.passengers[i].dk >> data.passengers[i].tk;
    file >> unused; // quota mínimo (não usado)
    data.b.assign(data.n, 0);
    data.g.assign(data.n, 0);
    for (int i = 0; i < data.n; ++i)
    {
        int idx, bonus, collect;
        file >> idx >> bonus >> collect;
        data.b[idx] = bonus;
        data.g[idx] = collect;
    }
    return data;
}

// ==================== UTILITÁRIOS PARETO ====================
static const double eps = 1e-9;
bool dominates(const Solution &a, const Solution &b) {
  bool leq = (a.cost <= b.cost + eps)
          && (a.time <= b.time + eps)
          && (a.total_bonus + eps >= b.total_bonus);
  bool strict = (a.cost < b.cost - eps)
             || (a.time < b.time - eps)
             || (a.total_bonus > b.total_bonus + eps);
  return leq && strict;
}

vector<Solution> getNonDominated(const vector<Solution>& pop) {
  int n = pop.size();
  vector<bool> isDominated(n, false);

  // marca quem é dominado
  for (int i = 0; i < n; ++i) {
    if (isDominated[i]) continue;
    for (int j = 0; j < n; ++j) {
      if (i == j) continue;
      if (dominates(pop[j], pop[i])) {
        isDominated[i] = true;
        break;
      }
    }
  }

  // coleta só os não‐dominados
  vector<Solution> nd;
  nd.reserve(n);
  for (int i = 0; i < n; ++i)
    if (!isDominated[i])
      nd.push_back(pop[i]);

  // remover duplicatas exatas (após epsilon)
  for (int i = 0; i < (int)nd.size(); ++i) {
    for (int j = i + 1; j < (int)nd.size(); ++j) {
      if (fabs(nd[i].cost - nd[j].cost) < eps &&
          fabs(nd[i].time - nd[j].time) < eps &&
          fabs(nd[i].total_bonus - nd[j].total_bonus) < eps) {
        nd.erase(nd.begin() + j);
        --j;
      }
    }
  }

  return nd;
}

tuple<double, double, double, double, double, double> calcBounds(const vector<Solution> &pop)
{
    double min_c = numeric_limits<double>::max(), max_c = numeric_limits<double>::lowest();
    double min_t = numeric_limits<double>::max(), max_t = numeric_limits<double>::lowest();
    double min_b = numeric_limits<double>::max(), max_b = numeric_limits<double>::lowest();
    for (const auto &s : pop)
    {
        min_c = min(min_c, s.cost);
        max_c = max(max_c, s.cost);
        min_t = min(min_t, s.time);
        max_t = max(max_t, s.time);
        min_b = min(min_b, s.total_bonus);
        max_b = max(max_b, s.total_bonus);
    }
    return {min_c, max_c, min_t, max_t, min_b, max_b};
}

// ==================== CÁLCULO DE CUSTO/TEMPO ====================
double calculate_passenger_cost(int origem_index,
    int destiny_index,
    const vector<int> &in_car,
    const Solution &solution,
    const InputData &instance)
{
    double cost = 0.0;
    int m = solution.route.size();

    // caso destino seja a cidade inicial (0)
    if (destiny_index == 0) {
        cost += instance.c[solution.route[m-1]][solution.route[0]] / double(1 + in_car[m-1]);
        destiny_index = m-1;
    }
    for (int i = origem_index; i < destiny_index; ++i) {
        int u = solution.route[i];
        int v = solution.route[i+1];
        cost += instance.c[u][v] / double(1 + in_car[i]);
    }
    return cost;
}

double calculate_passenger_time(int origem_index,
    int destiny_index,
    const Solution &solution,
    const InputData &instance)
{
    double time = 0.0;
    int m = solution.route.size();

    // caso destino seja a cidade inicial (0)
    if (destiny_index == 0) {
        time += instance.y[solution.route[m-1]][solution.route[0]];
        destiny_index = m-1;
    }
    for (int i = origem_index; i < destiny_index; ++i) {
        int u = solution.route[i];
        int v = solution.route[i+1];
        time += instance.y[u][v];
        if (solution.cities_colected[i]) {
            time += instance.g[u];
        }
    }
    if (solution.cities_colected[destiny_index]) {
        int u = solution.route[destiny_index];
        time += instance.g[u];
    }
    return time;
}

// ==================== EMBARQUE DE PASSAGEIROS ====================
void able_passengers(const InputData &d, Solution &sol)
{
    vector<bool> able(d.l, false);
    vector<int> in_car(sol.route.size(), 1);
    vector<pair<double, int>> order;
    for (int i = 0; i < d.l; ++i)
        order.emplace_back(d.passengers[i].wk, i);
    sort(order.begin(), order.end()); // do pobre ao rico
    for (int idx = (int)order.size() - 1; idx >= 0; --idx)
    {
        int pid = order[idx].second;
        bool sub = false, des = false;
        int oi = -1, di = -1;
        bool apto = true;
        for (int city = 0; city < (int)sol.route.size(); ++city)
        {
            int v = sol.route[city];
            if (!sub && v == d.passengers[pid].ok)
            {
                if (in_car[city] < d.vehicle_capacity)
                {
                    sub = true;
                    oi = city;
                }
                else
                {
                    apto = false;
                    break;
                }
            }
            if (sub && v == d.passengers[pid].dk)
            {
                des = true;
                di = city;
                break;
            }
            if (sub && in_car[city] >= d.vehicle_capacity)
            {
                apto = false;
                break;
            }
        }
        if (!sub || !des || !apto)
            continue;
        if (sol.route[0] == d.passengers[pid].dk && in_car.back() >= d.vehicle_capacity)
            continue;
        double c = calculate_passenger_cost(oi, di, in_car, sol, d);
        double t = calculate_passenger_time(oi, di, sol, d);
        if (c > d.passengers[pid].wk || t > d.passengers[pid].tk)
            continue;
        able[pid] = true;
        if (di == (int)sol.route.size() - 1 && d.passengers[pid].dk == sol.route[0])
            in_car.back()++;
        for (int k = oi; k < di; ++k)
            in_car[k]++;
    }
    sol.passengers_riding = able;
}


// ==================== Validação ===================================
double recalc_cost(const InputData &d, const Solution &sol) {
    double cost = 0;
    int people = 1;
    int m = sol.route.size();
    for (int i = 0; i < m; ++i) {
        int u = sol.route[i];
        int v = sol.route[(i+1)%m];
        double step = d.c[u][v];
        // conta passageiros a bordo neste trecho
        for (int p = 0; p < d.l; ++p) {
            if (sol.passengers_riding[p]) {
                if (d.passengers[p].ok == u)   ++people;
                if (d.passengers[p].dk == u && i!=0) --people;
            }
        }
        cost += step / people;
    }
    return cost;
}

double recalc_time(const InputData &d, const Solution &sol) {
    double t = 0;
    int m = sol.route.size();
    for (int i = 0; i < m; ++i) {
        int u = sol.route[i];
        int v = sol.route[(i+1)%m];
        t += d.y[u][v];
        if (sol.cities_colected[i])
            t += d.g[u];
    }
    return t;
}

double recalc_bonus(const InputData &d, const Solution &sol) {
    double B = 0;
    for (int i = 0; i < sol.route.size(); ++i)
        if (sol.cities_colected[i])
            B += d.b[sol.route[i]];
    return B;
}

// verifica se os passageiros em solution.passengers_riding são válidos
bool recalc_passengers_ok(const InputData &instance,
const Solution &solution)
{
int m = solution.route.size();
// começa com 1 pessoa (o motorista) em cada trecho
vector<int> in_car(m, 1);
// ordena passageiros de pior para melhor pagador
vector<pair<double,int>> by_cost;
for (int i = 0; i < instance.l; ++i) {
if (solution.passengers_riding[i]) {
by_cost.emplace_back(instance.passengers[i].wk, i);
}
}
if (by_cost.empty()) return true;

// do pobre ao rico
sort(by_cost.begin(), by_cost.end());

// testa cada passageiro de rico a pobre
for (int idx = (int)by_cost.size()-1; idx >= 0; --idx) {
int pid = by_cost[idx].second;
bool subiu = false, desceu = false;
int oi = -1, di = -1;
bool apto = true;

// percorre rota tentando embarcar e desembarcar
for (int city = 0; city < m; ++city) {
int v = solution.route[city];
if (!subiu && v == instance.passengers[pid].ok) {
// embarque
if (in_car[city] < instance.vehicle_capacity) {
subiu = true;
oi = city;
} else {
return false;
}
}
else if (subiu && v == instance.passengers[pid].dk) {
// desembarque
if (city == 0) { 
desceu = true;
di = city;
} else {
desceu = true;
di = city;
}
break;
}
else if (subiu && in_car[city] >= instance.vehicle_capacity) {
// carro lotado no meio do trajeto
return false;
}
}

// se origem e destino foram encontrados
if (!subiu || !desceu) continue;

// special: destino volta à 0
if (solution.route[0] == instance.passengers[pid].dk &&
in_car[m-1] >= instance.vehicle_capacity)
return false;

// calcula custo e tempo do passageiro
double c = calculate_passenger_cost(oi, di, in_car, solution, instance);
double t = calculate_passenger_time(oi, di, solution, instance);

// verifica se passageiro pode pagar
if (c > instance.passengers[pid].wk ||
t > instance.passengers[pid].tk)
return false;

// marca ocupação do carro nos trechos
if (di == 0) {
in_car[m-1]++;
di = m-1;
}
for (int k = oi; k < di; ++k) {
in_car[k]++;
}
}

return true;
}


bool solution_validity(const InputData &d, const Solution &sol) {
    bool ok = true;

    // 1) rota mínima e origem = 0
    if (sol.route.size() < 2) { 
     ok = false; 
    }
    if (sol.route.front() != 0) {
       ok = false;
    }

    // 2) objetivos não nulos
    if (sol.cost < EPS || sol.time < EPS) {
       ok = false;
    }

    // 3) bate com recalculado?
    double cost2  = recalc_cost(d, sol);
    double time2  = recalc_time(d, sol);
    double bonus2 = recalc_bonus(d, sol);

    if (fabs(cost2  - sol.cost)  > EPS) {
        ok = false;
    }
    if (fabs(time2  - sol.time)  > EPS) {
        ok = false;
    }
    if (fabs(bonus2 - sol.total_bonus) > EPS) {
        ok = false;
    }

    // 4) passageiros
    if (!recalc_passengers_ok(d, sol)) {
        ok = false;
    }

    if (!ok) {
        // imprime rota inteira para DEBUG
    }
    return ok;
}

// ==================== ATUALIZAÇÃO DE OBJETIVOS ====================
void update_objectives(const InputData &d, Solution &sol)
{
    // Calcular tempo
    sol.time = 0;
    for (int city = 0; city < sol.route.size(); city++)
    {
        int next_city;
        if (city == sol.route.size() - 1)
        {
            next_city = 0;
        }
        else
        {
            next_city = city + 1;
        }
        sol.time += d.y[sol.route[city]][sol.route[next_city]];

        if (sol.cities_colected[city])
        { // add time from bonus collecting
            sol.time += d.g[sol.route[city]];
        }
    }

    // Calcular custo
    sol.cost = 0;
    int people_in_car = 1;
    for (int city = 0; city < sol.route.size(); city++)
    {
        double temp_cost = 0;
        int next_city;
        if (city == sol.route.size() - 1)
        {
            next_city = 0;
        }
        else
        {
            next_city = city + 1;
        }
        temp_cost = d.c[sol.route[city]][sol.route[next_city]];
        if (sol.passengers_riding.size() > 0)
        {
            for (int passenger = 0; passenger < d.l; passenger++)
            {
                if (sol.passengers_riding[passenger] == true)
                {
                    if (d.passengers[passenger].ok == sol.route[city])
                    {
                        people_in_car++;
                    }
                    if (d.passengers[passenger].dk == sol.route[city])
                    {
                        if (city != 0)
                        {
                            people_in_car--;
                        }
                    }
                }
            }
        }
        temp_cost /= people_in_car;
        sol.cost += temp_cost;
    }

    // Calcular bônus
    sol.total_bonus = 0;
    for (int city = 0; city < sol.route.size(); city++)
    {
        if (sol.cities_colected[city])
        { // add bonus from cities collected
            sol.total_bonus += d.b[sol.route[city]];
        }
    }
}

// ==================== INICIALIZAÇÃO ALEATÓRIA ====================
Solution get_random_solution(const InputData &data)
{
    Solution sol;
    // escolhe número de cidades >=2
    int number_of_cities = 0;
    while (number_of_cities < 2)
    {
        number_of_cities = rand() % data.n;
    } // define a number of cities >= 2
    vector<int> visited_cities(data.n, -1);
    sol.route.push_back(0); // satisfy origin always 0
    visited_cities[0] = 0;
    int i = 1;
    while (i < number_of_cities)
    {
        int city = rand() % data.n;
        if (visited_cities[city] != city)
        {
            sol.route.push_back(city);
            visited_cities[city] = city;
            i++;
        }
    }
    sol.cities_colected.push_back(false); // first city is not collected
    for (int i = 0; i < sol.route.size() - 1; i++)
    {
        if (rand() % 2 == 0)
        {
            sol.cities_colected.push_back(true);
        }
        else
        {
            sol.cities_colected.push_back(false);
        }
    }
    sol.passengers_riding.assign(data.l, false);
    return sol;
}

vector<Solution> get_random_population(const InputData &data, BoundedParetoSet *EP, int pop_size)
{
    vector<Solution> pop;
    while ((int)pop.size() < pop_size)
    {
        Solution sol = get_random_solution(data);
        able_passengers(data, sol);
        update_objectives(data, sol);
        
        if(solution_validity(data, sol))
        {
            pop.push_back(sol);
            EP->adicionarSol(&sol);  // Adiciona ao EP se for não dominada
        }
    }
    return pop;
}

// ==================== OPERADORES GENÉTICOS ====================
Solution one_crossover(const InputData &d, const vector<Solution> &pop, int father, int mother)
{
    Solution baby;
    for (int i = 0; i < pop[father].route.size() / 2; i++)
    {
        baby.route.push_back(pop[father].route[i]);
        baby.cities_colected.push_back(pop[father].cities_colected[i]);
    } // first half of the route and collections from the father

    for (int j = pop[mother].route.size() / 2; j < pop[mother].route.size(); j++)
    {
        bool gene_is_repeated = false;
        for (int k = 0; k < pop[father].route.size() / 2; k++)
        {
            if (pop[mother].route[j] == pop[father].route[k])
            {
                gene_is_repeated = true;
            }
        }
        if (gene_is_repeated == false)
        {
            baby.route.push_back(pop[mother].route[j]);
            baby.cities_colected.push_back(pop[mother].cities_colected[j]);
        }
    } // second half of the route and collections from the mother
    baby.passengers_riding.assign(d.l, false);
    return baby;
}

vector<Solution> crossover(const InputData &d, const vector<Solution> &parents)
{
    vector<Solution> children;
    while (children.size() < parents.size())
    {
        int father = rand() % parents.size();
        int mother = rand() % parents.size();
        while (mother == father)
        {
            mother = rand() % parents.size();
        }
        Solution baby = one_crossover(d, parents, father, mother);
        able_passengers(d, baby);
        update_objectives(d, baby);
        children.push_back(baby);
    }
    return children;
}

void mutate_routes(const InputData &d, Solution &sol, int mode)
{
    if (mode == 3)
    {
        if (sol.route.size() <= 2)
        {
            mode = 2;
        }
        else if (sol.route.size() == d.n)
        {
            mode = rand() % 2;
        }
        else
        {
            mode = rand() % 3;
        }
    } // this if ensures that if mutate_routes is called, it will definitely occur a correct mutation.
    if (mode == 0)
    { // swap vertex (but not origin)
        int city1 = rand() % sol.route.size();
        while (city1 == 0)
        {
            city1 = rand() % sol.route.size();
        }
        int city2 = rand() % sol.route.size();
        while (city2 == 0)
        {
            city2 = rand() % sol.route.size();
        }
        swap(sol.route[city1], sol.route[city2]);
        swap(sol.cities_colected[city1], sol.cities_colected[city2]);
    }
    else if (mode == 1 && sol.route.size() > 2)
    { // remove random vertex, but not origin
        int city_to_diminish = 0;
        while (city_to_diminish == 0)
        {
            city_to_diminish = rand() % sol.route.size();
        }
        sol.route.erase(sol.route.begin() + city_to_diminish);
        sol.cities_colected.erase(sol.cities_colected.begin() + city_to_diminish);
    }
    else if (mode == 2 && sol.route.size() != d.n)
    {
        // add random vertex that is not already in the route in any place except origin
        vector<int> cities_not_in_route;
        bool marker = false;
        for (int j = 0; j < d.n; j++)
        {
            marker = false;
            for (int k = 0; k < sol.route.size(); k++)
            {
                if (sol.route[k] == j)
                {
                    marker = true;
                    break;
                }
            }
            if (marker == false)
            {
                cities_not_in_route.push_back(j);
            }
        }
        int city_to_add = rand() % cities_not_in_route.size();
        int city_to_add_index = rand() % sol.route.size();
        while (city_to_add_index == 0)
        {
            city_to_add_index = rand() % sol.route.size();
        }
        sol.route.insert(sol.route.begin() + city_to_add_index, cities_not_in_route[city_to_add]);
        sol.cities_colected.insert(sol.cities_colected.begin() + city_to_add_index, false);
    }
}

void mutate_bonuses(const InputData &d, Solution &sol)
{
    int bonus_change = rand() % sol.cities_colected.size();
    while (bonus_change == 0)
    {
        bonus_change = rand() % sol.cities_colected.size();
    }
    if (sol.cities_colected[bonus_change] == false)
    {
        sol.cities_colected[bonus_change] = true;
    }
    else
    {
        sol.cities_colected[bonus_change] = false;
    }
} // mutates a solution by inverting a bit in bonus colecting vector

void invert_bonuses(const InputData &d, Solution &sol)
{
    for (int i = 0; i < sol.cities_colected.size(); i++)
    {
        if (sol.cities_colected[i] == true)
        {
            sol.cities_colected[i] = false;
        }
        else
        {
            sol.cities_colected[i] = true;
        }
    }
}

// ==================== IBEA FUNCTIONS ====================

vector<NormSol> normalizePopulation(const vector<Solution> &pop, double rho)
{
    auto [minc, maxc, mint, maxt, minb, maxb] = calcBounds(pop);
    vector<NormSol> N;
    N.reserve(pop.size());

    for (auto &s : pop)
    {
        double ac = maxc + rho * (maxc - minc);
        double at = maxt + rho * (maxt - mint);
        NormSol ns = {
            (s.cost - minc) / (ac - minc),
            (s.time - mint) / (at - mint),
            1.0 - (s.total_bonus - minb) / (maxb - minb + 1e-9)
        };
        N.push_back(ns);
    }
    return N;
}

double calcHypervolumeIndicator(int i, int j, int d, const vector<NormSol> &pop)
{
    double r = 1.0; // normalized box
    double ai = pop[i][d - 1], bi = (j < 0 ? r : pop[j][d - 1]);
    if (d == 1)
        return ai < bi ? (bi - ai) / r : 0;
    if (ai < bi)
        return calcHypervolumeIndicator(i, -1, d - 1, pop) * (bi - ai) / r + calcHypervolumeIndicator(i, j, d - 1, pop) * (r - bi) / r;
    return calcHypervolumeIndicator(i, j, d - 1, pop) * (r - ai) / r;
}

double indicatorValue(int i, int j, const vector<NormSol> &pop)
{
    bool aDom = true;
    for (int k = 0; k < 3; k++)
        if (pop[i][k] > pop[j][k])
        {
            aDom = false;
            break;
        }
    return aDom ? -calcHypervolumeIndicator(i, j, 3, pop) : calcHypervolumeIndicator(j, i, 3, pop);
}

vector<double> computeFitness(const vector<NormSol> &N, double k)
{
    int M = N.size();
    vector<vector<double>> I(M, vector<double>(M));
    double mx = 0;
    for (int p = 0; p < M; p++)
        for (int q = 0; q < M; q++)
            if (p != q)
            {
                I[p][q] = indicatorValue(p, q, N);
                mx = max(mx, fabs(I[p][q]));
            }
    vector<double> F(M, 0);
    for (int j = 0; j < M; j++)
        for (int i = 0; i < M; i++)
            if (i != j)
                F[j] += -exp(-I[i][j] / (mx * k));
    return F;
}

void environmentalSelection(vector<Solution> &P, vector<double> &f, int MU)
{
    while (P.size() > MU)
    {
        int w = distance(f.begin(), max_element(f.begin(), f.end()));
        P.erase(P.begin() + w);
        f.erase(f.begin() + w);
    }
}

int tournamentSelect(const vector<Solution> &P, int t)
{
    int w = rand() % P.size();
    for (int i = 1; i < t; i++)
    {
        int o = rand() % P.size();
        if (P[o].fitness < P[w].fitness)
            w = o;
    }
    return w;
}

// ==================== ESCRITA DE RESULTADOS ====================
void writeResults(const vector<Solution> &sols, const string &path, const string &name)
{
    ofstream out(path + name);
    out << "Fo(custo)\tFo(tempo)\tFo(bonus)\n\n";
    for (const auto &s : sols)
    {
        out << s.cost << "\t" << s.time << "\t" << s.total_bonus << "\n";
        out << "tour: ";
        for (int city : s.route)
            out << city << " ";
        out << "0\n";
        out << "bonus: ";
        for (size_t i = 0; i < s.cities_colected.size(); ++i)
            if (s.cities_colected[i])
                out << s.route[i] << " ";
        out << "\npassengers: ";
        for (size_t pid = 0; pid < s.passengers_riding.size(); ++pid)
            if (s.passengers_riding[pid])
                out << pid << " ";
        out << "\n\n";
    }
}

int random_chance() {
    return rand() % 100;
}


// gera exatamente |parents| filhos aplicando crossover e depois possivelmente mutação
vector<Solution> generateOffspring(
    const InputData &d,
    const vector<Solution> &parents,
    int crossover_chance,
    int mutation_chance
) {
    vector<Solution> kids;
    kids.reserve(parents.size());

    for (size_t i = 0; i < parents.size(); ++i) {
        Solution child;
        // 2.1) Crossover X geração aleatória pura
        if (random_chance() < crossover_chance) {
            int p1 = rand() % parents.size();
            int p2 = rand() % parents.size();
            while (p2 == p1) p2 = rand() % parents.size();
            child = one_crossover(d, parents, p1, p2);
        } else {
            child = get_random_solution(d);
        }

        // reembarque e objetivos
        able_passengers(d, child);
        update_objectives(d, child);

        // 2.2) mutação opcional
        if (random_chance() < mutation_chance) {
            mutate_routes(d, child, 3);
            mutate_bonuses(d, child);
            able_passengers(d, child);
            update_objectives(d, child);
        }

        kids.push_back(child);
    }
    return kids;
}

// vector<Solution> get_pareto_objectives(BoundedParetoSet *EP) {
//     vector<Solution> front;
//     int sz = EP->getSize();
//     front.reserve(sz);
    
//     for (int i = 0; i < sz; ++i) {
//         Solution s = EP->getSolucao(i);  // Get the Solution object directly
//         front.push_back(s);  // Add the Solution object to the front
//     }
    
//     return front;
// }

//version that will receive BoundedParetoSet
Pareto_objectives get_pareto_objectives(BoundedParetoSet *received_population){
  Pareto_objectives matrix_of_objectives;
  int pop_size = received_population->getSize();
  for(int solution =0;solution<pop_size;solution++){
    matrix_of_objectives.pareto_set.push_back(received_population->getSolucao(solution));
    matrix_of_objectives.solutions.push_back(received_population->getSolucao(solution));
  }
  return matrix_of_objectives;
} // returns a Pareto_objectives variable with all objectives from the population


void fill_complete_archive(const InputData& instance, ofstream& arquivo_completo, const Generations& saved_generations, int generation, int pareto_set) {
    const Solution& sol = saved_generations.generations[generation].pareto_set[pareto_set];
    
    // Write route
    arquivo_completo << "Rota: ";
    for(int i = 0; i < sol.route.size(); i++) {
        arquivo_completo << sol.route[i] << " ";
    }
    arquivo_completo << endl;
    
    // Write route costs
    arquivo_completo << "Custos da rota: ";
    for(int city = 0; city < sol.route.size(); city++) {
        int next_city = (city == sol.route.size() - 1) ? 0 : city + 1;
        arquivo_completo << instance.c[sol.route[city]][sol.route[next_city]] << " ";
    }
    arquivo_completo << endl;
    
    // Write route times
    arquivo_completo << "Tempos da rota: ";
    for(int city = 0; city < sol.route.size(); city++) {
        int next_city = (city == sol.route.size() - 1) ? 0 : city + 1;
        arquivo_completo << instance.y[sol.route[city]][sol.route[next_city]] << " ";
    }
    arquivo_completo << endl;
    
    // Write collected cities
    arquivo_completo << "Cities_colected: " << endl;
    bool any_city_colected = false;
    for(int i = 0; i < sol.cities_colected.size(); i++) {
        if(sol.cities_colected[i]) {
            arquivo_completo << "City: " << sol.route[i] 
                           << " bonus " << instance.b[sol.route[i]]
                           << " tempo " << instance.g[sol.route[i]] << endl;
            any_city_colected = true;
        }
    }
    if(!any_city_colected) {
        arquivo_completo << "Nenhuma cidade coletada" << endl;
    }
    
    // Write passenger information
    int passengers_on = 0;
    for(int i = 0; i < sol.passengers_riding.size(); i++) {
        if(sol.passengers_riding[i]) {
            passengers_on++;
            arquivo_completo << "passageiro " << i 
                           << " com origem " << instance.passengers[i].ok
                           << ", destino: " << instance.passengers[i].dk
                           << " custo max: " << instance.passengers[i].wk
                           << " tempo max: " << instance.passengers[i].tk << endl;
        }
    }
    arquivo_completo << "Passageiros embarcados: " << passengers_on << endl;
    
    // Write objective values
    arquivo_completo << "Custo: " << sol.cost << endl;
    arquivo_completo << "Bonus: " << sol.total_bonus << endl;
    arquivo_completo << "Tempo: " << sol.time << "\n\n";
}

// ==================== FUNÇÕES DE SALVAMENTO ====================
void save_data_routine(Generations& generations, BoundedParetoSet *EP, std::chrono::high_resolution_clock::time_point &inicio, int &biggest_multiple, int valuations, std::chrono::duration<double>& tempoTotal) {
    auto fim = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> tempotrecho = std::chrono::duration_cast<std::chrono::duration<double>>(fim - inicio); //update stretch time
    generations.durations.push_back(tempotrecho);
    tempoTotal += std::chrono::duration_cast<std::chrono::duration<double>>(fim - inicio); //adding total time
    vector<Solution> pareto_solutions;
    for(int i = 0; i < EP->getSize(); i++) {
        pareto_solutions.push_back(EP->getSolucao(i));
    }
    
    Pareto_objectives sets_of_objectives;
    sets_of_objectives.solutions = pareto_solutions;
    sets_of_objectives.pareto_set = pareto_solutions;
    generations.generations.push_back(sets_of_objectives);
    inicio = std::chrono::high_resolution_clock::now();
    
    // Incrementa o contador de múltiplos
    biggest_multiple++;
    
    // Debug para mostrar quando salva
}

void create_test_archives(
    const InputData& instance, 
    string algorithm_name, 
    string folder_name, 
    vector<string> instances_addresses, 
    int current_instance, 
    int current_test, 
    Generations& saved_generations, 
    unsigned int seed,
    std::chrono::duration<double> tempoTotal) {
    
    // Criar diretório base
    string base_dir = folder_name + "/" + instances_addresses[current_instance] + "/test_" + to_string(current_test);
    filesystem::create_directories(base_dir);
    
    for(int generation = 0; generation < saved_generations.generations.size(); generation++) {
        string endereço_do_arquivo = base_dir + "/paretogeracao_" + to_string(generation) + ".txt";
        string endereço_do_arquivo_completo = base_dir + "/paretogeracao_" + to_string(generation) + "_completo.txt";
        string endereço_dados = base_dir + "/dados";
                
        ofstream arquivo(endereço_do_arquivo);
        if (!arquivo.is_open()) {
            cerr << "Erro ao abrir arquivo: " << endereço_do_arquivo << endl;
            continue;
        }
        
        ofstream arquivo_completo(endereço_do_arquivo_completo);
        if (!arquivo_completo.is_open()) {
            cerr << "Erro ao abrir arquivo: " << endereço_do_arquivo_completo << endl;
            continue;
        }
        
        ofstream arquivo_dados(endereço_dados);
        if (!arquivo_dados.is_open()) {
            cerr << "Erro ao abrir arquivo: " << endereço_dados << endl;
            continue;
        }
        
        for(int pareto_set = 0; pareto_set < saved_generations.generations[generation].pareto_set.size(); pareto_set++) {
            arquivo << saved_generations.generations[generation].pareto_set[pareto_set].cost << " " 
                   << saved_generations.generations[generation].pareto_set[pareto_set].time << " " 
                   << saved_generations.generations[generation].pareto_set[pareto_set].total_bonus;
            
            fill_complete_archive(instance, arquivo_completo, saved_generations, generation, pareto_set);
            
            if(pareto_set < saved_generations.generations[generation].pareto_set.size() - 1) {
                arquivo << endl;
                arquivo_completo << endl;
            }
        }
        
        arquivo_dados << "seed: " << seed << endl;
        arquivo_dados << "tempo total em segundos: " << tempoTotal.count() << endl;
        for(int duration = 0; duration < saved_generations.durations.size(); duration++) {
            arquivo_dados << "tempo da geração " << duration << ":" << endl 
                         << saved_generations.durations[duration].count() << endl;
        }
        
        arquivo.close();
        arquivo_completo.close();
        arquivo_dados.close();
            }
}


void create_archive_IRACE(Pareto_objectives sets_of_objectives){
  string endereço_do_arquivo =  "testeirace.txt";
  ofstream arquivo(endereço_do_arquivo);
  for(int pareto_set = 0; pareto_set< sets_of_objectives.pareto_set.size() ; pareto_set++){
    arquivo<<sets_of_objectives.pareto_set[pareto_set].cost<<" "<< sets_of_objectives.pareto_set[pareto_set].time<<" " << sets_of_objectives.pareto_set[pareto_set].total_bonus;
    if(pareto_set<sets_of_objectives.pareto_set.size()-1){
      arquivo<<endl;
    }
  }
  arquivo.close();
}

int main(int argc, char* argv[]) {

    // Inicializa variáveis de tempo
    std::chrono::duration<double> tempoTotal = std::chrono::duration<double>::zero();
    auto start_time = std::chrono::high_resolution_clock::now();

    // Semente aleatória
    unsigned int seed = static_cast<unsigned int>(time(nullptr));
    srand(seed);

    // Parâmetros do mIBEA conforme artigo
    const int NUM_EXECUTIONS = 1;
    const int POP_SIZE   = 300;      // α = tamanho da população
    const int MAX_EVAL   = 80000;    // número máximo de avaliações
    // Parâmetros ajustáveis por linha de comando
    double KAPPA   = 0.05;     // valor default artigo mIBEA
    double RHO     = 2.0;      // ρ, define ponto de referência para Hypervolume - valor default artigo mIBEA
    int CROSSOVER_RATE = 93;   //  93% de chance de aplicar crossover
    int MUTATION_RATE  = 56;   // 56% de chance de aplicar mutação
    const int TOURNAM    = 2;        // dimensão do torneio para seleção.

    // Parse extra command-line arguments if provided
    // Usage: <instancia> [--kappa <KAPPA>] [--rho <RHO>] [--crossover <CROSSOVER_RATE>] [--mutation <MUTATION_RATE>]
    if (argc < 2) {
        cerr << "Uso: " << argv[0] << " <instancia> [--kappa <KAPPA>] [--rho <RHO>] [--crossover <CROSSOVER_RATE>] [--mutation <MUTATION_RATE>]" << endl;
        return 1;
    }
    string path_in  = "";

    for(int i=1; i<argc; i++) {
        string arg = argv[i];
        if ((arg == "-i" || arg == "--input") && i + 1 < argc) {
            path_in = argv[++i];
        } else if (arg == "--crossover" && i + 1 < argc) {
            CROSSOVER_RATE = stoi(argv[++i]);
        } else if (arg == "--mutation" && i + 1 < argc) {
            MUTATION_RATE = stoi(argv[++i]);
        } else if (arg == "--kappa" && i + 1 < argc) {
            KAPPA = stod(argv[++i]);
        } else if (arg == "--rho" && i + 1 < argc) {
            RHO = stod(argv[++i]);
        }
    }

    if (path_in.empty()) {
        cerr << "Uso: " << argv[0] << " -i <instancia> [--kappa <KAPPA>] [--rho <RHO>] [--crossover <CROSSOVER_RATE>] [--mutation <MUTATION_RATE>]" << endl;
        return 1;
    }

    // Caminhos de entrada e saída

    string instance_type;
    if (path_in.find("asym") != string::npos || path_in.find("asymmetric") != string::npos) {
        instance_type = "asym";
    } else {
        instance_type = "sym";
    }
    // Montar diretório base de evolução
    string base_dir = "./evolutions/a1/" + instance_type + "/" + path_in.substr(path_in.find_last_of("/\\") + 1);
    base_dir = base_dir.substr(0, base_dir.find_last_of('.'));
    filesystem::create_directories(base_dir);

    // Listar execuções existentes
    vector<bool> exec_exists(NUM_EXECUTIONS, false);
    int num_existing = 0;
    try {
        for (const auto& entry : filesystem::directory_iterator(base_dir)) {
            string folder_name = entry.path().filename().string();
            if (folder_name.find("exec_") == 0) {
                int exec_num = stoi(folder_name.substr(5));
                if (exec_num >= 1 && exec_num <= NUM_EXECUTIONS) {
                    exec_exists[exec_num - 1] = true;
                    num_existing++;
                }
            }
        }
    } catch (const exception& e) {
    }

    double hv_total = 0.0;
    if (num_existing >= NUM_EXECUTIONS) {
        // Calcular hv_total lendo os arquivos paretogeracao_8.txt de cada exec_N
        for (int exec = 1; exec <= NUM_EXECUTIONS; ++exec) {
            string exec_folder = base_dir + "/exec_" + to_string(exec);
            string pareto_file = exec_folder + "/paretogeracao_8.txt";
            ifstream in(pareto_file);
            if (!in.is_open()) continue;
            vector<Solution> pareto_final;
            double cost, time, bonus;
            while (in >> cost >> time >> bonus) {
                Solution s;
                s.cost = cost;
                s.time = time;
                s.total_bonus = bonus;
                pareto_final.push_back(s);
            }
            in.close();
            if (!pareto_final.empty()) {
                auto normPop = normalizePopulation(pareto_final, RHO);
                double hv = 0.0;
                for (int i = 0; i < (int)normPop.size(); ++i) {
                    hv += calcHypervolumeIndicator(i, -1, 3, normPop);
                }
                hv_total += hv;
            }
        }
        cout << hv_total << endl;
        return 0;
    }

    // Inicializa o EP (Elite Population)
    BoundedParetoSet *EP = new BoundedParetoSet();

    // >> Passo 1: Leitura da instância
    InputData data = readInput(path_in);

    // Realizar execuções independentes apenas para índices faltantes
    for (int exec = 0; exec < NUM_EXECUTIONS; exec++) {
        if (exec_exists[exec]) continue; // Pular execuções já existentes
        int exec_index = exec + 1;
        string execution_folder = base_dir + "/exec_" + to_string(exec_index);
        filesystem::create_directories(execution_folder);

        // Nova semente para cada execução
        unsigned int exec_seed = static_cast<unsigned int>(time(nullptr) + exec_index);
        srand(exec_seed);

        // Reinicializa estruturas para esta execução
        delete EP;
        EP = new BoundedParetoSet();

        // Estrutura para salvar gerações
        Generations saved_generations;
        int biggest_multiple = 0;

        // Reinicializa variáveis de tempo
        std::chrono::duration<double> tempoTotal = std::chrono::duration<double>::zero();
        auto start_time = std::chrono::high_resolution_clock::now();

        // >> Passo 2: Inicialização da população
        vector<Solution> population = get_random_population(data, EP, POP_SIZE);
        int eval = population.size();  // Contador de avaliações

        // Salva estado inicial
        save_data_routine(saved_generations, EP, start_time, biggest_multiple, eval, tempoTotal);

        // >> Loop principal do mIBEA
        while (eval < MAX_EVAL) {
            // **Passo 3**: Filtra população para não-dominados
            population = getNonDominated(population);

            // **Passo 4**: Normaliza cada solução
            auto normPop = normalizePopulation(population, RHO);

            // **Passo 5**: Calcula fitness
            auto fitnessValues = computeFitness(normPop, KAPPA);
            for (size_t i = 0; i < population.size(); ++i)
                population[i].fitness = fitnessValues[i];

            // **Passo 6**: Seleção ambiental
            environmentalSelection(population, fitnessValues, POP_SIZE);

            // **Passo 7**: Torneio binário para pool de reprodução
            vector<Solution> matingPool;
            matingPool.reserve(POP_SIZE);
            for (int i = 0; i < POP_SIZE; ++i)
                matingPool.push_back(population[tournamentSelect(population, TOURNAM)]);

            // **Passo 8**: Gera offspring
            auto offspring = generateOffspring(data, matingPool, CROSSOVER_RATE, MUTATION_RATE);
            
            // **Passo 9**: Atualiza população e EP
            for (auto &child : offspring) {
                if (solution_validity(data, child)) {
                    population.push_back(child);
                    EP->adicionarSol(&child);
                }
            }
            
            eval += offspring.size();

            // Verifica se deve salvar o estado atual (a cada 10000 avaliações)
            // Calculamos o próximo ponto de salvamento
            int next_checkpoint = (biggest_multiple + 1) * 10000;
            
            // Verificamos se já passamos desse ponto
            if (eval >= next_checkpoint) {
                save_data_routine(saved_generations, EP, start_time, biggest_multiple, eval, tempoTotal);
            }
        }

        // Garantir que temos exatamente 9 arquivos (0-8)
        
        // Se temos menos de 9 gerações, forçamos o salvamento das que faltam
        while (saved_generations.generations.size() < 9) {
            save_data_routine(saved_generations, EP, start_time, biggest_multiple, MAX_EVAL, tempoTotal);
        }
        
        // Se temos mais de 9, removemos as excedentes (improvável, mas por segurança)
        while (saved_generations.generations.size() > 9) {
            saved_generations.generations.pop_back();
            saved_generations.durations.pop_back();
        }

        // Cria arquivos para esta execução
        for(int generation = 0; generation < saved_generations.generations.size(); generation++) {
            string arquivo_pareto = execution_folder + "/paretogeracao_" + to_string(generation) + ".txt";
            string arquivo_completo = execution_folder + "/paretogeracao_" + to_string(generation) + "_completo.txt";
            string arquivo_dados = execution_folder + "/dados";
            
            
            ofstream pareto_file(arquivo_pareto);
            if (!pareto_file.is_open()) {
                cerr << "Erro ao abrir arquivo: " << arquivo_pareto << endl;
                continue;
            }
            
            ofstream complete_file(arquivo_completo);
            if (!complete_file.is_open()) {
                cerr << "Erro ao abrir arquivo: " << arquivo_completo << endl;
                continue;
            }
            
            ofstream data_file(arquivo_dados);
            if (!data_file.is_open()) {
                cerr << "Erro ao abrir arquivo: " << arquivo_dados << endl;
                continue;
            }
            
            // Escrever dados do pareto
            for(int pareto_set = 0; pareto_set < saved_generations.generations[generation].pareto_set.size(); pareto_set++) {
                pareto_file << saved_generations.generations[generation].pareto_set[pareto_set].cost << " " 
                       << saved_generations.generations[generation].pareto_set[pareto_set].time << " " 
                       << saved_generations.generations[generation].pareto_set[pareto_set].total_bonus;
                
                fill_complete_archive(data, complete_file, saved_generations, generation, pareto_set);
                
                if(pareto_set < saved_generations.generations[generation].pareto_set.size() - 1) {
                    pareto_file << endl;
                    complete_file << endl;
                }
            }
            
            // Escrever dados de tempo
            data_file << "seed: " << exec_seed << endl;
            data_file << "tempo total em segundos: " << tempoTotal.count() << endl;
            for(int duration = 0; duration < saved_generations.durations.size(); duration++) {
                data_file << "tempo da geração " << duration << ":" << endl 
                         << saved_generations.durations[duration].count() << endl;
            }
            
            pareto_file.close();
            complete_file.close();
            data_file.close();
        }
        
        // Calcular e imprimir hipervolume do Pareto final desta execução
        
        auto pareto_final = get_pareto_objectives(EP);
      Pareto_objectives sets_of_objectives = get_pareto_objectives(EP); //Salva nesse struct os objetivos das soluções contidas no EP
      delete EP;
      
      create_archive_IRACE(sets_of_objectives); //cria arquivo nos moldes do que o irace precisa
      std::ofstream arquivo("iracehyp.txt", std::ios::out); //daqui pra baixo chama o hypervolume, calcula, e dá print no terminal.
      arquivo.close();
      string comando;
      comando = "hyp.exe hyp_ind_param_irace.txt testeirace.txt testeirace.txt iracehyp.txt";
      int result = system(comando.c_str());
      std::ifstream arquivoirace("iracehyp.txt");
      std::string linha;
      while (std::getline(arquivoirace, linha)) {
          double linha2 = stof(linha);
          cout <<linha2;
      }
      arquivoirace.close();
        
    }

    // cout << hv_total << endl;

    delete EP;
    return 0;
}

