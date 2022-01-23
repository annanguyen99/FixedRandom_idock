CC=g++-9 -g -O0

bin/idock: obj/io_service_pool.o obj/safe_counter.o obj/array.o obj/atom.o obj/scoring_function.o obj/receptor.o obj/ligand.o obj/randomnumgen_numpycompatible.o obj/result.o obj/random_forest.o obj/random_forest_x.o obj/random_forest_y.o obj/main.o
	${CC} -o $@ $^ -pthread -lstdc++fs -L${BOOST_ROOT}/lib -lboost_program_options

obj/%.o: src/%.cpp
	${CC} -o $@ $< -c -std=c++17 -Wall -I${BOOST_ROOT}/include

clean:
	rm -f bin/idock obj/*.o