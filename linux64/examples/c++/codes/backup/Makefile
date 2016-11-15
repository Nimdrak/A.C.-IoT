CC = g++
TARGET = main
CFLAGS = -I../../../include/ -L../../../lib/ -lgurobi_c++ -lgurobi60 -lpthread -lm

CARGS = -m64 -g
INC = ../../../include/
CLIB = -L../../lib/ -lgurobi60
CPPLIB = -L../../../lib/ -lgurobi_c++ -lgurobi60

$(TARGET): main.o Normsinv.o Normcdf.o C_Phy_topology.o C_Request.o Phy_topology.o Request.o Graph.o YenTopKShortestPathsAlg.o DijkstraShortestPathAlg.o
	$(CC) $(CARGS) -o $@ $@.o Normsinv.o Normcdf.o C_Phy_topology.o C_Request.o Phy_topology.o Request.o Graph.o YenTopKShortestPathsAlg.o DijkstraShortestPathAlg.o -I$(INC) $(CPPLIB) -lpthread -lm

main.o: main.cpp Normsinv.cpp Normsinv.h C_Phy_topology.cpp C_Phy_topology.h C_Request.cpp C_Request.h Phy_topology.cpp Phy_topology.h Request.cpp Request.h Graph.cpp Graph.h GraphElements.h YenTopKShortestPathsAlg.cpp YenTopKShortestPathsAlg.h DijkstraShortestPathAlg.cpp DijkstraShortestPathAlg.h
	$(CC) $(CARGS) -c $*.cpp Normsinv.cpp C_Phy_topology.cpp C_Request.cpp Phy_topology.cpp Request.cpp Graph.cpp YenTopKShortestPathsAlg.cpp DijkstraShortestPathAlg.cpp -I$(INC) $(CPPLIB) -lpthread -lm

Normsinv.o: Normsinv.cpp Normsinv.h
	$(CC) $(CARGS) -c $*.cpp -I$(INC) $(CPPLIB) -lpthread -lm

Normcdf.o: Normcdf.cpp Normcdf.h
	$(CC) $(CARGS) -c $*.cpp -I$(INC) $(CPPLIB) -lpthread -lm

C_Phy_topology.o: C_Phy_topology.cpp C_Phy_topology.h Normsinv.cpp Normsinv.h
	$(CC) $(CARGS) -c $*.cpp Normsinv.cpp -I$(INC) $(CPPLIB) -lpthread -lm

C_Request.o: C_Request.cpp C_Request.h
	$(CC) $(CARGS) -c $*.cpp -I$(INC) $(CPPLIB) -lpthread -lm

Phy_topology.o: Phy_topology.cpp Phy_topology.h Normsinv.cpp Normsinv.h Normcdf.cpp Normcdf.h
	$(CC) $(CARGS) -c $*.cpp Normsinv.cpp Normcdf.cpp -I$(INC) $(CPPLIB) -lpthread -lm

Request.o: Request.cpp Request.h
	$(CC) $(CARGS) -c $*.cpp -I$(INC) $(CPPLIB) -lpthread -lm

YenTopKShortestPathsAlg.o: YenTopKShortestPathsAlg.cpp YenTopKShortestPathsAlg.h
	$(CC) $(CARGS) -c $*.cpp -I$(INC) $(CPPLIB) -lpthread -lm

DijkstraShortestPathAlg.o: DijkstraShortestPathAlg.cpp DijkstraShortestPathAlg.h
	$(CC) $(CARGS) -c $*.cpp -I$(INC) $(CPPLIB) -lpthread -lm

Graph.o: Graph.cpp Graph.h GraphElements.h
	$(CC) $(CARGS) -c $*.cpp -I$(INC) $(CPPLIB) -lpthread -lm

clean:
	rm -f main.o C_Phy_topology.o C_Request.o Phy_topology.o Request.o Graph.o YenTopKShortestPathsAlg.o DijkstraShortestPathAlg.o
