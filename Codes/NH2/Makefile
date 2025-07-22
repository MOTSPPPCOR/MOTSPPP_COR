PLATAFORM 	= linux64
INC			= /home/{{user_path}}/gurobi1101/linux64/include/
CLIB		= -L/home/{{user_path}}/gurobi1101/linux64/lib/ -lgurobi110
CPPLIB		= -L/home/{{user_path}}/gurobi1101/linux64/lib/ -lgurobi_c++ $(CLIB)

version = 2

all: modelo

compile: run_modelo

modelo: /home/{{user_path}}/mestrado/gurobi/naives/v$(version)/
	g++ -m64 -g /home/{{user_path}}/mestrado/gurobi/naives/v$(version)/index.cpp -o exec -I$(INC) $(CPPLIB) -lpthread -lm

run_modelo: modelo
	./exec

clear:
	rm -f *.o exec

run: compile clear 