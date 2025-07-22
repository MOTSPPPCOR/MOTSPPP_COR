#ifndef MAIN_CPP
#define MAIN_CPP

#include "main.h"
#include "BoundedParetoSet.cpp"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm> 
#include <chrono>
#include <filesystem>

using namespace std;
using namespace std::chrono;

// basic functions

void print_solution_full(Instance instance, Solution &solution) {
  cout << "Route: ";
  for(int i = 0; i < solution.route.size(); i++){
    cout << solution.route[i] << " ";
  }
  cout<<endl;

  cout<<"Route cost:"<<endl;
  for(int city = 0; city < solution.route.size(); city++){
    int next_city;
    if(city == solution.route.size() - 1){
      next_city = 0;
    }
    else{
      next_city = city+1;
    }
    cout << instance.cost_matrix[solution.route[city]][solution.route[next_city]]<< " ";
  }
  cout<<endl;

  cout<<"Route times:"<<endl;
  for(int city = 0; city < solution.route.size(); city++){
    int next_city;
    if(city == solution.route.size() - 1){
      next_city = 0;
    }
    else{
      next_city = city+1;
    }
    cout << instance.time_matrix[solution.route[city]][solution.route[next_city]]<< " ";
  }
  cout<<endl;

  cout << "Cities_colected: "<<endl;
  bool any_city_colected = false;
  for(int i =0; i<solution.cities_colected.size();i++){
    if(solution.cities_colected[i]==true){
      cout<<"City: "<< solution.route[i] << " bonus " << instance.bonus_and_time[solution.route[i]].first << " time" << instance.bonus_and_time[solution.route[i]].second << endl;
      any_city_colected = true;
    }
  }
  if(!any_city_colected){
    cout<<"No cities_colected"<<endl;
  }
  cout<<endl;
}

void print_solution(Solution solution) {
  cout << "Route: ";
  for(int i = 0; i < solution.route.size(); i++){
    cout << solution.route[i] << " ";
  }
  cout<<endl;
  cout << "Cost: " << solution.cost << endl;
  cout << "Bonus: " << solution.total_bonus << endl;
  cout << "Time: " << solution.time << "\n \n";

}

void print_solution(Instance instance, Solution &solution) {
  cout << "Route: ";
  for(int i = 0; i < solution.route.size(); i++){
    cout << solution.route[i] << " ";
  }
  cout<<endl;

  cout<<"Route:"<<endl;
  for(int city = 0; city < solution.route.size(); city++){
    int next_city;
    if(city == solution.route.size() - 1){
      next_city = 0;
    }
    else{
      next_city = city+1;
    }
    cout << instance.cost_matrix[solution.route[city]][solution.route[next_city]]<< " ";
  }
  cout<<endl;

  cout<<"Route times:"<<endl;
  for(int city = 0; city < solution.route.size(); city++){
    int next_city;
    if(city == solution.route.size() - 1){
      next_city = 0;
    }
    else{
      next_city = city+1;
    }
    cout << instance.time_matrix[solution.route[city]][solution.route[next_city]]<< " ";
  }
  cout<<endl;

  cout << "Cities_colected: "<<endl;
  bool any_city_colected = false;
  for(int i =0; i<solution.cities_colected.size();i++){
    if(solution.cities_colected[i]==true){
      cout<<"City: "<< solution.route[i] << " bonus " << instance.bonus_and_time[solution.route[i]].first << " time " << instance.bonus_and_time[solution.route[i]].second << endl;
      any_city_colected = true;
    }
  }
  if(!any_city_colected){
    cout<<"No city colected"<<endl;
  }
  cout<<endl;
  int passengers_on = 0;
  for (int i = 0; i < solution.passengers_riding.size(); i++) {
    if (solution.passengers_riding[i]) {
      passengers_on++;
      cout<< "passenger "<<i<<" with origin "<<instance.passengers[i].origin<<", destin: "<<instance.passengers[i].destiny<< " max cost: "<<instance.passengers[i].max_cost<<" max time: "<<instance.passengers[i].max_time<<endl;
    }
  }
  cout << "boarded passengers: " << passengers_on << endl;
  cout << "Cost: " << solution.cost << endl;
  cout << "Bonus: " << solution.total_bonus << endl;
  cout << "Time: " << solution.time << "\n \n";
}

void print_population(Instance instance, Population population) {
  for (int i = 0; i < population.population.size(); i++) {
    cout << "Solution " << i << ":" << endl;
    print_solution(instance, population.population[i]);
  }
  cout << "Soluções quantity: " << population.population.size() << endl;
}

void print_instance(Instance instance){
  cout << "Number of cities: " << instance.number_of_cities << endl;
  cout << "Number of passengers: " << instance.number_of_passengers << endl;
  cout << "Car capacity: " << instance.car_capacity <<"\n " <<endl;
}

void create_all_directories(int num_tests, int num_instances, std::string folder_name, vector<string> instances_addresses) {
  for (const std::string& algo : {"MOEAD"}) { 
    std::string algo_dir = folder_name + "/" + algo;
    for (int instance = 0; instance < num_instances; ++instance) {
        std::string instance_dir = algo_dir + "/"+instances_addresses[instance];  
          for (int test = 0; test < num_tests; ++test) {
            std::string teste_dir = instance_dir + "/tests_" + std::to_string(test);
            filesystem::create_directories(teste_dir);
           }
        }
    }
}

