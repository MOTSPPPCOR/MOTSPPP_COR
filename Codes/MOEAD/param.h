#ifndef ZG_H
#define ZG_H

/*DEBUG options */
//#define DEBUG
#ifdef DEBUG
#define D(x) x
#endif
#ifndef DEBUG
#define D(x)
#endif

//#define ASSERT
#ifdef ASSERT
#define ASS(x) x
#endif
#ifndef ASSERT
#define ASS(x)
#endif

#define f(k,i,j) custos[k][i][j]
#define EPS 1e-9

#define NUMOBJETIVOS 3



#define NUMEROVERTICES 600

#define NUMEROARESTAS (NUMEROVERTICES-1)
#define PROFUNDIDADEGRID 5


#endif