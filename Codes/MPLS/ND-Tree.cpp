#include "ND-Tree.h"

    NDNode::NDNode() {
      ideal_point.cost = numeric_limits<double>::max();
      ideal_point.time = numeric_limits<int>::max();
      ideal_point.total_bonus = 0;    
      nadir_point.cost =0;
      nadir_point.time = 0;
      nadir_point.total_bonus = numeric_limits<int>::max();
    }

    NDNode::~NDNode() {
        for (NDNode* child : children) {
            delete child; // Libera memória recursivamente
        }
        children.clear();
    }
      
  
    void NDNode::updateIdealNadir(const Solution& y) {
      bool changed = false;
  
      if (y.cost < ideal_point.cost) {
          ideal_point.cost = y.cost;
          changed = true;
      }
      if (y.time < ideal_point.time) {
          ideal_point.time = y.time;
          changed = true;
      }
      if (y.total_bonus > ideal_point.total_bonus) {
          ideal_point.total_bonus = y.total_bonus;
          changed = true;
      }
  
      if (y.cost > nadir_point.cost) {
          nadir_point.cost = y.cost;
          changed = true;
      }
      if (y.time > nadir_point.time) {
          nadir_point.time = y.time;
          changed = true;
      }
      if (y.total_bonus < nadir_point.total_bonus) {
          nadir_point.total_bonus = y.total_bonus;
          changed = true;
      }
  
      // recursively on parent if its not the root
      if (changed && parent != nullptr) {
          parent->updateIdealNadir(y);
      }
    }

    void NDNode::split(
        int max_children, 
        int max_solutions
    ) {
        if (is_leaf && solutions.size() <= max_solutions) return;

        // Passo 1: Encontra o ponto z com maior distância média
        Solution z = findPointWithMaxAvgDistance();

        // Passo 2: Cria o primeiro filho com z
        createChildWithPoint(z, max_solutions); //it takes the solution z away from the current node

        // Passo 3: Cria os filhos restantes
        while (children.size() < max_children && !solutions.empty()) {
            z = findPointFarthestFromChildren();
            createChildWithPoint(z, max_solutions);
        }

        // Passo 4: Distribui os pontos restantes para o filho mais próximo
        while (!solutions.empty()) {
            Solution z = solutions.front();
            assignPointToClosestChild(z);
        }

        if (!children.empty()) {
            is_leaf = false;
        }
    }
  
    // Algorithm 5: Insert
    void NDNode::insert(
        const Solution& y, 
        int max_children, 
        int max_solutions) {
        if (is_leaf) {
            // 1. Adiciona ao nó folha
            solutions.push_back(y);
            updateIdealNadir(y);

            // 2. Verifica se precisa dividir
            if (solutions.size() > max_solutions) {
                split(max_children, max_solutions);
            }//didnt do yet the case to max_children
        } else {
            // 3. Encontra o filho mais próximo e insere recursivamente
            if(children.empty()){
                cout<<"children empty mesmo não sendo folha. tá errado isso ai"<<endl;
            }
            NDNode* closest_child = findClosestChild(y);
            if(closest_child != nullptr){
               closest_child->insert(y, max_children, max_solutions);   
            }
        }
    }//rever se esse aqui tá certo com o artigo
    
    
    bool NDNode::updateNode(const Solution& y, bool& should_delete, int& current_size) {
        
        // Property 1: y é coberto pelo nadir_point
        if (nadir_point.cost <= y.cost && nadir_point.time <= y.time &&
            nadir_point.total_bonus >= y.total_bonus) {
            return false; // Rejeita y
        }

        // Property 2: y cobre ideal_point => remove o nó e sua subárvore
        else if (ideal_point.cost >= y.cost && ideal_point.time >= y.time &&
            ideal_point.total_bonus <= y.total_bonus) {
            
            should_delete = true;
            return true;
        }
    
        // terceiro caso
        else if ((ideal_point.cost <= y.cost && 
            ideal_point.time <= y.time &&
            ideal_point.total_bonus >= y.total_bonus) or 
            (nadir_point.cost >= y.cost && 
            nadir_point.time >= y.time &&
            nadir_point.total_bonus <= y.total_bonus)) {
            if (is_leaf) {
                // Verifica dominância com cada ponto na folha
                for (auto it = solutions.begin(); it != solutions.end();) {
                    if (x_dominates_y(*it, y) or 
                    (it->cost == y.cost and it->time == y.time and it->total_bonus == y.total_bonus)) {
                        return false; // y é dominado ou igual a um ponto existente
                    } else if (x_dominates_y(y, *it)) {
                        current_size--;
                        it = solutions.erase(it); // remove ponto dominado
                    } else {
                        ++it;
                    }
                }
                if(solutions.empty()){
                    should_delete = true;
                }
            } else {
                // Recursivamente atualiza os filhos
                if(children.empty()){
                    should_delete = true;
                    return true;
                }
                for (auto it = children.begin(); it != children.end();) {
                    NDNode* child = *it;    
                    bool accepted = child->updateNode(y, should_delete, current_size);
                    
    
                    if (should_delete) {
                        current_size -= child->solutions.size();
                        delete child;
                        it = children.erase(it);
                        should_delete = false; // Resetar para o próximo loop
                    } else {
                        ++it;
                    }
                    if (!accepted) return false;
                }
                
                if(children.empty()){
                    should_delete = true;
                    return true;
                }
                // Se só sobrou um filho, este nó se torna desnecessário
                if (children.size() == 1) {
                    NDNode* only = children.front();
                    
                    // Mover os dados importantes manualmente
                    is_leaf = only->is_leaf;
                    solutions = only->solutions;
                    ideal_point = only->ideal_point;
                    nadir_point = only->nadir_point;
                    
                    // Primeiro limpa qualquer children atual
                    for (auto* child : children) {
                        if (child != only) delete child;
                    }
                    
                    children = only->children; // Move filhos (sem criar ponteiros inválidos)
                    for (auto* child : children) {
                        if (child) child->parent = this; // Atualiza parent!
                    }
                    only->children.clear(); // Limpa os filhos do nó
                    delete only;
                }
                return true;
            }
        }
    
        // Property 1 ou 2 já lidaram com rejeição e remoção
        return true; // Property 3: y é aceito (não dominado)
    }
    

     // Fixed to find closest child to y taking in account the method in the paper
     NDNode* NDNode::findClosestChild(const Solution& y) {
        double min_distance = numeric_limits<double>::max();
        NDNode* closest = nullptr;
    
        for (auto* child : children) {
            double dist = calculateDistanceToMiddlePointOfChild(y, *child);
            if (dist < min_distance) {
                min_distance = dist;
                closest = child;
            }
        }
        return closest;
    }
    
    double NDNode::calculateDistanceToMiddlePointOfChild(const Solution& y, const NDNode& child) {//took out sqrt to avoid unnecessary calculations
        const double mid_cost = (child.ideal_point.cost + child.nadir_point.cost)/2;
        const double mid_time = (child.ideal_point.time + child.nadir_point.time)/2;
        const double mid_bonus = (child.ideal_point.total_bonus + child.nadir_point.total_bonus)/2;
        
        const double diff_cost = y.cost - mid_cost;
        const double diff_time = y.time - mid_time;
        const double diff_bonus = y.total_bonus - mid_bonus;
        
        return diff_cost*diff_cost + diff_time*diff_time + diff_bonus*diff_bonus;
}
    
    // Encontra o ponto em solutions com a maior distância média para os outros
    Solution NDNode::findPointWithMaxAvgDistance() {
        Solution best_z;
        double max_avg_distance = -1.0;
        
        // Pré-calcular todas as distâncias uma vez
        vector<vector<double>> distance_matrix(solutions.size(), vector<double>(solutions.size()));
        for (size_t i = 0; i < solutions.size(); ++i) {
            for (size_t j = i+1; j < solutions.size(); ++j) {
                double dist = euclidean_distance(solutions[i], solutions[j]);
                distance_matrix[i][j] = dist;
                distance_matrix[j][i] = dist;
            }
        }
        
        for (size_t i = 0; i < solutions.size(); ++i) {
            double total_distance = accumulate(distance_matrix[i].begin(), distance_matrix[i].end(), 0.0);
            double avg_distance = total_distance / (solutions.size() - 1);
            
            if (avg_distance > max_avg_distance) {
                max_avg_distance = avg_distance;
                best_z = solutions[i];
            }
        }
        
        return best_z;
    }

    // Encontra o ponto em solutions mais distante dos filhos existentes
    Solution NDNode::findPointFarthestFromChildren() {
        Solution best_z;
        double max_min_distance = -1.0;

        for (const auto& z : solutions) {
            double min_distance = std::numeric_limits<double>::max();
            for (const auto& child : children) {
                for (const auto& child_point : child->solutions) {
                    double dist = euclidean_distance(z, child_point);
                    if (dist < min_distance) {
                        min_distance = dist;
                    }
                }
            }

            if (min_distance > max_min_distance) {
                max_min_distance = min_distance;
                best_z = z;
            }
        }

        return best_z;
    }

    // Cria um novo filho com um ponto e atualiza ideal/nadir
    void NDNode::createChildWithPoint(
        const Solution& z, 
        int max_solutions
    ) {
        NDNode* child = new NDNode();
        child->parent = this;
        child->solutions.push_back(z);
        child->updateIdealNadir(z);
        children.push_back(child);
    
        // Remove z de solutions
        solutions.erase(
            std::remove_if(solutions.begin(), solutions.end(),
                [&z](const Solution& s) {
                    return fabs(s.cost - z.cost) < 1e-6 &&
                           s.time == z.time &&
                           s.total_bonus == z.total_bonus; }),
            solutions.end()
        );
    }
    

    // Atribui um ponto ao filho mais próximo
    void NDNode::assignPointToClosestChild(const Solution& z) {
        NDNode* closest_child = findClosestChild(z);
    
        if (closest_child) {
            closest_child->solutions.push_back(z);
            closest_child->updateIdealNadir(z);
            solutions.erase(
                std::remove_if(solutions.begin(), solutions.end(),
                    [&z](const Solution& s) { 
                        return fabs(s.cost - z.cost) < 1e-6 &&
                               s.time == z.time &&
                               s.total_bonus == z.total_bonus; }),
                solutions.end()
            );
        }
    }
    
    void NDNode::printSolutions() const {
        if(is_leaf){
            for(const auto solution : solutions)
            print_solution(solution);
        }
        else{
            for(const auto& child:children){
                if (child) child->printSolutions();
            }
        }
    }
    void NDNode::GetAllSolutions(vector<Solution>& allSolutions) {
        if (is_leaf) {
            allSolutions.insert(allSolutions.end(), solutions.begin(), solutions.end());
        } else {
            for (const auto& child : children) {
                if (child) {
                    child->GetAllSolutions(allSolutions);
                }
            }
        }
    }
    


    NDTree::NDTree(int max_c, int max_s, int max_siz): 
    max_children(max_c), max_solutions(max_s), max_size(max_siz) {}

    NDTree::~NDTree() {
        if (root) delete root;
    }

    void NDTree::update(const Solution& y) {
        // Algoritmo 3: ND-Tree-based update

        if (root == nullptr) {
            // Y_N está vazio: cria um nó folha com y como raiz
            root = new NDNode();
            root->solutions.push_back(y);
            root->updateIdealNadir(y);
        } else {
            bool should_delete = false;
            if (root->updateNode(y, should_delete, current_size)) {
                // y foi aceito => insere na árvore
                if(should_delete){
                    delete root;
                    root = new NDNode();
                    root->is_leaf = true;
                    root->solutions.push_back(y);
                    root->updateIdealNadir(y);
                    current_size = 1;
                }else{
                    if(current_size < max_size){
                        
                        root->insert(y, max_children, max_solutions);
                        current_size++;
                    }
                }
            }
        }
    }    
    void NDTree::printTree(){
        if (root){
            root->printSolutions();
        }
    }
