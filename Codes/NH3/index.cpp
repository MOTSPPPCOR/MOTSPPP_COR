#include "gurobi_c++.h"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <limits>
#include <vector>
#include <string>
#include <tuple>
#include <set>
#include <cmath>
#include <chrono>
#include <numeric>

using namespace std;

int s_initial = 0;

// Classe de callback para limitar o tempo de execução
class TimeLimitCallback : public GRBCallback {
public:
    std::chrono::steady_clock::time_point start_time;
    int time_limit_seconds;
    bool stop_optimization = false;

    TimeLimitCallback(int time_limit) : time_limit_seconds(time_limit) {
        start_time = std::chrono::steady_clock::now();
    }

protected:
    void callback() override {
        if (where == GRB_CB_MIP || where == GRB_CB_MIPSOL || where == GRB_CB_MIPNODE) {
            auto current_time = std::chrono::steady_clock::now();
            std::chrono::duration<double> elapsed_seconds = current_time - start_time;

            if (elapsed_seconds.count() >= time_limit_seconds) {
                stop_optimization = true;
                abort();  // Interrompe a otimização
            }
        }
    }
};

struct Passenger {
    int wk;  // Contribuição do passageiro
    int ok;  // Cidade de origem do passageiro
    int dk;  // Cidade de destino do passageiro
    int tk;  // Tempo máximo da rota do passageiro
};

struct InputData {
    int n;                  // Número de cidades
    int l;                  // Número de passageiros
    int vehicle_capacity;   // Capacidade do veículo
    vector<vector<int>> c;  // Matriz de custosnObjectives
    vector<vector<int>> y;  // Matriz de tempo
    vector<int> b;          // Bônus de cada cidade
    vector<int> g;          // Tempo de coleta de bônus de cada cidade
    vector<Passenger> passengers; // Dados dos passageiros
};

// Estrutura para armazenar os pontos não-dominados
struct Solution {
    double obj_cost;
    double obj_time;
    double obj_bonus;
    vector<int> tour;
    vector<int> collectedBonuses;
    vector<int> passengers;
};

// leitura dos dados do 'input.txt'
InputData readInput(const string &filename) {
    ifstream file(filename);
    InputData data;

    if (file.is_open()) {
        int quota;
        file >> data.n >> data.l >> data.vehicle_capacity;

        data.c.resize(data.n, vector<int>(data.n));
        data.y.resize(data.n, vector<int>(data.n));
        data.b.resize(data.n);
        data.g.resize(data.n);
        data.passengers.resize(data.l);

        for (int i = 0; i < data.n; i++) {
            for (int j = 0; j < data.n; j++) {
                file >> data.c[i][j];
            }
        }

        for (int i = 0; i < data.n; i++) {
            for (int j = 0; j < data.n; j++) {
                file >> data.y[i][j];
            }
        }

        for (int i = 0; i < data.l; i++) {
            int wk, ok, dk, tk;
            file >> wk >> ok >> dk >> tk;
            data.passengers[i] = {wk, ok, dk, tk};
        }

        file >> quota; // não utilizada;

        for (int i = 0; i < data.n; i++) {
            int cityIndex, bonus, collectTime;
            file >> cityIndex >> bonus >> collectTime;
            data.b[cityIndex] = bonus;
            data.g[cityIndex] = collectTime;
        }

        file.close();
    } else {
        cerr << "Não foi possível abrir o arquivo " << filename << endl;
    }

    return data;
}