void fill_complete_archive(Instance instance, ofstream &arquivo_completo, Generations saved_generations, int generation, int pareto_set){
    arquivo_completo<<"Route: ";
    for(int i = 0; i < saved_generations.generations[generation].solutions[pareto_set].route.size(); i++){
      arquivo_completo<< saved_generations.generations[generation].solutions[pareto_set].route[i] << " ";
    }
    arquivo_completo<<endl;
    arquivo_completo<<"Route costs: ";
    for(int city = 0; city < saved_generations.generations[generation].solutions[pareto_set].route.size(); city++){
    int next_city;
    if(city == saved_generations.generations[generation].solutions[pareto_set].route.size() - 1){
      next_city = 0;
    }
    else{
      next_city = city+1;
    }
    arquivo_completo << instance.cost_matrix[saved_generations.generations[generation].solutions[pareto_set].route[city]][saved_generations.generations[generation].solutions[pareto_set].route[next_city]]<< " ";
  }
  arquivo_completo<<endl;

  arquivo_completo<<"Route times: ";
  for(int city = 0; city < saved_generations.generations[generation].solutions[pareto_set].route.size(); city++){
    int next_city;
    if(city == saved_generations.generations[generation].solutions[pareto_set].route.size() - 1){
      next_city = 0;
    }
    else{
      next_city = city+1;
    }
    arquivo_completo<< instance.time_matrix[saved_generations.generations[generation].solutions[pareto_set].route[city]][saved_generations.generations[generation].solutions[pareto_set].route[next_city]]<< " ";
  }
  arquivo_completo<<endl;

  arquivo_completo<< "Cities_colected: "<<endl;
  bool any_city_colected = false;
  for(int i =0; i<saved_generations.generations[generation].solutions[pareto_set].cities_colected.size();i++){
    if(saved_generations.generations[generation].solutions[pareto_set].cities_colected[i]==true){
      arquivo_completo<<"City: "<< saved_generations.generations[generation].solutions[pareto_set].route[i] << " bonus " << instance.bonus_and_time[saved_generations.generations[generation].solutions[pareto_set].route[i]].first << " time " << instance.bonus_and_time[saved_generations.generations[generation].solutions[pareto_set].route[i]].second << endl;
      any_city_colected = true;
    }
  }
  if(!any_city_colected){
    arquivo_completo<<"Nenhuma cidade coletada"<<endl;
  }
  int passengers_on = 0;
  for (int i = 0; i < saved_generations.generations[generation].solutions[pareto_set].passengers_riding.size(); i++) {
    if (saved_generations.generations[generation].solutions[pareto_set].passengers_riding[i]) {
      passengers_on++;
      arquivo_completo<< "passenger"<<i<<" with origin"<<instance.passengers[i].origin<<", destin: "<<instance.passengers[i].destiny<< " max cost: "<<instance.passengers[i].max_cost<<" max time: "<<instance.passengers[i].max_time<<endl;
    }
  }
  arquivo_completo << "boarded passengers: " << passengers_on << endl;
  arquivo_completo<< "Cost: " << saved_generations.generations[generation].solutions[pareto_set].cost << endl;
  arquivo_completo<< "Bonus: " << saved_generations.generations[generation].solutions[pareto_set].total_bonus << endl;
  arquivo_completo<< "Time: " << saved_generations.generations[generation].solutions[pareto_set].time << "\n \n";

}

void create_test_archives(Instance instance, string algorithm_name, string folder_name, vector<string> instances_addresses, int current_instance, int current_test, Generations saved_generations, unsigned int seed){
  for(int generation = 0; generation < saved_generations.generations.size(); generation++){
    string endereço_do_arquivo = folder_name+ "/"+algorithm_name+"/" + instances_addresses[current_instance] +"/"+ "test_" + to_string(current_test) +"/paretogeneration_" + to_string(generation) + ".txt";
    string endereço_do_arquivo_completo = folder_name+ "/"+algorithm_name+"/" + instances_addresses[current_instance] +"/"+ "test_" + to_string(current_test) +"/paretogeneration_" + to_string(generation) + "_complete.txt";
    string endereço_dados = folder_name+ "/"+algorithm_name+"/" + instances_addresses[current_instance] +"/"+ "test_" + to_string(current_test) + "/data";
    ofstream arquivo(endereço_do_arquivo);
    ofstream arquivo_completo(endereço_do_arquivo_completo);
    ofstream arquivo_dados(endereço_dados);
    for(int pareto_set = 0; pareto_set< saved_generations.generations[generation].pareto_set.size() ; pareto_set++){
      arquivo<<saved_generations.generations[generation].pareto_set[pareto_set].cost<<" "<< saved_generations.generations[generation].pareto_set[pareto_set].time<<" " << saved_generations.generations[generation].pareto_set[pareto_set].total_bonus;
      fill_complete_archive(instance, arquivo_completo, saved_generations, generation, pareto_set);
      //adicionar rota, cities, passengers, custos, tempos, etc
      if(pareto_set<saved_generations.generations[generation].pareto_set.size()-1){
        arquivo<<endl;
        arquivo_completo<<endl;
      }
    }
    arquivo_dados<< "seed: "<<seed<<endl;
    arquivo.close();
    arquivo_completo.close();
    arquivo_dados.close();
  }
}


Instance get_instance(string file_path) {
  Instance instance;

  ifstream file(file_path.c_str());
  if (!file.is_open()) {
    cerr << "Error " << file_path << endl;
    return instance;
  }

  // Reading data archives
  file >> instance.number_of_cities >> instance.number_of_passengers >> instance.car_capacity;

  // resizing matrices
  instance.cost_matrix.resize(instance.number_of_cities,  vector<int>(instance.number_of_cities));
  instance.time_matrix.resize(instance.number_of_cities,  vector<int>(instance.number_of_cities));

  // Reading cost_matrix
  for (int i = 0; i < instance.number_of_cities; ++i) {
    for (int j = 0; j < instance.number_of_cities; ++j) {
      file >> instance.cost_matrix[i][j];
    }
  }

  // Reading time_matrix
  for (int i = 0; i < instance.number_of_cities; ++i) {
    for (int j = 0; j < instance.number_of_cities; ++j) {
      file >> instance.time_matrix[i][j];
    }
  }

  // Reading passengers
  int max_cost, origin, destiny, max_time;
  for (int i = 0; i < instance.number_of_passengers; i++) {
    Passenger passenger;
    file >> max_cost >> origin >> destiny >> max_time;   
    passenger.max_cost = max_cost;
    passenger.origin = origin;
    passenger.destiny = destiny;
    passenger.max_time = max_time;
    instance.passengers.push_back(passenger);
  }

  // Reading min_quota
  file >> instance.min_quota;

  // Reading bonus_and_time
  vector<pair<int, int>> temp_bonus_and_time;
  vector<int> index;
  int city, bonus, time;
  while (file >> city >> bonus >> time) {
    temp_bonus_and_time.push_back(make_pair(bonus, time));
    index.push_back(city);
  }
  for(int i=0;i<temp_bonus_and_time.size();i++){
    for(int j=0;j<temp_bonus_and_time.size();j++){
      if(index[j] == i){
        instance.bonus_and_time.push_back(temp_bonus_and_time[j]);
        break;
      }
    }
  }

  file.close();

  return instance;
} // extracts the instance from a given file path and returns an Instance variable

void print_fronts(Population population) {
  for(int i=0; i<population.fronts.size(); i++){
    cout << "Fronte " << i << ":" << endl;
    for(int j=0; j<population.fronts[i].size(); j++){
      cout<<population.fronts[i][j]<<" ";
    }
    cout<<endl;
  }
}

