#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <sys/stat.h>
#include <iostream>

#include <chrono>
#include <fstream>
#include <unordered_map>

#include "SS.h"
#include "OO_FPI.h"
#include "OO_PE.h"
#include "Ours.h"
#include "hash.h"
#include "WS.h"
#include "Ours_SIMD.h"
#include "CM.h"


template<typename DATA_TYPE,typename COUNT_TYPE>
class BenchMark{
public:

    typedef std::vector<Abstract<DATA_TYPE, COUNT_TYPE>*> AbsVector;
    typedef std::unordered_map<DATA_TYPE, COUNT_TYPE> HashMap;

    vector<DATA_TYPE> packets;

    BenchMark(const char* _PATH, const COUNT_TYPE _W, const int _M):
            PATH(_PATH){
        W = _W;

        M = _M;

        FILE* file = fopen(PATH, "rb");

        COUNT_TYPE number = 0;
        DATA_TYPE item;
        HashMap record;

        TOTAL = 0;
        T = 0;

        int max = 0;

        char tmp[105];
        while(fread(&tmp, data_size, 1, file) > 0){
            tmp[data_size] = '\0';
            memcpy(&item, tmp, 4);

            packets.push_back(item);
            if(number % W == 0)
                T += 1;
            number += 1;

            if(record[item] != T){
                record[item] = T;
                mp[item] += 1;
                TOTAL += 1;
                if(mp[item] > max) max = mp[item];
            }
        }
        packet_num = number;
        fclose(file);

        HIT = T * 0.2;

        printf("Memory = %dKB\nWindow length = %d\nWindows cnt = %d\nHIT threshold = %d\n\n", M, W, T, HIT);
    }

    void SketchError(){
        std::cout << "PE exam begin" << std::endl;
        AbsVector PEs = {
                new OO_PE<DATA_TYPE, COUNT_TYPE>(3, M * 1024 / 3.0 / (BITSIZE + sizeof(COUNT_TYPE))),
                new WS<DATA_TYPE, COUNT_TYPE, 8, 16, false>(M),
                new Ours<DATA_TYPE, COUNT_TYPE, 2, 1, 5>(M, 0.85, 100, 25, 85),
                new Ours_SIMD<DATA_TYPE, COUNT_TYPE, 2, 1, 5>(M, 0.85, 100, 25, 85),
                new CM<DATA_TYPE, COUNT_TYPE>(4, M * 1024 / 32),
        };

        BenchInsert(PEs);

        for(auto PE : PEs){
            PECheckError(PE);
            delete PE;
        }
        std::cout << "PE exam end\n" << std::endl;
    }

    void TopKError(){
        std::cout << "FPI exam begin" << std::endl;
        AbsVector FPIs = {
                new OO_FPI<DATA_TYPE, COUNT_TYPE, 8>(M * 1024),
                new WS<DATA_TYPE, COUNT_TYPE, 8, 16, true>(M),
                new SS<DATA_TYPE, COUNT_TYPE>(2.0 / T / (15.0 / M)),
                new Ours<DATA_TYPE, COUNT_TYPE, 3, 3, 1>(M, 0.6, HIT > 300 ? 255 : HIT - 25 - 20, 25, 85),
                new Ours_SIMD<DATA_TYPE, COUNT_TYPE, 3, 3, 1>(M, 0.6, HIT > 300 ? 255 : HIT - 25 - 20, 25, 85),
        };

        BenchInsert(FPIs);
        for(auto FPI : FPIs){
            FPICheckError(FPI, HIT);
            delete FPI;
        }
        std::cout << "FPI exam end\n" << std::endl;
    }

    void Thp(){
        AbsVector FPIs = {
                new OO_PE<DATA_TYPE, COUNT_TYPE>(3, M * 1024 / 3.0 / (BITSIZE + sizeof(COUNT_TYPE))),
                new WS<DATA_TYPE, COUNT_TYPE, 8, 16, false>(M),
                new CM<DATA_TYPE, COUNT_TYPE>(4, M * 1024 / 32),
                new SS<DATA_TYPE, COUNT_TYPE>(2.0 / T / (15.0 / M)),
                new Ours<DATA_TYPE, COUNT_TYPE, 2, 1, 5>(M, 0.85, 100, 25, 85),
                new Ours_SIMD<DATA_TYPE, COUNT_TYPE, 2, 1, 5>(M, 0.85, 100, 25, 85),
        };

        for(auto FPI : FPIs){
            InsertThp(FPI);
            delete FPI;
        }
    }

private:
    int M;
    int data_size = 13;
    unordered_map<string, double> insert_through;
    int test_cycle = 1;