// Função para extrair o tour da solução com base na variável x
vector<int> extractTour(GRBModel &model, vector<vector<GRBVar>> &x, int n) {
    vector<int> tour;

    try {
        vector<bool> visited(n, false);
        int current = 0;  // Cidade inicial
        tour.push_back(current);
        visited[current] = true;

        while (true) {
            bool found = false;

            // Busca a próxima cidade no tour
            for (int next = 0; next < n; next++) {
                if (!visited[next] && x[current][next].get(GRB_DoubleAttr_X) > 0.5) {
                    tour.push_back(next);
                    visited[next] = true;
                    current = next;
                    found = true;
                    break;
                }
            }

            // Se não há mais cidades acessíveis, encerra o loop
            if (!found) {
                break;
            }
        }

    } catch (const GRBException &e) {
        cerr << "Erro: " << e.getMessage() << endl;
        cerr << "Código do erro: " << e.getErrorCode() << endl;
    } catch (const exception &e) {
        cerr << "Exceção: " << e.what() << endl;
    } catch (...) {
        cerr << "Exceção desconhecida." << endl;
    }

    // Retorna o tour gerado
    tour.push_back(0);
    return tour;
}

// Função para coletar bônus ao longo do tour
vector<int> collectBonuses(const vector<int>& tour, const InputData &data, GRBModel &model,  vector<GRBVar> &p) {
    vector<int> collectedBonuses;

    try {
        for (int city : tour) {
            //if (p[city].get(GRB_DoubleAttr_X) > .5) {
            if (fabs(p[city].get(GRB_DoubleAttr_X)-1.0) < 0.000001) {
                collectedBonuses.push_back(city);
            }
        }
    } catch (const GRBException &e) {
        cerr << "Erro: " << e.getMessage() << endl;
        cerr << "Código do erro: " << e.getErrorCode() << endl;
    } catch (const exception &e) {
        cerr << "Exceção: " << e.what() << endl;
    } catch (...) {
        cerr << "Exceção desconhecida." << endl;
    }

    return collectedBonuses;
}

// Função para coletar os passageiros viáveis
vector<int> collectPassengers(GRBModel &model, vector<vector<vector<GRBVar>>> &h, int n, int m) {
    vector<int> passengers;

    try {
        for (int k = 0; k < m; k++) {
            bool collected = false;
            for (int i = 0; i < n; i++) {
                for (int j = 0; j < n; j++) {
                    if (h[k][i][j].get(GRB_DoubleAttr_X) > 0.5) {
                        passengers.push_back(k);
                        collected = true;
                        break;
                    }
                }
                if (collected) break;
            }
        }
    } catch (const GRBException &e) {
        cerr << "Erro: " << e.getMessage() << endl;
        cerr << "Código do erro: " << e.getErrorCode() << endl;
    } catch (const exception &e) {
        cerr << "Exceção: " << e.what() << endl;
    } catch (...) {
        cerr << "Exceção desconhecida." << endl;
    }
    return passengers;
}

// Função para salvar os resultados intermediários
void saveProgress(const vector<Solution> &solutions, const string &path_output, const string &file_name_output) {
    ofstream outFile(path_output + file_name_output);
    if (outFile.is_open()) {
        outFile << "Fo(custo)\tFo(tempo)\tFo(bônus)\n";
        outFile << endl;

        for (const auto &solution : solutions) {
            outFile << fabs(solution.obj_cost) << "\t" << fabs(solution.obj_time) << "\t" << solution.obj_bonus << "\n";
            outFile << "tour: ";
            for (int city : solution.tour) {
                outFile << city << " ";
            }
            outFile << "\n";
            outFile << "bonus: ";
            for (int bonus : solution.collectedBonuses) {
                outFile << bonus << " ";
            }
            outFile << "\n";
            outFile << "passengers: ";
            for (int passenger : solution.passengers) {
                outFile << passenger << " ";
            }
            outFile << "\n\n";
        }
        outFile.close();
        cout << "Resultados intermediários gravados!" << endl;
    } else {
        cerr << "Não foi possível abrir o arquivo de resultados intermediários." << endl;
    }
}

