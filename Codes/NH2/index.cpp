#include "gurobi_c++.h"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <limits>
#include <vector>
#include <string>
#include <tuple>
#include <set>
#include <numeric>
#include <chrono>

using namespace std;

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
                abort();
            }
        }
    }
};

struct Passenger {
    int wk;
    int ok;
    int dk;
    int tk;
};

struct InputData {
    int n;
    int k;
    int vehicle_capacity;
    vector<vector<int>> c;
    vector<vector<int>> y;
    vector<int> b;
    vector<int> t;
    vector<Passenger> passengers;
};

struct Solution {
    double obj1;
    double obj2;
    vector<int> tour;
    vector<int> collectedBonuses;
    int bonusSum;
    vector<int> passengers;
};

InputData readInput(const string &filename) {
    ifstream file(filename);
    InputData data;

    if (file.is_open()) {
        int quota;
        file >> data.n >> data.k >> data.vehicle_capacity;

        data.c.resize(data.n, vector<int>(data.n));
        data.y.resize(data.n, vector<int>(data.n));
        data.b.resize(data.n);
        data.t.resize(data.n);
        data.passengers.resize(data.k);

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

        for (int i = 0; i < data.k; i++) {
            int wk, ok, dk, tk;
            file >> wk >> ok >> dk >> tk;
            data.passengers[i] = {wk, ok, dk, tk};
        }

        file >> quota;

        for (int i = 0; i < data.n; i++) {
            int cityIndex, bonus, collectTime;
            file >> cityIndex >> bonus >> collectTime;
            data.b[cityIndex] = bonus;
            data.t[cityIndex] = collectTime;
        }

        file.close();
    } else {
        cerr << "Could not open  file" << filename << endl;
    }

    return data;
}

vector<int> extractTour(GRBModel &model, const vector<vector<GRBVar>> &x, int n) {
    vector<int> tour;
    vector<bool> visited(n, false);
    int current = 0;
    tour.push_back(current);
    visited[current] = true;

    while (tour.size() < n) {
        bool found = false;
        for (int next = 0; next < n; next++) {
            if (!visited[next] && model.getVarByName("x_" + to_string(current) + "_" + to_string(next)).get(GRB_DoubleAttr_X) > 0.5) {
                tour.push_back(next);
                visited[next] = true;
                current = next;
                found = true;
                break;
            }
        }
        if (!found) {
            cerr << "Error: Tour incomplete or invalid." << endl;
            return tour;
        }
    }

    return tour;
}

int calculateTravelTime(const InputData &data, const Solution &solution, int origin, int destination) {
    int travelTime = 0;
    bool inRoute = false;
    int tuorLength = solution.tour.size();

    for (int i = 0; i < tuorLength; i++) {
        int currentCity = solution.tour[i];
        int nextCity = i == tuorLength - 1 ? solution.tour[0] : solution.tour[i + 1];
        if (currentCity == origin) {
            inRoute = true;
        }
        if (inRoute) {
            travelTime += data.y[currentCity][nextCity];
            
            if (nextCity == destination) {
                break;
            }
        }

    }
    return travelTime;
}

int calculateTravelCost(const InputData &data, const Solution &solution, int origin, int destination, const vector<int>& onboardPassengers) {
    int travelCost = 0;
    bool inRoute = false;
    int numPassengers = onboardPassengers.size() + 1;
    for (size_t i = 0; i < solution.tour.size() - 1; ++i) {
        int currentCity = solution.tour[i];
        int nextCity = solution.tour[i + 1];
        if (currentCity == origin) {
            inRoute = true;
        }
        if (inRoute) {
            travelCost += data.c[currentCity][nextCity] / numPassengers;
            if (nextCity == destination) {
                break;
            }
        }
    }
    return travelCost;
}

double calculateTotalPassengerCost(const InputData &data, const Solution &solution, const vector<int> &passengers) {
    double totalPassengerCost = 0;
    int currentCapacity = 1;

    for (size_t i = 0; i < solution.tour.size() - 1; ++i) {
        int currentCity = solution.tour[i];
        int nextCity = solution.tour[i + 1];

        double cost = static_cast<double>(data.c[currentCity][nextCity]) / currentCapacity;

        if (currentCapacity > 1) {
            totalPassengerCost += cost * (currentCapacity - 1);
        }

        for (int k : passengers) {
            if (solution.tour[i + 1] == data.passengers[k].dk) {
                currentCapacity--;
            }
        }

        for (int k : passengers) {
            if (solution.tour[i + 1] == data.passengers[k].ok) {
                currentCapacity++;
            }
        }
    }

    return totalPassengerCost;
}