    double TOTAL;
    COUNT_TYPE T;
    COUNT_TYPE W;
    uint32_t packet_num;
    COUNT_TYPE HIT;

    HashMap mp;
    const char* PATH;

    typedef std::chrono::high_resolution_clock::time_point TP;

    inline TP now(){
        return std::chrono::high_resolution_clock::now();
    }

    inline double durations(TP finish, TP start){
        return std::chrono::duration_cast<std::chrono::duration<double,std::ratio<1,1000000>>>(finish - start).count();
    }

    void BenchInsert(AbsVector sketches){
        for(auto sketch : sketches){
            TP start, end;
            double new_window_time = 0.0;
            start = now();
            for(int j = 0; j < test_cycle; j++){
                sketch->reset();
                COUNT_TYPE windowId = 0;
                for (int i = 0; i < packet_num; i++){
                    if (i % W == 0) {
                        windowId += 1;
                        new_window_time += newWindow(sketch, windowId);
                    }
                    sketch->Insert(packets[i], windowId);
                }
                new_window_time += newWindow(sketch, windowId);
            }
            end = now();
            insert_through[sketch->getName()] = (double) test_cycle * packet_num / (durations(end, start) - new_window_time);
        }
    }

    double newWindow(Abstract<DATA_TYPE, COUNT_TYPE>* sketch, COUNT_TYPE windowId){
        TP window_start, window_end;
        window_start = now();
        sketch->NewWindow(windowId);
        window_end = now();
        return durations(window_end, window_start);
    }

    void InsertThp(Abstract<DATA_TYPE, COUNT_TYPE>* sketch){
        TP start, end;
        double new_window_time = 0.0;
        int windowId = 0;
        start = now();
        for(int j = 0; j < test_cycle; j++){
            sketch->reset();
            for (int i = 0; i < packet_num; i++){
                if (i % W == 0) {
                    windowId += 1;
                    new_window_time += newWindow(sketch, windowId);
                }
                sketch->Insert(packets[i], windowId);
            }
            new_window_time += newWindow(sketch, windowId);
        }
        end = now();
        std::cout << sketch->getName() << " Thp: " << (double) test_cycle * packet_num / (durations(end, start) - new_window_time) << std::endl;
    }

    void FPICheckError(Abstract<DATA_TYPE, COUNT_TYPE>* sketch, COUNT_TYPE HIT){
        double f1 = 0, are = 0;
        double tp = 0, fn = 0, fp = 0, tn = 0;
        double precision = 0, recall = 0, fnr = 0, fpr = 0;

        for(auto it = mp.begin();it != mp.end();++it){
            COUNT_TYPE predict = sketch->Query(it->first);
            COUNT_TYPE real = it->second;

            if(real > HIT){
                if(predict > HIT){
                    tp++;
                    are += (double) abs(real - predict) / real * 1.0;
                }else{
                    fn++;
                }
            }else{
                if(predict > HIT){
                    fp++;
                }else{
                    tn++;
                }
            }
        }

        if(tp <= 0){
            std::cout << "Not Find Real Persistent" << std::endl;
        }else{
            are /= tp;
        }

        precision = tp / (tp + fp + 0.0);
        recall = tp / (tp + fn + 0.0);
        f1 = 2 * precision * recall / (precision + recall);
        fnr = fn / (fn + tp);
        fpr = fp / (fp + tn);

        std::cout << sketch->getName() << ":" << std::endl;
        std::cout << "\tprecision " << precision << std::endl
                  << "\trecall " << recall << std::endl
                  << "\tf1_score " << f1 << std::endl
                  << "\tARE " << are << std::endl
                  << "\tFNR " << fnr << std::endl
                  << "\tFPR " << fpr << std::endl;

        ofstream outFile(R"(.\output.csv)",std::ios::app);
        outFile << sketch->getName() << "," << M << "," << W << "," << precision
                << "," << are << "," << recall << "," << f1 << "," << fnr << "," << fpr << "\n";
        outFile.close();
    }

    void PECheckError(Abstract<DATA_TYPE, COUNT_TYPE>* sketch){
        double aae = 0, are = 0;

        for(auto it = mp.begin();it != mp.end();++it) {
            COUNT_TYPE per = sketch->Query(it->first);
            COUNT_TYPE real = it->second;
            aae += abs(real - per);
            are += abs(real - per) / (real + 0.0);
        }

        ofstream outFile(R"(./output.csv)",std::ios::app);
        outFile << sketch->getName() << "," << M << "," << W << "," << aae / mp.size() << ","
                << are / mp.size() << "," << insert_through[sketch->getName()] << "\n";
        outFile.close();
    }
};

#endif //BENCHMARK_H