// Função para configurar e resolver o modelo Gurobi
void solveModel(GRBEnv &env, const InputData &data, vector<Solution> &solutions) {
    try {
        bool stop = false;
        int count_interaction = 0;

        int time_limit_seconds = 7200;  // 2 horas em segundos
        TimeLimitCallback cb(time_limit_seconds);
        
        auto start_time = std::chrono::steady_clock::now();

        while(!stop) {
            GRBModel model = GRBModel(env);
            model.setCallback(&cb);
            model.set(GRB_DoubleParam_MIPGap, 1e-9);
            model.set(GRB_DoubleParam_MIPGapAbs, 1e-9);


            int n = data.n;
            int m = data.l;
            double M = 1000000;//1e7;//1e6;
            double epsilon = 0.001;//0.001;//1e-7;//0.0000001; // 10^-9
            int nObjectives = 2;

            // +----------------------+
            // + Variáveis de decisão +
            // +----------------------+
            
            vector<vector<GRBVar>> x(n, vector<GRBVar>(n));                                      // Variável binária que indica se a aresta de cidade i para cidade j é percorrida;
            vector<GRBVar> u(n);                                                                 // Variável inteira que indica a ordem da cidade i na rota do caixeiro viajante, usada nas restrições MTZ para eliminação de subtours
            vector<GRBVar> p(n);                                                                 // Variável binária que indica se o bônus na cidade i é coletado
            // vector<vector<vector<GRBVar>>> h(m, vector<vector<GRBVar>>(n, vector<GRBVar>(n)));   // Variável binária que indica se o k-ésimo passageiro está no veículo na aresta (i, j)
            // vector<vector<vector<GRBVar>>> psi(n, vector<vector<GRBVar>>(n, vector<GRBVar>(m))); // Variável continua para linearização das restrições envolvendo alpha // by Felipe 28/08/2024
            // vector<vector<GRBVar>> alpha(n, vector<GRBVar>(n));                                  // Variável contínua que indica o inverso do número de ocupantes do carro na aresta (i, j);
            vector<vector<GRBVar>> y(solutions.size(), vector<GRBVar>(nObjectives));             // Ajuste o tamanho conforme necessário

            for (int i = 0; i < n; i++) {
                p[i] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, "p_" + to_string(i));
                
                for (int j = 0; j < n; j++) {
                    if (i != j) {
                        x[i][j] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, "x_" + to_string(i) + "_" + to_string(j));
                        // alpha[i][j] = model.addVar(0.0, 1.0, 0.0, GRB_CONTINUOUS, "alpha_" + to_string(i) + "_" + to_string(j));
                    }
                }
            }
            
            for (int i = 0; i < n; i++) {
                u[i] = model.addVar(1.0, n - 1, 0.0, GRB_INTEGER, "u_" + to_string(i));
            }

            // for (int k = 0; k < m; k++){
            //     for (int i = 0; i < n; i++){
            //         for (int j = 0; j < n; j++){
            //             // h[k][i][j] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, "h_" + to_string(k) + "_" + to_string(i) + "_" + to_string(j));
            //             // psi[i][j][k] = model.addVar(0, 1, 0.0, GRB_CONTINUOUS, "psi" + std::to_string(i) + std::to_string(j) + std::to_string(k)); // by Felipe (28/08/2024)

            //         }
            //     }
            // }

            for (int t = 0; t < solutions.size(); t++) {
                for (int j = 0; j < nObjectives; j++) {
                    y[t][j] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, "y_" + to_string(t) + "_" + to_string(j));
                }
            }


            // +-----------------+
            // + Funcao objetivo +
            // +-----------------+

            GRBLinExpr obj_cost = 0;
            GRBLinExpr obj_bonus = 0;
            for (int i = 0; i < n; i++) {
                for (int j = 0; j < n; j++) {
                    if (i != j) {
                        // obj_cost -= x[i][j] * data.c[i][j] * alpha[i][j];
                        obj_cost -= x[i][j] * data.c[i][j];
                        // obj_time -= data.y[i][j] * x[i][j];
                    }
                }
                // obj_time -= p[i] * data.g[i];
                obj_bonus += p[i] * data.b[i];
            }

            // model.setObjective(obj_bonus + (epsilon * obj_cost) + (epsilon * obj_time), GRB_MAXIMIZE);
            model.setObjective(obj_bonus + (epsilon * obj_cost), GRB_MAXIMIZE);

            

            // +------------------------------+
            // + Restrições Lokman e Koksalan +
            // +------------------------------+

           for (int t = 0; t < solutions.size(); t++) {
                for (int j = 0; j < nObjectives; j++) {
                    if (j != 1) {
                        model.addConstr(obj_cost >= (solutions[t].obj_cost + 1) * y[t][0] - (M * (1 - y[t][0])), "comp1_" + to_string(t) + "_" + to_string(0));
                    }
                }
            }
            for (int t = 0; t < solutions.size(); t++) {
                GRBLinExpr sumY = 0;
                for (int j = 0; j < nObjectives; j++){
                    if (j != 1) {
                        sumY += y[t][j];
                    }
                }
                model.addConstr(sumY == 1, "compSum_" + to_string(t));
            }

            // +------------------------+
            // + Restrições do problema +
            // +------------------------+

            // Restrição 4: Cada cidade, exceto a origem, deve ser visitada exatamente uma vez
            // Restrição 5: Apenas uma saída de cada cidade
            GRBLinExpr expr4 = 0;
            GRBLinExpr expr5 = 0;
            for (int i = 0; i < n; i++){
                if (i != s_initial){
                    expr4 += x[s_initial][i];
                    expr5 += x[i][s_initial];
                }
            }
            model.addConstr(expr4 == 1, "visit_" + to_string(s_initial));
            model.addConstr(expr5 == 1, "departure_" + to_string(s_initial));

            // Restrição 6: Cada cidade deve ser visitada no máximo uma vez
            // Restrição 7: Somatório de entradas na cidade j deve ser no máximo 1
            for (int j = 0; j < n; j++) {
                if (j != s_initial) {
                    GRBLinExpr expr6 = 0;
                    GRBLinExpr expr7 = 0;
                    for (int i = 0; i < n; i++) {
                        if (i != j) {
                            expr6 += x[i][j];
                            expr7 += x[j][i];
                        }
                    }
                    model.addConstr(expr6 <= 1, "visit_once_" + to_string(j));
                    model.addConstr(expr7 <= 1, "entry_once_" + to_string(j));
                }
            }

            // Restrição 8: Fluxo de entrada igual ao fluxo de saída
            for (int j = 0; j < n; j++){
                if (j != s_initial) {
                    GRBLinExpr expr81 = 0;
                    GRBLinExpr expr82 = 0;
                    for (int i = 0; i < n; i++) {
                        if (i != j) {
                            expr81 += x[i][j];
                            expr82 += x[j][i];
                        }
                    }
                    model.addConstr(expr81 - expr82 == 0, "flow_balance_" + to_string(j));
                }
            }

            // Restrição 9: Restrições MTZ para eliminação de subtours
            for (int i = 1; i < n; i++) {
                for (int j = 1; j < n; j++) {
                    if (i != j) {
                        model.addConstr(u[i] - u[j] + (n - 1) * x[i][j] <= n - 2, "mtz_" + to_string(i) + "_" + to_string(j));
                    }
                }
            }

            // Restrição 10: Somatório de x <= p para todas as cidades j exceto a cidade de origem
            for (int i = 0; i < n; i++){
                GRBLinExpr expr10 = 0;
                for (int j = 0; j < n; j++){
                    if (j != i){
                        expr10 += x[i][j];
                    }
                }
                model.addConstr(expr10 >= p[i], "max_visits_" + to_string(i));   
            }

            model.optimize();
            
           // Verificar viabilidade e ignorar se o tempo foi excedido
            if (model.get(GRB_IntAttr_Status) == GRB_OPTIMAL && !cb.stop_optimization) {
                Solution newSolution = {
                    obj_cost.getValue(),
                    0, // obj_time.getValue(), // => coletar o time
                    obj_bonus.getValue(),
                    extractTour(model, x, n),
                    collectBonuses(newSolution.tour, data, model, p),
                    // collectPassengers(model, h, n, m)  // => coletar os passageiros possiveis
                };

                solutions.push_back(newSolution);

            } else if (cb.stop_optimization) {
                cout << "Otimização interrompida devido ao limite de tempo. Solução ignorada." << endl;
                stop = cb.stop_optimization;
            } else if (model.get(GRB_IntAttr_Status) == GRB_INFEASIBLE) {
                cout << "Otimização finalizada!" << endl;
                stop = true;
            }

       }

    } catch (GRBException &e) {
        cerr << "Error code = " << e.getErrorCode() << endl;
        cerr << e.getMessage() << endl;
    } catch (...) {
        cerr << "Exception during optimization" << endl;
    }
}

