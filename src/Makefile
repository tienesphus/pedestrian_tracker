
CFLAGS = -std=c++11 -Wfatal-errors

OPENCV = `pkg-config --cflags --libs opencv4`

buscount: utils.o world.o detection.o tracker.o buscount.o
	g++ ${CFLAGS} -o buscount buscount.o tracker.o detection.o world.o utils.o ${OPENCV} -ltbb

buscount.o: buscount.cpp tracker.hpp detection.hpp world.hpp utils.hpp
	g++ ${CFLAGS} -c buscount.cpp ${OPENCV} -ltbb

tracker.o: tracker.cpp tracker.hpp detection.hpp utils.hpp world.hpp
	g++ ${CFLAGS} -c tracker.cpp ${OPENCV}

world.o: world.cpp world.hpp utils.hpp
	g++ ${CFLAGS} -c world.cpp ${OPENCV}

detection.o: detection.cpp detection.hpp
	g++ ${CFLAGS} -c detection.cpp ${OPENCV}

utils.o: utils.cpp utils.hpp
	g++ ${CFLAGS} -c utils.cpp ${OPENCV}

clean:
	rm *.o
	rm buscount