int random_chance() {
  int random_chance = rand() % 100;
  return random_chance;
}
// returns a random number between 0 and 99

int random_city(Instance instance) {
  return rand() % instance.number_of_cities;
}
// returns a valid random city index from the instance

void swap_random_route_slots(Solution &solution) {
  int i = rand() % solution.route.size();
  int j = rand() % solution.route.size();
  swap(solution.route[i], solution.route[j]);
}
// swaps two random slots from a route of a solution

void swap(std::pair<double, int> &a, std::pair<double, int> &b) {
  pair<double, int> temp = a;
  a = b;
  b = temp;
}

void swap(int &a, int &b) {
  int temp = a;
  a = b;
  b = temp;
}

void swap(double &a, double &b) {
  double temp = a;
  a = b;
  b = temp;
}

// Function to adjust the heap
void heapify(vector<pair<double, int>> &vec, int n, int i) {
  int largest = i;       // Initialize the largest as root
  int left = 2 * i + 1;  // left = 2*i + 1
  int right = 2 * i + 2; // right = 2*i + 2

  // If the left child is greater than the root
  if (left < n && vec[left].first > vec[largest].first) {
    largest = left;
  }

  // If the right child is greater than the root
  if (right < n && vec[right].first > vec[largest].first) {
    largest = right;
  }

  // If the biggest is not the root
  if (largest != i) {
    swap(vec[i], vec[largest]);
    // Recursively heapify the affected subtree
    heapify(vec, n, largest);
  }
} //heapify a pair

void heapify(vector<int> &vec, int n, int i) {
  int largest = i;       // Initialize the largest as root
  int left = 2 * i + 1;  // left = 2*i + 1
  int right = 2 * i + 2; // right = 2*i + 2


  // If the left child is greater than the root
  if (left < n && vec[left] > vec[largest]) {
    largest = left;
  }

  // If the right child is greater than the root
  if (right < n && vec[right] > vec[largest]) {
    largest = right;
  }

  // If the biggest is not the root
  if (largest != i) {
    swap(vec[i], vec[largest]);
    // Recursively heapify the affected subtree
    heapify(vec, n, largest);
  }
} //heapify a vector

void heapify(vector<double> &vec, int n, int i) {
  int largest = i;       // Initialize the largest as root
  int left = 2 * i + 1;  // left = 2*i + 1
  int right = 2 * i + 2; // right = 2*i + 2

  // If the left child is greater than the root
  if (left < n && vec[left] > vec[largest]) {
    largest = left;
  }

  // If the right child is greater than the root
  if (right < n && vec[right] > vec[largest]) {
    largest = right;
  }

  // If the biggest is not the root
  if (largest != i) {
    swap(vec[i], vec[largest]);
    // Recursively heapify the affected subtree
    heapify(vec, n, largest);
  }
}

// Main function to do Heap Sort
void heapSort(vector<pair<double, int>> &vec) {
  int n = vec.size();

  // Builds the heap (rearranges the vector)
  for (int i = n / 2 - 1; i >= 0; i--) {
    heapify(vec, n, i);
  }

  // Extracts one element from the heap at a time
  for (int i = n - 1; i >= 0; i--) {
    // Move current root to the end
    swap(vec[0], vec[i]);
    // Call heapify on the reduced heap
    heapify(vec, i, 0);
  }
}// Sort ascending first

void heapSort(vector<int> &vec) {
  int n = vec.size();

  // Builds the heap (rearranges the vector)
  for (int i = n / 2 - 1; i >= 0; i--) {
    heapify(vec, n, i);
  }

  // Extracts one element from the heap at a time
  for (int i = n - 1; i >= 0; i--) {
    // Move current root to the end
    swap(vec[0], vec[i]);
    // Call heapify on the reduced heap
    heapify(vec, i, 0);
  }
}

void heapSort(vector<double> &vec) {
  int n = vec.size();

  // Builds the heap (rearranges the vector)
  for (int i = n / 2 - 1; i >= 0; i--) {
    heapify(vec, n, i);
  }

  // Extracts one element from the heap at a time
  for (int i = n - 1; i >= 0; i--) {
    // Move current root to the end
    swap(vec[0], vec[i]);
    // Call heapify on the reduced heap
    heapify(vec, i, 0);
  }
}

bool x_dominates_y(Solution solutionx, Solution solutiony) {
  if (solutionx.cost <= solutiony.cost && solutionx.time <= solutiony.time &&
      solutionx.total_bonus >= solutiony.total_bonus) {
    if (solutionx.cost < solutiony.cost || solutionx.time < solutiony.time ||
        solutionx.total_bonus > solutiony.total_bonus) {
      return true;
    }
  }
  return false;
} //Returns true only if solutionx dominates solutiony

Population get_non_dominated_population(Population received_population){
  Population population = received_population;
  for(int solution = 0; solution < population.population.size(); solution++){
    for(int solution_to_compare = solution+1; solution_to_compare  < population.population.size(); solution_to_compare++){
      if(x_dominates_y(population.population[solution], population.population[solution_to_compare])){
        population.population.erase(population.population.begin() + solution_to_compare );
        solution_to_compare--;
      }
      else if(x_dominates_y(population.population[solution_to_compare], population.population[solution])){
        population.population.erase(population.population.begin() + solution );
        solution--;
        break;
      }
      else if(population.population[solution].cost == population.population[solution_to_compare].cost and population.population[solution].total_bonus == population.population[solution_to_compare].total_bonus and population.population[solution].time == population.population[solution_to_compare].time){
        population.population.erase(population.population.begin() + solution_to_compare );
        solution_to_compare--;
      }
    }
  }
  return population;
} //return only the non-dominated solutions

Pareto_objectives get_pareto_objectives(Population received_population){
  Population population = get_non_dominated_population(received_population);
  Pareto_objectives matrix_of_objectives;
  for(int solution =0;solution<population.population.size();solution++){
   Objectives vector_of_objectives;
   vector_of_objectives.cost = population.population[solution].cost;
   vector_of_objectives.time = population.population[solution].time;
   vector_of_objectives.total_bonus = population.population[solution].total_bonus;

   matrix_of_objectives.pareto_set.push_back(vector_of_objectives);
  }
  return matrix_of_objectives;
} // returns a Pareto_objectives variable with all objectives from the population


//version that will receive BoundedParetoSet
Pareto_objectives get_pareto_objectives(BoundedParetoSet *received_population){
  Pareto_objectives matrix_of_objectives;
  int pop_size = received_population->get_size();
  for(int solution =0;solution<pop_size;solution++){
    matrix_of_objectives.pareto_set.push_back(received_population->get_objectives(solution));
    matrix_of_objectives.solutions.push_back(received_population->get_solution(solution));
  }
  return matrix_of_objectives;
} // returns a Pareto_objectives variable with all objectives from the population