// Função para calcular o tempo total da viagem de origem a destino no tour
int calculateTravelTime(const InputData &data, const Solution &solution, int origin, int destination) {
    int travelTime = 0;
    bool inRoute = false;
    bool originBonus = false;
    int tuorLength = solution.tour.size();


    for (int i = 0; i < tuorLength; i++) {
        int currentCity = solution.tour[i];
        int nextCity = i == tuorLength - 1 ? solution.tour[0] : solution.tour[i + 1];
        if (currentCity == origin) {
            originBonus = true;
            inRoute = true;
        }
        if (inRoute) {
            if (originBonus) {
                travelTime += data.g[currentCity];
                originBonus = false;
            }

            travelTime += data.y[currentCity][nextCity];
            travelTime += data.g[nextCity]; // Adiciona o tempo de coleta do bônus

            
            if (nextCity == destination) {
                break;
            }
        }

    }
    return travelTime;
}

double calculateTotalPassengerCost(const InputData &data, const Solution &solution, vector<int> passengers) {
    double totalPassengerCost = 0.0;
    int currentCapacity = 1; // Começa com apenas o caixeiro

    for (size_t i = 0; i < solution.tour.size() - 1; ++i) {
        int currentCity = solution.tour[i];
        int nextCity = solution.tour[i + 1];

        // Verifica se passageiros embarcam na cidadpassengerse atual
        for (int k : passengers) {
            if (currentCity == data.passengers[k].ok) {
                currentCapacity++;
            }
        }

        // Verifica se passageiros desembarcam na cidade atual
        for (int k : passengers) {
            if (currentCity == data.passengers[k].dk) {
                currentCapacity--;
            }
        }

        // Adiciona custo atual dividido pela capacidade atual
        totalPassengerCost += static_cast<double>(data.c[currentCity][nextCity]) / currentCapacity;

    }

    return totalPassengerCost;
}

