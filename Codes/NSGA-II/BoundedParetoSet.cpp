#ifndef BOUNDEDPARETOSET_CPP
#define BOUNDEDPARETOSET_CPP

#include "ParetoSet.cpp"
#include <math.h>
#include "main.h"
#include <cstdio>
#include <string>
#include <vector> 
#include <iostream>
using namespace std;


using namespace std;

class BoundedParetoSet : public ParetoSet {
	private:
	const static int MAXARCSIZE = 300;
	// FILE *globalf;
    // string nomeglobalf;
	
	public:
	/* Complexidade: O(n) */
	BoundedParetoSet () {
	}
	~BoundedParetoSet () {

	}



	//if the added solution is dominated, it is not added and the function returns false. If added, returns true.
	bool adicionarSol(Solution *s) {
		ASS ( assert( confereGrid() ); )
		list<Solution *>::iterator maisPopuloso = sol.begin();
		int maiorPositionCount = -1;
		//print_solution(*s);
		if (sol.size() > 0) {maiorPositionCount = getPositionCount( **sol.begin() );}
		// goes through the vector of solutions and values ​​and, if there is a dominated solution, removes it and returns true. otherwise, returns false
		list<Solution *>::iterator i = sol.begin();
		list< list<Solution *>::iterator > remover;
		while (i != sol.end()) {
			// if the solution that will enter dominates the solution i that is already in the set
			if (x_dominates_y(*s , **i)) {
				remover.push_back(i);
			}
			// if the solution that will be entered does not dominate solution i, we look for the solution that is in most populated location, if the pareto is at maximum size
			if (remover.size() == 0 && getSize()+1 > MAXARCSIZE) {
				int positionCountAtual = getPositionCount(**i);
				if (maiorPositionCount < positionCountAtual) {
					maiorPositionCount = positionCountAtual;
					maisPopuloso = i;
				}
			}
			
			if (x_dominates_y(**i,*s) || ((*i)->cost == s->cost && (*i)->time == s->time && (*i)->total_bonus == s->total_bonus))
				return false;
			i++;
		}
		// if the solution that will be entered does not dominate any and the size of the pareto set is already at its maximum
		// (if no solution will leave the set), remove the most populated one
		if (remover.size() == 0 && getSize()+1 > MAXARCSIZE) {
			remover.push_back(maisPopuloso);
		}
		//fprintf(stderr,"getSize = %d %d\n",getSize(),sol.size());
			
		list< list<Solution *>::iterator >::iterator j = remover.begin();
		while (j != remover.end()) {
			// remove from grid
			g.removeGrid( calcularGridPos(***j) );
			// remove the frequency of the edges
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
		}
		// add to pareto set
		sol.push_front( t );
		if (sol.size() > MAXARCSIZE) fprintf(stderr,"ERRO!\n");
		// add the frequency of the edges of the solution
		// add to grid
		g.addGrid( calcularGridPos(*t) );
		for(int k=0;k<NUMOBJETIVOS;k++) {
			rangeNovo[k].min = min(rangeNovo[k].min,getObj(*t,k));
			rangeNovo[k].max = max(rangeNovo[k].max,getObj(*t,k));
		}

		// if there was a big change in the ranges (greater than 10% value), update the grid
		for (int k=0;k<NUMOBJETIVOS;k++) {
			if (fabs(rangeNovo[k].min-rangeAtual[k].min) > 0.1*rangeAtual[k].min || fabs(rangeNovo[k].max-rangeAtual[k].max) > 0.1*rangeAtual[k].max) {
				updateGrid();
				break;
			}
		}

		ASS ( assert( confereGrid() ); )
		return true;
	}
};

#endif