int get_bonus(Instance instance, Solution &solution) {
  int bonus = 0;
  for (int city = 0; city < solution.route.size(); city++) {
    if (solution.cities_colected[city]) { // add bonus from cities collected
      bonus += instance.bonus_and_time[solution.route[city]].first;
    }
  }
  return bonus;
}
// calculates and returns the total bonus of the solution

int get_time(Instance instance, Solution &solution) {
  int time = 0;
  for (int city = 0; city < solution.route.size(); city++) {
    int next_city;
    if(city==solution.route.size()-1){
      next_city=0;
    }
    else{
      next_city=city+1;
    }
    time += instance.time_matrix[solution.route[city]][solution.route[next_city]];

    if (solution.cities_colected[city]){ // add time from bonus collecting
      time += instance.bonus_and_time[solution.route[city]].second;
    }
  } 
  return time;
}
// calculates and returns the total time of the solution

double get_cost(Instance instance, Solution &solution) {
  double cost = 0;
  int people_in_car = 1;

  for (int city = 0; city < solution.route.size(); city++) {
    double temp_cost = 0;
    int next_city;
    if(city==solution.route.size()-1){
      next_city=0;
    }
    else{
      next_city=city+1;
    }
    temp_cost = instance.cost_matrix[solution.route[city]][solution.route[next_city]];
    if(solution.passengers_riding.size()>0){
      for (int passenger = 0; passenger < instance.number_of_passengers; passenger++) {
        if (solution.passengers_riding[passenger]==true) {
          if (instance.passengers[passenger].origin == solution.route[city]) {
            people_in_car++;
          }
          if (instance.passengers[passenger].destiny == solution.route[city]) {
            if(city !=0){
              people_in_car--;
            }
          }
        }
      }
    }
    temp_cost /= people_in_car;
    cost += temp_cost;
  } // at every city checks how many are in the car to calculate the cost for
    // this part of the route
  return cost;
}
// calculates and returns the total cost of the solution

void update_objectives(Instance instance, Solution &solution) {
  solution.time = get_time(instance, solution);
  solution.cost = get_cost(instance, solution);
  solution.total_bonus = get_bonus(instance, solution);
}
// calls the 3 functions to query the value of the objectives and updates them in the respective places

vector<int> get_random_route(Instance instance) {
  int number_of_cities = 0;
  while (number_of_cities < 2) {
    number_of_cities = rand() % instance.number_of_cities;
  } // define a number of cities >= 2
  vector<int> route;
  vector<int> visited_cities(instance.number_of_cities, -1);
  route.push_back(0); // satisfy origin always 0
  visited_cities[0] =0; 
  int i = 1;
  while (i < number_of_cities) {
    int city = random_city(instance);
    if (visited_cities[city] != city) {
      route.push_back(city);
      visited_cities[city] = city;
      i++;
    }
  }
  return route;
}
// number of cities will be at minimum 2, max all cities, equal chances to all possibilities

vector<bool> get_random_bonus(Instance instance, Solution solution) {
  vector<bool> cities_colected;
  cities_colected.push_back(false); //first city is not collected
  for (int i = 0; i < solution.route.size()-1; i++) {
    if (rand() % 2 == 0) {
      cities_colected.push_back(true);
    } else {
      cities_colected.push_back(false);
    }
  }
  return cities_colected;
}
// 50% chance to collect a bonus. Treats cities_collected as having the same size as route. 
// This implies that the city in route[x] was collected if cities_collected[x] is True.

void print_solution(Solution solution) {
  cout << "Route: ";
  for(int i = 0; i < solution.route.size(); i++){
    cout << solution.route[i] << " ";
  }
  cout<<endl;
  cout << "Cost: " << solution.cost << endl;
  cout << "Bonus: " << solution.total_bonus << endl;
  cout << "Time: " << solution.time << "\n \n";

}

void print_solution(Instance instance, Solution &solution) {
  cout << "Route: ";
  for(int i = 0; i < solution.route.size(); i++){
    cout << solution.route[i] << " ";
  }
  cout<<endl;

  cout<<"Route:"<<endl;
  for(int city = 0; city < solution.route.size(); city++){
    int next_city;
    if(city == solution.route.size() - 1){
      next_city = 0;
    }
    else{
      next_city = city+1;
    }
    cout << instance.cost_matrix[solution.route[city]][solution.route[next_city]]<< " ";
  }
  cout<<endl;

  cout<<"Route times:"<<endl;
  for(int city = 0; city < solution.route.size(); city++){
    int next_city;
    if(city == solution.route.size() - 1){
      next_city = 0;
    }
    else{
      next_city = city+1;
    }
    cout << instance.time_matrix[solution.route[city]][solution.route[next_city]]<< " ";
  }
  cout<<endl;

  cout << "Cities_colected: "<<endl;
  bool any_city_colected = false;
  for(int i =0; i<solution.cities_colected.size();i++){
    if(solution.cities_colected[i]==true){
      cout<<"City: "<< solution.route[i] << " bonus " << instance.bonus_and_time[solution.route[i]].first << " time " << instance.bonus_and_time[solution.route[i]].second << endl;
      any_city_colected = true;
    }
  }
  if(!any_city_colected){
    cout<<"No city colected"<<endl;
  }
  cout<<endl;
  int passengers_on = 0;
  for (int i = 0; i < solution.passengers_riding.size(); i++) {
    if (solution.passengers_riding[i]) {
      passengers_on++;
      cout<< "passenger "<<i<<" with origin "<<instance.passengers[i].origin<<", destin: "<<instance.passengers[i].destiny<< " max cost: "<<instance.passengers[i].max_cost<<" max time: "<<instance.passengers[i].max_time<<endl;
    }
  }
  cout << "boarded passengers: " << passengers_on << endl;
  cout << "Cost: " << solution.cost << endl;
  cout << "Bonus: " << solution.total_bonus << endl;
  cout << "Time: " << solution.time << "\n \n";
}

double getObj(Solution s, int k){
	if(k == 0){
		return s.cost;
	}
	else if(k==1){
		return s.time;
	}
	else if(k==2){
		return s.total_bonus;
	}
  else{
    return -1;
  }
}