// Heurística de Correspondência de Viagens (Ride-Matching Heuristic)
void rideMatchingHeuristic(const InputData &data, Solution &solution) {
    Solution bestSolution;  // Criar uma cópia da solução inicial
    bestSolution.obj_cost = fabs(solution.obj_cost);
    bestSolution.obj_time = solution.obj_time;
    bestSolution.obj_bonus = solution.obj_bonus;
    bestSolution.collectedBonuses = solution.collectedBonuses;
    bestSolution.passengers = solution.passengers;
    bestSolution.tour = solution.tour;

    // Passo 2: Inicializar a lista de passageiros ordenada
    vector<int> sortedPassengers(data.passengers.size());
    iota(sortedPassengers.begin(), sortedPassengers.end(), 0);
    sort(sortedPassengers.begin(), sortedPassengers.end(), [&data](int a, int b) {
        return data.passengers[a].wk > data.passengers[b].wk;
    });

    vector<bool> passengerAssigned(data.passengers.size(), false);
    vector<int> onboardPassengers;

    // Passo 3: Verificação de tour e tempo
    for (int j : sortedPassengers) {
        const Passenger &p = data.passengers[j];
        bool embarkFound = false;
        bool disembarkFound = false;
        int embarkIndex = -1;
        int disembarkIndex = -1;

        // Verifica se o passageiro tem seu vértice de embarque e desembarque visitados
        for (int i = 0; i < bestSolution.tour.size(); i++) {
            int city = bestSolution.tour[i];

            if (city == p.ok) {
                embarkFound = true;
                embarkIndex = i;
            }
            if (city == p.dk) {
                disembarkFound = true;
                disembarkIndex = i;
            }

            // Se ambos vértices de embarque e desembarque são encontrados, interrompe a busca
            if (embarkFound && disembarkFound) {
                break;
            }
        }

        // Se ambos vértices de embarque e desembarque são visitados e embarque ocorre antes do desembarque
        if (embarkFound && disembarkFound && embarkIndex < disembarkIndex) {
            int travelTime = calculateTravelTime(data, bestSolution, p.ok, p.dk);

            if (travelTime <= p.tk) {
                // Passageiro potencialmente viável, adicionar para verificação de custo
                bestSolution.passengers.push_back(j);
            }
        }
    }


    // Passo 4: Verificação de custo e ajuste da capacidade do veículo
    vector<int> finalPassengers;
    int lotacao = 0;

    for (int i = 0; i < bestSolution.tour.size(); i++) {
        int currentCity = bestSolution.tour[i];

        // verificar desembarque
        for (auto it = finalPassengers.begin(); it != finalPassengers.end(); it++) {
            const Passenger &p = data.passengers[*it];
            if(currentCity == p.dk) {
                lotacao--;
            }
        }

        // verificar o embarque
        for (int j : bestSolution.passengers) {
            const Passenger &p = data.passengers[j];

            if (currentCity == p.ok && lotacao < data.vehicle_capacity) {
                // calcular o custo do trajeto do passageiro
                int travelCost = 0;
                int embarkIndex = i;
                int disembarkIndex = -1;

                for (int k = i; k < bestSolution.tour.size(); k++) {
                    if (bestSolution.tour[k] == p.dk) {
                        disembarkIndex = k;
                        break;
                    }
                }

                if (disembarkIndex != -1) {
                    for (int k = embarkIndex + 1; k < disembarkIndex; k++) {
                        travelCost += data.c[bestSolution.tour[k]][bestSolution.tour[k+1]];
                    }

                    //dividir o custo pela lotacao atual (motorista + passageiros)
                    double costPerPassenger = static_cast<double>(travelCost) / (lotacao + 1);

                    if (costPerPassenger <= p.wk) {
                        finalPassengers.push_back(j);
                        lotacao++;
                    }
                }
            }
        }
    }

   
    // Atualiza a solução original com a lista final de passageiros
    bestSolution.passengers = finalPassengers;
    bestSolution.obj_cost = calculateTotalPassengerCost(data, solution, bestSolution.passengers);
    solution = bestSolution;
}

