#include "param.h"
#include <cstring>

class Grid {
	private:
	const static int tam = 1<<(NUMOBJETIVOS*PROFUNDIDADEGRID);
	int grid[1<<(NUMOBJETIVOS*PROFUNDIDADEGRID)];
	public:
	int getPositionCount(int p) {
		if (p < 0 || p >= tam) return -1;
		return grid[p];
	}
	void addGrid(int p) {
		grid[p]++;
	}
	void removeGrid(int p) {
		grid[p]--;
	}
	void clearGrid() {
		memset(grid,0,sizeof(grid[0])*tam);
	}
	int getSize() {
	    return tam;
	}
};