// Function to calculate passenger cost
double calculate_passenger_cost(int origem_index, int destiny_index, vector<int> passengers_in_car_by_city, Solution solution, Instance instance) {
    double cost = 0.0;
    // If the destination is the starting city, include the cost of the last part
    if (destiny_index == 0) {
        cost += instance.cost_matrix[solution.route[solution.route.size() - 1]][solution.route[0]] / (1+passengers_in_car_by_city[solution.route.size() - 1]);
        destiny_index = solution.route.size() - 1;
    }
    for (int i = origem_index; i < destiny_index; i++) {
        int next_index = i + 1;
        cost += instance.cost_matrix[solution.route[i]][solution.route[next_index]] / (1+ passengers_in_car_by_city[i]);
    }
    return cost;
}

// Function to calculate passenger time
double calculate_passenger_time(int origem_index, int destiny_index, Solution solution, Instance instance) {
    double time = 0;
    // If the destination is the starting city, include the time of the last part  
    if (destiny_index == 0) {
        time += instance.time_matrix[solution.route[solution.route.size() - 1]][solution.route[0]];
        destiny_index = solution.route.size() - 1;
    }
    for (int i = origem_index; i < destiny_index; i++) {
        int next_index = i + 1;
        time += instance.time_matrix[solution.route[i]][solution.route[next_index]];
        if (solution.cities_colected[i]) { //boarding and disembarking happens after bonus collection
          time += instance.bonus_and_time[solution.route[i]].second;
        }
    }
    if (solution.cities_colected[destiny_index]) { //boarding and disembarking happens after bonus collection
          time += instance.bonus_and_time[solution.route[destiny_index]].second;
    }
    return time;
}

void able_passengers(Instance instance, Solution &solution) {
    vector<bool> able_passengers(instance.number_of_passengers, false); 
    vector<int> passengers_in_car_by_city(solution.route.size(), 1); 
    vector<pair<double, int>> passengers_by_cost;

    for(int i = 0; i < instance.number_of_passengers; i++) {
        passengers_by_cost.push_back(make_pair(instance.passengers[i].max_cost, i));
    }

    heapSort(passengers_by_cost);  // Sort passengers by cost (from poorest to richest)

    for(int passenger = passengers_by_cost.size() - 1; passenger >= 0; passenger--) {  // Goes from richest to poorest
        int origem_index = -1, destiny_index = -1;
        double cost = 0.0;
        double time = 0;
        bool subiu = false, desceu = false, apto = true;

        for (int city = 0; city < solution.route.size(); city++) {
            if (solution.route[city] == instance.passengers[passengers_by_cost[passenger].second].origin) {
                // Passenger can board
                if (passengers_in_car_by_city[city] < instance.car_capacity) {
                    origem_index = city;
                    subiu = true;
                } else {
                    apto = false;
                    break;
                }
            } else if(solution.route[city] == instance.passengers[passengers_by_cost[passenger].second].destiny) {
                // Passenger disembarks
                if(city==0){ //special case in which destination is starting city
                    desceu = true;
                    destiny_index = city;
                }
                else{
                  if (subiu) { 
                      destiny_index = city;
                      desceu = true;
                      break;
                  } else {
                      apto = false;
                      break;
                  }
                }
            } else if (subiu) {
                if (passengers_in_car_by_city[city] >= instance.car_capacity) {// If the car is full
                  apto = false;
                  break;
                }
            }
        }

        // Handles the case where the passenger's destination is the starting city (route that returns to the point of origin)
        if (subiu && desceu && solution.route[0] == instance.passengers[passengers_by_cost[passenger].second].destiny) {
            if(passengers_in_car_by_city[solution.route.size()-1] >= instance.car_capacity){ //testa se o carro está cheio
              apto = false;
            }
        }

        // OBJECTIVE TESTING PHASE: If the passenger went up and down, calculates the cost, time and tests if he can afford it
        if (subiu && desceu && apto) {
            cost = calculate_passenger_cost(origem_index, destiny_index, passengers_in_car_by_city, solution, instance);
            time = calculate_passenger_time(origem_index, destiny_index, solution, instance);

            // Verifica se o passageiro pode arcar com o custo e tempo
            if (cost > instance.passengers[passengers_by_cost[passenger].second].max_cost || 
                time > instance.passengers[passengers_by_cost[passenger].second].max_time) {
                apto = false;
            }
        }

        // POST-TESTING PHASE: If the passenger is able, mark it as "able" and mark the car's occupancy in the sections where he is boarded
        if (subiu && desceu && apto) {
            able_passengers[passengers_by_cost[passenger].second] = true;
            if (destiny_index == 0) {
                passengers_in_car_by_city[solution.route.size()-1]++;
                destiny_index = solution.route.size() - 1;
            }
            for (int city = origem_index; city < destiny_index; city++){
                passengers_in_car_by_city[city]++;
            }
        }
    }
    solution.passengers_riding = able_passengers;
}


void able_passengers(Instance instance, Population &population) {
  for(int solution = 0; solution<population.population.size();solution++){
    able_passengers(instance, population.population[solution]);
  }
} // applies "able passenger"to all individuals in the population