// Função para calcular o tempo total do percurso do tour, incluindo o tempo de coleta de bônus
void calculateTotalTime(const InputData &data, Solution &solution) {
    double totalTime = 0;
    Solution bestSolution = solution;

    // for (size_t i = 0; i < solution.collectedBonuses.size(); i++) {
    for(int bonus : solution.collectedBonuses) {
        totalTime += data.g[bonus];
    };

    // Percorre o tour para somar o tempo de viagem e o tempo de coleta de bônus
    for (size_t i = 0; i < bestSolution.tour.size() - 1; ++i) {
        int currentCity = bestSolution.tour[i];
        int nextCity = bestSolution.tour[i + 1];

        // Adiciona o tempo de viagem entre as cidades consecutivas no tour
        totalTime += data.y[currentCity][nextCity];
    }

    // Adiciona o tempo de retorno ao ponto inicial, se necessário
    if (!bestSolution.tour.empty()) {
        int lastCity = bestSolution.tour.back();
        int startCity = bestSolution.tour.front();
        totalTime += data.y[lastCity][startCity];
    }

    bestSolution.obj_time = totalTime;
    solution = bestSolution;
}

bool isDominated(const Solution &sol1, const Solution &sol2) {
    // sol1 é dominado por sol2 se e somente se sol2 é melhor em pelo menos um objetivo
    // e não é pior em nenhum outro objetivo.
    bool isBetterInAtLeastOne = false;
    
    if (sol2.obj_cost <= sol1.obj_cost && sol2.obj_time <= sol1.obj_time && sol2.obj_bonus >= sol1.obj_bonus) {
        if (sol2.obj_cost < sol1.obj_cost || sol2.obj_time < sol1.obj_time || sol2.obj_bonus > sol1.obj_bonus) {
            isBetterInAtLeastOne = true;
        }
        return isBetterInAtLeastOne;
    }
    return false;
}

