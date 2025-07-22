#pragma once

#include <iostream>
#include <vector> 
#include <chrono>
#include <limits>
#include <algorithm>
using namespace std;
using namespace std::chrono;


struct NormalizedObjectives {
  double cost;
  double time;
  double bonus;
};

struct ObjectiveBounds {
  double min_cost, max_cost;
  double min_time, max_time;
  double min_bonus, max_bonus;
};

struct Instance {
  int number_of_cities;
  int number_of_passengers;
  int car_capacity;
  vector<vector<int>> cost_matrix;
  vector<vector<int>> time_matrix;
  vector<Passenger> passengers;
  int min_quota; // we will ignore this for the purpose of MOTSPP
  vector<pair<int, int>> bonus_and_time;
};


struct Population {
  vector<Solution> population;

  //NSGA2 information
  vector<vector<int>> fronts;
  vector<double> crowding_distance;

  //SPEA2 information
  vector<int> raw_fitness;
  vector<double> fitness;
  vector<vector<double> > distances_to_other_solutions;
};

struct Objectives{
  double cost;
  int time;
  int total_bonus;
};

struct Pareto_objectives{
  vector<Objectives> pareto_set;
  vector<Solution> solutions;
};

struct Generations{
  vector<Pareto_objectives> generations;
  vector<duration<double>> durations;
};

struct Tests{
  vector<Generations> tests;
  vector<unsigned int> seeds;
  vector<duration<double>> final_durations;
  vector<Pareto_objectives> final_generations;
};


double getObj(Solution s, int k);
bool solution_validity(Instance instance, Solution solution);
void print_solution(Solution solution);
bool x_dominates_y(Solution solutionx, Solution solutiony);
double euclidean_distance(const Solution& a, const Solution& b);
double get_tchebycheff(Solution solution, vector<float> z, vector<double> weight_vector);
double get_tchebycheff(Objectives solution, vector<float> z, vector<double> weight_vector);
void mutate_routes(Instance instance, Solution &Solution, int mode);
void invert_bonuses(Instance instance, Solution &solution);
void able_passengers(Instance instance, Solution &solution);
//void save_data_routine(Generations& generations, BoundedParetoSet *EP, int &biggest_multiple, int valuations);