bool check_passengers_riding(Instance instance, Solution solution) {
    vector<int> passengers_in_car_by_city(solution.route.size(), 1); 
    vector<pair<double, int>> passengers_by_cost;
    for(int i =0; i <instance.number_of_passengers;i++){//adds all passengers loaded from the solution to cost and index pairs
      if(solution.passengers_riding[i]){ 
        passengers_by_cost.push_back(make_pair(instance.passengers[i].max_cost,i));
      }
    }
    if(passengers_by_cost.size()==0){
      return true;
    }
    heapSort(passengers_by_cost); //here we have already ordered the passengers by poorest

    for(int passenger = passengers_by_cost.size() - 1; passenger >= 0; passenger--) { // Goes from richest to poorest
        int origem_index = -1, destiny_index = -1;
        double cost = 0.0;
        int time = 0;
        bool subiu = false, desceu = false, apto = true;

        for (int city = 0; city < solution.route.size(); city++) {
            if (solution.route[city] == instance.passengers[passengers_by_cost[passenger].second].origin) {
                // Passenger can board
                if (passengers_in_car_by_city[city] < instance.car_capacity) {
                    origem_index = city;
                    subiu = true;
                } else {
                    return false;
                }
            }else if(solution.route[city] == instance.passengers[passengers_by_cost[passenger].second].destiny) {
                // Passenger gets out
                if(city==0){ //special case in which destination is starting city
                    desceu = true;
                    destiny_index = city;
                }
                else{
                  if (subiu) { 
                      destiny_index = city;
                      desceu = true;
                      break;
                  } else {
                      return false;
                  }
                }
            }else if (subiu) {
                if (passengers_in_car_by_city[city] >= instance.car_capacity) {// if car is full
                  return false;
                  break;
                }
            }
        }

        // Handles the case where the passenger's destination is the starting city (route that returns to the point of origin)
        if (subiu && desceu && solution.route[0] == instance.passengers[passengers_by_cost[passenger].second].destiny) {
            if(passengers_in_car_by_city[solution.route.size()-1] >= instance.car_capacity){ //tests if the car is full
              return false;
            }
        }

        // OBJECTIVE TESTING PHASE: If the passenger went up and down, calculate the cost and time and test if he can afford it
        if (subiu && desceu && apto) {
            cost = calculate_passenger_cost(origem_index, destiny_index, passengers_in_car_by_city, solution, instance);
            time = calculate_passenger_time(origem_index, destiny_index, solution, instance);

            // Checks if the passenger can afford the cost and time
            if (cost > instance.passengers[passengers_by_cost[passenger].second].max_cost || 
                time > instance.passengers[passengers_by_cost[passenger].second].max_time) {
                return false;
            }
        }

        // POST-TESTING PHASE: If the passenger is able, mark the occupancy of the car in the sections where it was boarded
        if (subiu && desceu && apto) {
            if (destiny_index == 0) {
                passengers_in_car_by_city[solution.route.size()-1]++;
                destiny_index = solution.route.size() - 1;
            }
            for (int city = origem_index; city < destiny_index; city++){
                passengers_in_car_by_city[city]++;
            }
        }
    }

    return true;
}


bool solution_validity(Instance instance, Solution solution){
  bool validity = true;
  if(solution.route.size()<2){
    validity = false;
    cout<<"rota menor que 2"<<endl;
  }
  if(solution.route[0]!=0){
      validity = false;
      cout<<"Origem não é 0"<<endl;
  }
  if(solution.cost==0 or solution.time ==0){
    validity = false;
    cout<<"custo ou tempo 0"<<endl;
  }
  if(solution.cost != get_cost(instance, solution)){
    validity = false;
    cout<<"custo errado"<<endl;
  }
  if(solution.time != get_time(instance, solution)){
    validity = false;
    cout<<"tempo errado"<<endl;
  }
  if(solution.total_bonus != get_bonus(instance, solution)){
    validity = false;
    cout<<"bonus errado"<<endl;
  }
  if(check_passengers_riding(instance, solution)==false){
    validity = false;
    cout<<"passageiros errado"<<endl;
  }
    if(validity ==false){ print_solution(instance,solution);}

  return validity;
}

Solution get_random_solution(Instance instance) {
  Solution solution;
  solution.route = get_random_route(instance);
  solution.cities_colected = get_random_bonus(instance, solution);
  able_passengers(instance, solution);
  update_objectives(instance, solution);
  return solution;
}

Population get_random_population(Instance instance, int max_population) {
  Population population;
  int i =0;
  while(i < max_population) {
    Solution solution;
    solution = get_random_solution(instance);
    if(solution_validity(instance, solution)){
      population.population.push_back(solution);
      i++;
    }
  }
  return population;
}

Population get_random_population(Instance instance, BoundedParetoSet *EP ,int max_population, int &valuations) {
  Population population;
  int i =0;
  while(i < max_population) {
    Solution solution;
    solution = get_random_solution(instance);
    valuations++;
    if(solution_validity(instance, solution)){
      population.population.push_back(solution);
      if(EP->adicionarSol(&solution)){
          continue;
      }
      i++;
    }
    
  }
  return population;
}

void mutate_routes(Instance instance, Solution &Solution, int mode) {
  if (mode == 3) {
    if (Solution.route.size() <= 2) {
      mode = 2;
    } else if (Solution.route.size() == instance.number_of_cities) {
      mode = rand() % 2;
    } else {
      mode = rand() % 3;
    }
  } // this if ensures that if mutate_routes is called, it will definitely occur some mutation. 
  if (mode == 0) { // swap vertex (but not origin)
    int city1 = rand() % Solution.route.size(); 
    while(city1 == 0){
      city1 = rand() % Solution.route.size();
    } 
    int city2 = rand() % Solution.route.size();
    while(city2==0){
      city2 = rand() % Solution.route.size();
    }
    swap(Solution.route[city1], Solution.route[city2]);
    swap(Solution.cities_colected[city1], Solution.cities_colected[city2]);
  } else if (mode == 1) { // remove random vertex, but not origin
      int city_to_diminish = 0;
      while(city_to_diminish==0){
        city_to_diminish = rand() % Solution.route.size();
      }
      Solution.route.erase(Solution.route.begin() + city_to_diminish);
      Solution.cities_colected.erase(Solution.cities_colected.begin() + city_to_diminish);
  }else if (mode == 2) { // add random vertex that is not already in the route in any place except origin
  
      vector<int> cities_not_in_route;
      bool marker = false;
      for (int j = 0; j < instance.number_of_cities; j++) {
        marker = false;
        for (int k = 0; k < Solution.route.size(); k++) {
          if (Solution.route[k] == j) {
            marker = true;
            break;
          }
        }
        if (marker == false) {
          cities_not_in_route.push_back(j);
        }
      }
      int city_to_add = rand() % cities_not_in_route.size();
      int city_to_add_index = rand() % Solution.route.size();
      while(city_to_add_index==0){
        city_to_add_index = rand() % Solution.route.size();
      }
      Solution.route.insert(Solution.route.begin() + city_to_add_index, cities_not_in_route[city_to_add]);
      Solution.cities_colected.insert(Solution.cities_colected.begin() + city_to_add_index, false);
  }
}
// mutates a solution in four different settings according to "mode": 0- swapping, 1 - removing, 2 - adding a new vertex and 3 - random


void mutate_bonuses(Instance instance, Solution &solution) {
  int bonus_change = rand() % solution.cities_colected.size();
  while(bonus_change==0){
    bonus_change = rand() % solution.cities_colected.size();
  }
  if (solution.cities_colected[bonus_change] == false) {
    solution.cities_colected[bonus_change] = true;
  } else {
    solution.cities_colected[bonus_change] = false;
  }
} //mutates a solution by inverting a bit in bonus colecting vector


