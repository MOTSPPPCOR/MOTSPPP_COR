#ifndef PARETOSET_CPP
#define PARETOSET_CPP

#include <list>
#include <map>
#include "main.h"
#include <math.h>
#include "param.h"
#include "Grid.cpp"

using namespace std;

typedef struct {
	double min, max;
} range;



class ParetoSet {
	protected:
	list<Solution *> sol;
	range rangeNovo[3], rangeAtual[3];
	//int frequencia[NUMEROVERTICES][NUMEROVERTICES];
	Grid g;
	

	//calculate the position on a grid for the solution based on the objective values
	int calcularGridPos(Solution &s) {
		int bit = 0;
		int gridPos = 0;
		//print_solution(s);
		for (int obj=0;obj<NUMOBJETIVOS;obj++) {
			double inicio = rangeAtual[obj].min, fim = rangeAtual[obj].max, meio = (inicio+fim)/2.0;
			for (int k=0;k<PROFUNDIDADEGRID;k++) {
				if (getObj(s,obj) >= meio) { //3 ifs for each of the obj
					gridPos |= (1 << bit); //shifts the number 1 to the left by the number of positions specified by "bit". binary operation, basically
					inicio = meio;
				} else {
					fim = meio;
				}
				meio = (inicio+fim)/2.0;
				bit++;
			}
		}

		return gridPos;
	}

	void updateGrid() {
		g.clearGrid();

		list<struct Solution *>::iterator it = sol.begin();
		reiniciarRanges(); // leave the initial settings of -1 and infinity
		while (it != sol.end()) {
			for (int k=0;k<NUMOBJETIVOS;k++) { //Updates the ranges of all objectives
				rangeAtual[k].min = rangeNovo[k].min = min(rangeAtual[k].min,getObj(**it,k));
				rangeAtual[k].max = rangeNovo[k].max = max(rangeAtual[k].max,getObj(**it,k));
			}
			it++;
		}

		it = sol.begin();
		while (it != sol.end()) {
			g.addGrid( calcularGridPos(**it) );
			it++;
		}
	}

	void reiniciarRanges() {
		#define INF 1e9
		for (int k=0;k<NUMOBJETIVOS;k++) {
			rangeAtual[k].min = rangeNovo[k].min = INF;
			rangeAtual[k].max = rangeNovo[k].max = -INF;
		}
		#undef INF
	}

	public:
	ParetoSet() {
		reiniciarRanges();
		//memset(frequencia,0,sizeof(frequencia));
	}
	virtual ~ParetoSet() {
		clear();
	}
	
	Objectives get_objectives(int k){
		Objectives objetivos;
		list<struct Solution *>::iterator i = sol.begin();
		std::advance(i, k); 
		objetivos.cost = (**i).cost;
   		objetivos.time = (**i).time;
   		objetivos.total_bonus = (**i).total_bonus;
		return objetivos;
	}

	int get_size(){
		return sol.size();
	}

	Solution get_solution(int k){
		list<struct Solution *>::iterator i = sol.begin();
		std::advance(i, k); 
		Solution solution = (**i);
		return solution;
	}

	int getPositionCount(struct Solution &s) {
		//print_solution(s);
		return g.getPositionCount( calcularGridPos(s) );
	}

	int getPositionCount(int p) {
		return g.getPositionCount( p );
	}

	list<struct Solution *> getElementos() {
		return sol;
	}

	/* Complexity: O(n) */
	virtual bool adicionarSol(struct Solution *s) {
	    ASS ( assert( confereGrid() ); )

		/* goes through the vector of solutions and values ​​and, if there is a dominated solution, removes it and returns true. otherwise, return false */
		list<struct Solution *>::iterator i = sol.begin();
		list< list<struct Solution *>::iterator > remover;
		while (i != sol.end()) {
			if (x_dominates_y(*s , **i)) {
				remover.push_back(i);
			}
			if (x_dominates_y(**i,*s) || ((*i)->cost == s->cost && (*i)->time == s->time && (*i)->total_bonus == s->total_bonus)) //Testing whether the solution to be inserted is dominated or equal to any existing one
				return false;
			i++;
		}

		list< list<struct Solution  *>::iterator >::iterator j = remover.begin();
		while (j != remover.end()) {
		    // removes from grid
			g.removeGrid( calcularGridPos(***j) );
			// removes the frequency of the edges

			delete( **j );
			// remove from pareto set
			sol.erase( *j );
			j++;
		}

		Solution *t = new Solution(); //the following lines copy things correctly
		t->cost= s->cost;
		t->time = s->time;
		t->total_bonus = s->total_bonus;
		for(int vertice =0; vertice < s->route.size();vertice++){
			t->route.push_back(s->route[vertice]);
		}
		for(int city = 0; city  < s->cities_colected.size(); city ++){
			t->cities_colected.push_back(s->cities_colected[city ]);
		}
		for(int passenger = 0; passenger < s->passengers_riding.size(); passenger++){
			t->passengers_riding.push_back(s->passengers_riding[passenger]);
		}// add to pareto set
		sol.push_front( t );
		// add the frequency of the edges of the solution
		//addFrequency( t );
		// add to grid
		g.addGrid( calcularGridPos(*t) );

		for(int k=0;k<NUMOBJETIVOS;k++) {
			rangeNovo[k].min = min(rangeNovo[k].min,getObj(*t,k));
			rangeNovo[k].max = max(rangeNovo[k].max,getObj(*t,k));
		}

		// if there was a big change in the ranges (greater than 10% value), update the grid
		for (int k=0;k<NUMOBJETIVOS;k++) {
			if (fabs(rangeNovo[k].min-rangeAtual[k].min) > 0.1*rangeAtual[k].min || fabs(rangeNovo[k].max-rangeAtual[k].max) > 0.1*rangeAtual[k].max) {
				//fprintf(stderr,"Atualizando grid!\n");
				updateGrid();
				break;
			}
		}

        ASS ( assert( confereGrid() ); )
		return true;
	}


	int getSize() {
		return sol.size();
	}

	Solution *getSolucao(int p) {
		int c = 0;
		list<Solution *>::iterator i = sol.begin();
		while (i != sol.end()) {
			if (c == p) return *i;
			i++;
			c++;
		}
		return NULL;
	}

	void clear() {
		list<Solution *>::iterator i = sol.begin(), j;
		while (i != sol.end()) {
			j = i;
			i++;
			delete (*j);
		}
		sol.clear();
		g.clearGrid();
	}

	bool confereGrid() {
	    unsigned s = 0;
	    for (int i=0;i<g.getSize();i++) s += g.getPositionCount(i);
	    return s == sol.size();
	}
};

#endif