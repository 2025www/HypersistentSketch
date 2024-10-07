#include "benchmark.h"

int main(int argc, char *argv[]) {

    char* dataset = "./data/caida.dat";
    int M = 25; //KB
    int W = 1500;
    int c;
    while((c = getopt(argc, argv, "d:m:w:")) != -1) {
        switch(c) {
            case 'd':
                strcpy(dataset, optarg);
                break;
            case 'm':
                M = atoi(optarg);
                break;
            case 'w':
                W = atoi(optarg);
                break;
        }
    }
    BenchMark<uint32_t, int32_t > benchMark(dataset, W, M);
    benchMark.SketchError();
    benchMark.TopKError();
    return 0;
}
