#ifndef BOUNDEDPARETOSET_H
#define BOUNDEDPARETOSET_H

#include "structures.h"
#include <list>
#include <algorithm>

class ParetoSet {
protected:
    std::list<Solution*> sol;
    // ... other members ...

public:
    ParetoSet();
    virtual ~ParetoSet();
    virtual bool adicionarSol(Solution* s);
    Solution getSolucao(int k);
    int getSize();
    void clear();
    // ... other methods ...
};

class BoundedParetoSet : public ParetoSet {
private:
    static const int MAXARCSIZE = 1000;

public:
    BoundedParetoSet();
    virtual ~BoundedParetoSet();
    virtual bool adicionarSol(Solution* s) override;
};

#endif // BOUNDEDPARETOSET_H 