Solution one_crossover(Instance instance, vector<Solution> &population, int father, int mother) {
  Solution baby;
  for (int i = 0; i < population[father].route.size() / 2; i++) {
    baby.route.push_back(population[father].route[i]);
    baby.cities_colected.push_back( population[father].cities_colected[i]);
  } // first half of the route and collections from the father

  for (int j = population[mother].route.size() / 2;
       j < population[mother].route.size(); j++) {
    bool gene_is_repeated = false;
    for (int k = 0; k < population[father].route.size() / 2; k++) {
      if (population[mother].route[j] ==  population[father].route[k]) {
        gene_is_repeated = true;
      }
    }
    if (gene_is_repeated == false) {
      baby.route.push_back(population[mother].route[j]);
      baby.cities_colected.push_back(population[mother].cities_colected[j]);
    }
  } // second half of the route and collections from the mother
  return baby;
}// returns a child that will have the initial half of the parent's route, second half of the mother's route (except repeated cities), collecting bonuses if they collect in this city. 
// THIS FUNCTION DOES NOT CALL passenger loading heuristics

Solution one_crossover(Instance instance, vector<Solution> &population) {
  int father = rand() % population.size(), mother = rand() % population.size();
  Solution baby = one_crossover(instance, population, father, mother);
  return baby;
}

vector<Solution> crossover(Instance instance, Population &parents) {
  vector<Solution> children;
  while (children.size() < parents.population.size()) {
    Solution baby = one_crossover(instance, parents.population);
    update_objectives(instance, baby);
    children.push_back(baby);
  }
  return children;
}
// returns a population of children equal to the size of the population of parents

void save_data_routine(Generations& generations, BoundedParetoSet *EP, std::chrono::high_resolution_clock::time_point &inicio, int &biggest_multiple, int valuations, duration<double>& tempoTotal){
    auto fim = high_resolution_clock::now();
    duration<double> tempotrecho = duration_cast<duration<double>>(fim - inicio);//update stretch time
    generations.durations.push_back(tempotrecho);
    tempoTotal += duration_cast<duration<double>>(fim - inicio); //adding total time
    Pareto_objectives sets_of_objectives = get_pareto_objectives(EP);
    generations.generations.push_back(sets_of_objectives);
    inicio = high_resolution_clock::now();
}

float calculate_euclidian_distance(vector<double> &weight_vector1, vector<double> &weight_vector2) {
  float distance = 0.0;
  for (int i = 0; i < weight_vector1.size(); i++) {
    distance += pow(weight_vector1[i] - weight_vector2[i], 2);
  }
  return sqrt(distance);
} // calculate distance between two weight vectors

vector<vector<float>> get_T_neighbors(vector<vector<double> > weight_vectors, int T_neighborhood_size){
    vector<vector<float>> T_neighbors(weight_vectors.size());
    for(int i = 0; i<weight_vectors.size();i++){
      vector<pair<double,int>> distances;
      for(int j = 0; j< weight_vectors.size();j++){
        if(i!=j){
          distances.push_back(make_pair(calculate_euclidian_distance(weight_vectors[i], weight_vectors[j]),j));
        }
      }
      heapSort(distances);
      for(int t = 0; t<T_neighborhood_size;t++){
        T_neighbors[i].push_back(distances[t].second);
      }
    }
    return T_neighbors;
}

vector<float> get_initial_z(Instance instance, Population population) {
  vector<float> z(3, 0);

  z[0] = population.population[0].cost;
  for (int i = 1; i < population.population.size(); i++) {
    double cost = get_cost(instance, population.population[i]);
    if (cost < z[0]) {
      z[0] = cost;
    }
  }

  z[1] = population.population[0].time;
  for (int i = 1; i < population.population.size(); i++) {
    double time = get_time(instance, population.population[i]);
    if (time < z[1]) {
      z[1] = time;
    }
  }

  z[2] = population.population[0].total_bonus;
  for (int i = 1; i < population.population.size(); i++) {
    double bonus = get_bonus(instance, population.population[i]);
    if (bonus > z[2]) {
      z[2] = bonus;
    }
  }

  return z;
} // returns z = [cost, time, bonus]

void update_z(vector<float> z, Solution solution){
  if(solution.cost<z[0]){//step 2.3 of MOEAD paper
    z[0] = solution.cost;
  }
  if(solution.time<z[1]){
    z[1] = solution.time;
  }
  if(solution.total_bonus> z[2]){
    z[2] = solution.total_bonus;
  }
}//z[cost,time,bonus]

double get_tchebycheff(Solution solution, vector<float> z, vector<double> weight_vector){
  double result = fabs(weight_vector[0]*(solution.cost - z[0]));
  if(result<fabs(weight_vector[1]*(solution.time - z[1]))){
    result = fabs(weight_vector[1]*(solution.time - z[1]));
  }
  if(result<fabs(weight_vector[2]*(solution.total_bonus - z[2]))){
    result = fabs(weight_vector[2]*(solution.total_bonus - z[2]));
  }
  return result;
}

pair<vector<Solution>, vector<int>> initial_population_moead (Population Population, vector<float> z, vector<vector<double> > weight_vectors){
  pair<vector<Solution>, vector<int>> initial_population;
  for(int i = 0; i<weight_vectors.size();i++){
    pair<double,int> best_for_this_vector;
    best_for_this_vector.second = 0;
    best_for_this_vector.first = get_tchebycheff(Population.population[0], z, weight_vectors[i]);

    for(int j=1;j<Population.population.size();j++){
      double j_tchebycheff = get_tchebycheff(Population.population[j], z, weight_vectors[i]);
      if(best_for_this_vector.first> j_tchebycheff){
        best_for_this_vector.second = j;
        best_for_this_vector.first = j_tchebycheff;
      }
      //define which of the population is best for weight vector i
    }
    initial_population.first.push_back(Population.population[best_for_this_vector.second]);
    initial_population.second.push_back(best_for_this_vector.first);
  }
  return initial_population;
}

void update_neighboring_solutions(int n, pair<vector<Solution>, vector<int> > &internal_population, Solution baby, vector<vector<double> > weight_vectors, vector<vector<float>> T_neighbors,vector<float> z ){
  for(int t=0;t<T_neighbors[n].size(); t++){
    double baby_tchebycheff = get_tchebycheff(baby, z, weight_vectors[T_neighbors[n][t]]);
    double neighbor_tchebycheff = get_tchebycheff(internal_population.first[T_neighbors[n][t]], z, weight_vectors[T_neighbors[n][t]]);
    if(baby_tchebycheff < neighbor_tchebycheff){
        internal_population.first[T_neighbors[n][t]] = baby;
        internal_population.second[T_neighbors[n][t]] = baby_tchebycheff;
    }
  }
}

