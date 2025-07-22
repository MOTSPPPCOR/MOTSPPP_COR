#ifndef MAIN_FUNCTIONS_H
#define MAIN_FUNCTIONS_H

#include <vector> 
#include <chrono>
using namespace std;
using namespace std::chrono;


struct Passenger {
  int max_cost;
  int origin;
  int destiny;
  int max_time;
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

struct Solution {
  vector<int> route;
  vector<bool> cities_colected;
  vector<bool> passengers_riding; //(instance.number_of_passengers,false)
  double cost;
  int time;
  int total_bonus;
};


struct Population {
  vector<Solution> population;

  //NSGA2 information
  vector<vector<int>> fronts;
  vector<double> crowding_distance;

  //SPEA2 information
  vector<int> strenghts;
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
void print_solution(Solution solution);
bool x_dominates_y(Solution solutionx, Solution solutiony);

#endif