void filterDominatedSolutions(vector<Solution> &solutions) {
    vector<Solution> nonDominatedSolutions;
    
    for (int i = 0; i < solutions.size(); i++) {
        bool dominated = false;
        for (int j = 0; j < solutions.size(); j++) {
            if (i != j && isDominated(solutions[i], solutions[j])) {
                dominated = true;
                break;
            }
        }
        if (!dominated) {
            nonDominatedSolutions.push_back(solutions[i]);
        }
    }
    
    solutions = nonDominatedSolutions;
}


int main() {
    vector<string> typeInstancies = { "asymmetric", "symmetric" };
    vector<string> instances = {
        "5.1", "5.2", "5.3", "5.4", "5.5", "5.6",
        "8.1", "8.2", "8.3", "8.4", "8.5", "8.6",
        "10.1", "10.2", "10.3", "10.4", "10.5", "10.6",
        "20.1", "20.2", "20.3", "20.4", "20.5", "20.6",
        "50.1", "50.2", "50.3", "50.4", "50.5", "50.6",
        "100.1", "100.2", "100.3", "100.4", "100.5", "100.6",
        "200.1", "200.2", "200.3", "200.4", "200.5", "200.6",
    };

    for(string type : typeInstancies) {
        string outputFileType = "asym";
        if (type == "symmetric") { outputFileType = "sym"; };

        for(string instance : instances)
        {
            auto start_time = chrono::high_resolution_clock::now();

            string path_input = "../../instances/A3/"+type+"/"+instance+".in";
            string path_output = "../../resultados/naive_v3/a3/"+outputFileType+"/";
            string path_log = "../../logs/naive_v3/a3/"+outputFileType+"/";
            string file_name_output = instance+".txt";
            string file_log_name_output = instance+".log";
            string file_log_time_output = "time."+file_log_name_output;


            InputData data = readInput(path_input);

            GRBEnv env = GRBEnv(true);
            string log_file = path_log + file_log_name_output;
            env.set("LogFile", log_file);
            env.start();

            vector<Solution> solutions;

            solveModel(env, data, solutions);

            for (int i = 0; i < solutions.size(); ++i) {
                calculateTotalTime(data, solutions[i]);
                rideMatchingHeuristic(data, solutions[i]);
            }

            filterDominatedSolutions(solutions);


            ofstream outTimeLog(path_log + file_log_time_output);
            auto end_time = chrono::high_resolution_clock::now();
            chrono::duration<double> execution_time = end_time - start_time;

            outTimeLog << "Tempo de execução: " << execution_time.count() << " segundos\n";


            if (execution_time.count() >= 7200.0) {
                file_name_output = instance + ".2h.txt";
            }

            // Escrever resultados em solutions.txt
            ofstream outFile(path_output + file_name_output);
            if (outFile.is_open()) {
                outFile << "Fo(custo)\tFo(tempo)\tFo(bônus)\n";
                outFile << endl;

                for (const auto &solution : solutions) {
                    outFile << fabs(solution.obj_cost) << "\t" << fabs(solution.obj_time) << "\t" << solution.obj_bonus << "\n";
                    outFile << "tour: ";
                    for (int city : solution.tour) {
                        outFile << city << " ";
                    }
                    outFile << "\n";
                    outFile << "bonus: ";
                    for (int bonus : solution.collectedBonuses) {
                        outFile << bonus << " ";
                    }
                    outFile << "\n";
                    outFile << "passengers: ";
                    for (int passenger : solution.passengers) {
                        outFile << passenger << " ";
                    }
                    outFile << "\n\n";
                }
                outFile.close();
            } else {
                cerr << "Unable to open file solutions.txt";
            }
            
            remove("mip1.log");
        }

    }
    return 0;
}