void rideMatchingHeuristic(const InputData &data, Solution &solution) {
    Solution bestSolution = solution;
    bestSolution.tour.push_back(0);

    vector<int> sortedPassengers(data.passengers.size());
    iota(sortedPassengers.begin(), sortedPassengers.end(), 0);
    sort(sortedPassengers.begin(), sortedPassengers.end(), [&data](int a, int b) {
        return data.passengers[a].wk > data.passengers[b].wk;
    });

    vector<bool> passengerAssigned(data.passengers.size(), false);
    vector<int> onboardPassengers;

    for (int j : sortedPassengers) {
        const Passenger &p = data.passengers[j];
        bool embarkFound = false;
        bool disembarkFound = false;
        int embarkIndex = -1;
        int disembarkIndex = -1;

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

            if (embarkFound && disembarkFound) {
                break;
            }
        }

        if (embarkFound && disembarkFound && embarkIndex < disembarkIndex) {
            int travelTime = calculateTravelTime(data, bestSolution, p.ok, p.dk);

            if (travelTime <= p.tk) {
                bestSolution.passengers.push_back(j);
            }
        }
    }


    vector<int> finalPassengers;
    int lotacao = 0;

    for (int i = 0; i < bestSolution.tour.size(); i++) {
        int currentCity = bestSolution.tour[i];

        for (auto it = finalPassengers.begin(); it != finalPassengers.end(); it++) {
            const Passenger &p = data.passengers[*it];
            if(currentCity == p.dk) {
                lotacao--;
            }
        }

        for (int j : bestSolution.passengers) {
            const Passenger &p = data.passengers[j];

            if (currentCity == p.ok && lotacao < data.vehicle_capacity) {
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

                    double costPerPassenger = static_cast<double>(travelCost) / (lotacao + 1);

                    if (costPerPassenger <= p.wk) {
                        finalPassengers.push_back(j);
                        lotacao++;
                    }
                }
            }
        }
    }

   
    bestSolution.passengers = finalPassengers;
    bestSolution.obj1 -= calculateTotalPassengerCost(data, bestSolution, finalPassengers);
    solution = bestSolution;
    
}

void collectBonuses(const InputData &data, Solution &solution) {
    int totalBonus = 0;
    int totalTime = 0;
    vector<int> collectedBonuses;

    for (int city : solution.tour) {
        if (data.b[city] > 0) {
            collectedBonuses.push_back(city);
            totalBonus += data.b[city];
            totalTime += data.t[city];
        }
    }

    for (int passengerIdx : solution.passengers) {
        const Passenger &p = data.passengers[passengerIdx];
        int embarkIndex = -1;
        int disembarkIndex = -1;

        for (int i = 0; i < solution.tour.size(); i++) {
            if (solution.tour[i] == p.ok) {
                embarkIndex = i;
            }
            if (solution.tour[i] == p.dk) {
                disembarkIndex = i;
                
                if (disembarkIndex == 0) 
                    disembarkIndex = solution.tour.size() - 1;
            }
            if (embarkIndex != -1 && disembarkIndex != -1) {
                break;
            }
        }

        if (embarkIndex == -1 || disembarkIndex == -1) {
            continue;
        }

        int travelTime = 0;
        
        for (int i = embarkIndex + 1; i <= disembarkIndex; i++) {
            travelTime += data.y[solution.tour[i - 1]][solution.tour[i]];
        }

        int accumulatedBonusTime = 0;
        for (int i = embarkIndex; i <= disembarkIndex; i++) {
            int city = solution.tour[i];
            if (data.b[city] > 0) {
                accumulatedBonusTime += data.t[city];
                int totalTimeWithBonus = travelTime + accumulatedBonusTime;

                if (totalTimeWithBonus > p.tk) {
                    auto it = find(collectedBonuses.begin(), collectedBonuses.end(), city);
                    if (it != collectedBonuses.end()) {
                        collectedBonuses.erase(it);
                        totalBonus -= data.b[city];
                        totalTime -= data.t[city];
                        accumulatedBonusTime -= data.t[city];
                    }
                }
            }
        }
    }

    solution.collectedBonuses = collectedBonuses;
    solution.bonusSum = totalBonus;
    solution.obj2 += totalTime;
}