vector<vector<double>> generateWeightVectors(int NUMSUMPROBLEMAS) {
  vector<vector<double>> lambda(NUMSUMPROBLEMAS, vector<double>(3, 0.0f));
  int  s= 23; // dont change
	int cont =0;
	double l1, l2, l3;
    for (int i=0; i<=s && cont<NUMSUMPROBLEMAS; i++){
        l1 = i;
        for (int j=0; j<=s-i && cont<NUMSUMPROBLEMAS; j++){
            l2 = j;
            l3 = s-i - l2;
            lambda[cont][0] = static_cast<double>(l1/s); // first coordinate
            lambda[cont][1] = static_cast<double>(l2/s); // second coordinate
            lambda[cont][2] = static_cast<double>(l3/s); // third coordinate
            cont++; 
        }
    }
    return lambda;
}

Population MOEAD(Generations& MOEAD_generations, BoundedParetoSet *EP ,duration<double>& tempoTotal, Instance instance,  Population &population, int max_valuations, int N_subproblems, int T_neighborhood_size, vector<vector<double> > weight_vectors, int mutation_chance, int crossover_chance, int valuations_before_alg ) {
  //Population EP; //1.1
  vector<vector<float>> T_neighbors(weight_vectors.size());
  T_neighbors = get_T_neighbors(weight_vectors, T_neighborhood_size);//1.2
  vector<float> z = get_initial_z(instance, population);//1.4
  pair<vector<Solution>, vector<int>> internal_population = initial_population_moead(population, z, weight_vectors); //1.3
  //end of initialization
  int valuations = valuations_before_alg ;
  int biggest_multiple = 0;
  int multiple = biggest_multiple+1;
  Pareto_objectives sets_of_objectives = get_pareto_objectives(EP);
  MOEAD_generations.generations.push_back(sets_of_objectives);
  auto inicio = high_resolution_clock::now();
  while (valuations < max_valuations) {
    int n=0;
    while(n<N_subproblems){
        int father,mother;
        Solution baby;
        if(crossover_chance < random_chance()){
          father= T_neighbors[n][rand()%T_neighborhood_size];
          mother= T_neighbors[n][rand()%T_neighborhood_size];
          baby = one_crossover(instance, internal_population.first, father, mother);//2.1
          able_passengers(instance, baby);
          update_objectives(instance, baby);
          if(solution_validity(instance, baby)){
            if(EP->adicionarSol(&baby)){
              continue;
            }
          }
          valuations++;
          int multiple = biggest_multiple+1;
          if(multiple*10000<=valuations){
              save_data_routine(MOEAD_generations,EP, inicio, biggest_multiple, valuations, tempoTotal);
              if(valuations>=max_valuations){
                break;
              }
              biggest_multiple++;
          }
          
          if(mutation_chance < random_chance()){
            mutate_routes(instance,baby,3);
            mutate_bonuses(instance, baby);
            able_passengers(instance, baby);
            update_objectives(instance, baby);
            if(solution_validity(instance, baby)){
              if(EP->adicionarSol(&baby)){
                continue;
              }
            }
            valuations++;
            int multiple = biggest_multiple+1;
            if(multiple*10000<=valuations){
                save_data_routine(MOEAD_generations,EP, inicio, biggest_multiple, valuations, tempoTotal);
                if(valuations>=max_valuations){
                  break;
                }
                biggest_multiple++;
            }
          }//2.2
        }
        else{
          baby = get_random_solution(instance);
          if(solution_validity(instance, baby)){
            if(EP->adicionarSol(&baby)){
              continue;
            }
          }
          valuations++;
          int multiple = biggest_multiple+1;
          if(multiple*10000<=valuations){
              save_data_routine(MOEAD_generations,EP, inicio, biggest_multiple, valuations, tempoTotal);
              if(valuations>=max_valuations){
                break;
              }
              biggest_multiple++;
          }
        }
        if(solution_validity(instance,baby)==true){
          update_z(z,baby);//2.3
          update_neighboring_solutions(n,internal_population, baby, weight_vectors, T_neighbors, z); //2.4 
          n++;
        }
    }    
  }
  return population;
}



int main() {
  string folder_name = "run_congress";  //results directories
  int qt_tests=20;
  //Instances setup
  vector<string> instances_addresses = {
    "instances/A1/symmetric/5.1.in",    "instances/A1/symmetric/5.2.in",   "instances/A1/symmetric/5.3.in",  
    "instances/A1/symmetric/5.4.in",    "instances/A1/symmetric/5.5.in",   "instances/A1/symmetric/5.6.in",  
  };
  int numero_de_instancias = instances_addresses.size();

  //Algorithms setup
  int max_population;
  int max_valuations ;
  int N_subproblems;
  int crossover_chance_moead;
  int mutation_chance_moead;
  int T_neighborhood_size;
  vector<vector<double>> weight_vectors = generateWeightVectors(N_subproblems);
  try {
    create_all_directories(qt_tests, numero_de_instancias, folder_name, instances_addresses);
    std::cout << "Directories created!" << std::endl;
  } catch (const  std::filesystem::filesystem_error& e) {
    std::cerr << "Error:  " << e.what() << std::endl;
  }

  //Tests area
  for(int instance_index =0; instance_index< numero_de_instancias;instance_index++){
    Instance instance = get_instance(instances_addresses[instance_index]); 
    cout<<"Instance starting: "<<instances_addresses[instance_index]<<endl;
    print_instance(instance);

    //Testes MOEAD
    for(int current_test =0; current_test<qt_tests;current_test++){
      cout<<"current test MOEAD: "<<current_test<<" in instance "<<instances_addresses[instance_index]<<endl;
      BoundedParetoSet *EP = new BoundedParetoSet();
      Population population;
      Generations saved_generations;
      unsigned int seed = static_cast<unsigned int>(time(0)); 
      srand(seed);

      int valuations_before_alg =0;
      Population initial_population = get_random_population(instance, EP, max_population,valuations_before_alg);
      int max_valuations_after_alg = max_valuations- valuations_before_alg;
      duration<double> tempoTotal = duration<double>::zero();
      population = MOEAD(saved_generations, EP, tempoTotal,instance, initial_population, max_valuations_after_alg, N_subproblems, T_neighborhood_size, weight_vectors, mutation_chance_moead , crossover_chance_moead, valuations_before_alg );

      create_test_archives(instance, "MOEAD", folder_name, instances_addresses, instance_index, current_test,  saved_generations, seed, tempoTotal);
      delete EP;
    }
    cout<<"MOEAD Results available!"<<endl;

  }

}

#endif