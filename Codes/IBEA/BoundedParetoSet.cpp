#include "BoundedParetoSet.h"
#include "structures.h"
#include <algorithm>
#include <list>

// Construtor e Destrutor da classe base
ParetoSet::ParetoSet() = default;
ParetoSet::~ParetoSet() {
    clear();
}

// Implementação dos métodos da classe base ParetoSet
bool ParetoSet::adicionarSol(Solution* s) {
    // Primeiro verifica se a solução é igual a alguma existente
    for (auto it = sol.begin(); it != sol.end(); ++it) {
        if (solutions_are_equal(*s, **it)) {
            return false;  // Não adiciona soluções duplicadas
        }
    }

    // Verifica se a solução é dominada ou domina alguma existente
    for (auto it = sol.begin(); it != sol.end();) {
        if (x_dominates_y(**it, *s)) {
            return false;  // Se a solução é dominada, não adiciona
        }
        if (x_dominates_y(*s, **it)) {
            delete *it;  // Libera a memória
            it = sol.erase(it);  // Remove a solução dominada
        } else {
            ++it;
        }
    }
    
    // Cria uma cópia da solução
    Solution* new_sol = new Solution();
    new_sol->cost = s->cost;
    new_sol->time = s->time;
    new_sol->total_bonus = s->total_bonus;
    new_sol->route = s->route;
    new_sol->cities_colected = s->cities_colected;
    new_sol->passengers_riding = s->passengers_riding;
    new_sol->fitness = s->fitness;
    
    // Adiciona a nova solução
    sol.push_back(new_sol);
    return true;
}

Solution ParetoSet::getSolucao(int k) {
    auto it = sol.begin();
    std::advance(it, k);
    return **it;
}

int ParetoSet::getSize() {
    return sol.size();
}

void ParetoSet::clear() {
    for (auto* s : sol) {
        delete s;
    }
    sol.clear();
}

// Construtor e Destrutor da classe derivada
BoundedParetoSet::BoundedParetoSet() : ParetoSet() {}

BoundedParetoSet::~BoundedParetoSet() {
    clear();
}

// Implementação dos métodos da classe derivada BoundedParetoSet
bool BoundedParetoSet::adicionarSol(Solution* s) {
    // Se já atingiu o tamanho máximo e a nova solução não domina nenhuma existente,
    // só adiciona se for melhor que a pior solução atual
    if (sol.size() >= MAXARCSIZE) {
        bool domina_alguma = false;
        for (auto it = sol.begin(); it != sol.end(); ++it) {
            if (x_dominates_y(*s, **it)) {
                domina_alguma = true;
                break;
            }
        }
        
        if (!domina_alguma) {
            // Encontra a solução com menor fitness
            auto worst = std::min_element(sol.begin(), sol.end(),
                [](const Solution* a, const Solution* b) {
                    return a->fitness < b->fitness;
                });
                
            // Só substitui se a nova solução for melhor
            if (s->fitness <= (*worst)->fitness) {
                return false;
            }
            
            delete *worst;  // Libera a memória
            sol.erase(worst);
        }
    }
    
    // Chama o método da classe base para adicionar a solução
    return ParetoSet::adicionarSol(s);
} 