void solveModel(GRBEnv &env, const InputData &data, vector<Solution> &solutions) {
    try {
        bool stop = false;

        int time_limit_seconds = 7200;
        TimeLimitCallback cb(time_limit_seconds);

        while(!stop) {
            GRBModel model = GRBModel(env);
            model.setCallback(&cb);
            model.set(GRB_DoubleParam_MIPGap, 1e-9);
            model.set(GRB_DoubleParam_MIPGapAbs, 1e-9);

            int n = data.n;
            double M = 10000;
            double epsilon = 0.1;

            
            vector<vector<GRBVar>> x(n, vector<GRBVar>(n));
            vector<GRBVar> u(n);
            vector<vector<GRBVar>> y(solutions.size(), vector<GRBVar>(2)); 

            for (int i = 0; i < n; i++) {
                u[i] = model.addVar(2.0, n, 0.0, GRB_INTEGER, "u_" + to_string(i));
                for (int j = 0; j < n; j++) {
                    if (i != j) {
                        x[i][j] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, "x_" + to_string(i) + "_" + to_string(j));
                    }
                }
            }
            
            for (int t = 0; t < solutions.size(); t++) {
                for (int j = 0; j < 2; j++) {
                    y[t][j] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, "y_" + to_string(t) + "_" + to_string(j));
                }
            }


            GRBLinExpr obj1 = 0;
            GRBLinExpr obj2 = 0;
            for (int i = 0; i < n; i++) {
                for (int j = 0; j < n; j++) {
                    if (i != j) {
                        obj1 += data.c[i][j] * x[i][j];
                        obj2 += data.y[i][j] * x[i][j];
                    }
                }
            }

            model.setObjective(obj2 + (epsilon * obj1), GRB_MINIMIZE);


            for (int i = 0; i < n; i++) {
                GRBLinExpr expr_in = 0;
                GRBLinExpr expr_out = 0;
                for (int j = 0; j < n; j++) {
                    if (i != j) {
                        expr_in += x[j][i];
                        expr_out += x[i][j];
                    }
                }
                model.addConstr(expr_in == 1, "in_" + to_string(i));
                model.addConstr(expr_out == 1, "out_" + to_string(i));
            }

            for (int i = 1; i < n ; i++) {
                for (int j = 1; j < n; j++) {
                    if (i != j) {
                        model.addConstr(u[i] - u[j] + (n * x[i][j]) <= n - 1, "subtour_" + to_string(i) + "_" + to_string(j));
                    }
                }
            }


            for (int t = 0; t < solutions.size(); t++) {
                for (int j = 0; j < 2; j++) {
                    if (j != 1) {
                        model.addConstr(obj1 <= (solutions[t].obj1 - 1) * y[t][j] - 1, "comp1_" + to_string(t) + "_" + to_string(j));
                    }
                }
            }

            for (int t = 0; t < solutions.size(); t++) {
                GRBLinExpr sumY = 0;
                for (int j = 0; j < 2; j++) {
                    if (j != 1) { 
                        sumY += y[t][j];
                    }
                }
                model.addConstr(sumY == 1, "comp2_" + to_string(t));
            }


            model.optimize();
            
            if (model.get(GRB_IntAttr_Status) == GRB_OPTIMAL && !cb.stop_optimization) {
                Solution newSolution = {obj1.getValue(), obj2.getValue(), extractTour(model, x, n)};
                solutions.push_back(newSolution);
            } else if (cb.stop_optimization) {
                cout << "Otimização interrompida devido ao limite de tempo. Solução ignorada." << endl;
                stop = cb.stop_optimization;
            } else if (model.get(GRB_IntAttr_Status) == GRB_INFEASIBLE) {
                cout << "Otimização finalizada! - GRB_INFEASIBLE" << endl;
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

bool isDominated(const Solution &sol1, const Solution &sol2) {
    bool isBetterInAtLeastOne = false;
    
    if (sol2.obj1 <= sol1.obj1 && sol2.obj2 <= sol1.obj2 && sol2.bonusSum >= sol1.bonusSum) {
        if (sol2.obj1 < sol1.obj1 || sol2.obj2 < sol1.obj2 || sol2.bonusSum > sol1.bonusSum) {
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
        if (type == "symmetric") {
            outputFileType = "sym";
        };
    
        for(string instance : instances)
        {
            auto start_time = chrono::high_resolution_clock::now();

            string path_input = "../../instances/A3/"+type+"/"+instance+".in";
            string path_output = "../../resultados/naive_v2/a3/"+outputFileType+"/";
            string file_log_name_output = instance+".log";

            string file_name_output = instance+".txt";

            InputData data = readInput(path_input);
            GRBEnv env = GRBEnv(true);
            env.start();

            vector<Solution> solutions;

            solveModel(env, data, solutions);

            for (int i = 0; i < solutions.size(); ++i) {
                rideMatchingHeuristic(data, solutions[i]);
                collectBonuses(data, solutions[i]);
            }

            filterDominatedSolutions(solutions);

            auto end_time = chrono::high_resolution_clock::now();
            chrono::duration<double> execution_time = end_time - start_time;

            if (execution_time.count() >= 7200.0) {
                file_name_output = instance + ".2h.txt";
            }


            ofstream outFile(path_output + file_name_output);
            if (outFile.is_open()) {
                outFile << "Fo(custo)\tFo(tempo)\tFo(bônus)\n";
                outFile << endl;

                for (const auto &solution : solutions) {
                    outFile << solution.obj1 << "\t" << solution.obj2 << "\t" << solution.bonusSum << "\n";
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
        }
    }
    remove("mip1.log");
    return 0;
}
