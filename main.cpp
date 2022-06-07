#include <opencv2/opencv.hpp>

// Fastflow Libraries
#include <ff/ff.hpp>
#include <ff/pipeline.hpp>
#include <ff/farm.hpp>
#include <ff/parallel_for.hpp>
#include <ff/map.hpp>

// Useful libraries
#include <iostream>
#include <chrono>
#include <thread>
#include <condition_variable>
#include <vector>
#include <queue>
#include <atomic>

// More readable code
using namespace std;	
using namespace ff;
using namespace cv;

#define ushort unsigned short
#define ulong unsigned long
#define DEFAULT_IMG Scalar(128)
#define VIDEOSOURCE "./videos/video2FULLHD.mp4"
#define ERROR_MSG(cond,msg) if(cond) { cout << msg << endl; exit(-1);}

#include <Utimer.cpp>
// Define a Shared queue
#include <Utils.cpp>

#include <Videodetect.cpp>
#include <Sequential.cpp> // Sequential program
#include <ThreadFarm.cpp> // Standard Thread program
#include <Fastflow_a.cpp> // Fastflow implementation (Normal form)
#include <Fastflow_b.cpp> // Fastflow implementation Map&ParallelFor

// Oss. It's better read first the report.

int main(int argc,char* argv[]) {

	ERROR_MSG(argc<6,"Wrong argument:\n\tVersion[\n\t\t0 = Sequential\n\t\t1 = Threads\n\t\t2 = Fastflow farm of Sequential node\n\t\t3 = Farm of map (+parallel for)]\n\tNumber of workers (n>0)\n\tKernel size(ksize>=3)\n\tPercentage(k>0 and k=<1)\n\tTime execution[ 0 = False| 1 = True]\n")

	int version = atoi(argv[1]); // Version
	int nw      = atoi(argv[2]); // Number of workers
	int ksize   = atoi(argv[3]); // Kernel size
	float k     = atof(argv[4]); // Percentage trigger
	int stat    = atoi(argv[5]); // Print Statistic

	if ( version == 0 ) { // Sequential approach
		Sequential s(VIDEOSOURCE,ksize,k);
		if(stat == 0) s.execute_to_result();
		else if (stat == 1) s.execute_to_stat();
		else if (stat == 2) s.execute_to_stat2();
		else exit(-1);		
	}

	if ( version == 1 ) { // Farm of thread, standard C++ thread implementation
		ThreadFarm s(VIDEOSOURCE,ksize,k,nw);
		if(stat == 0) s.execute_to_result();
		else if (stat == 1) s.execute_to_stat();
		else exit(-1);	
	}

	if ( version == 2 ) { // Farm of sequential node (Normal form)
		fastflow_a s(VIDEOSOURCE,ksize,k,nw);
		if(stat == 0) s.execute_to_result();
		else if (stat == 1) s.execute_to_stat();
		else exit(-1);		
	}

	if ( version == 3 ) { // Farm of pipeline of map-node
		fastflow_b s(VIDEOSOURCE,ksize,k,4,8,nw);
		if(stat == 0) s.execute_to_result();
		else if (stat == 1) s.execute_to_stat();
		else exit(-1);
	}
	return 0;
}