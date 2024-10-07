#ifndef BENCH_OURS_H
#define BENCH_OURS_H

#include "Abstract.h"
#include "Filter.h"
#include "HotStorage.h"
#include "hash.h"

template<typename DATA_TYPE,typename COUNT_TYPE, int d1, int d2, int cache_size>
class Filters
{
public:

    Filters(int _memory_sum, double _memory_ratio, int l1_thres, int l2_thres, int _l1_ratio): memory_sum(_memory_sum)
    {
        memory_ratio = _memory_ratio;
        memory_in_bytes = memory_ratio * memory_sum * 1024 * 1.0;

        l1_threshold = l1_thres;
        l2_threshold = l2_thres;
        l1_ratio = _l1_ratio;

        remained = memory_in_bytes - cache_max_size_in_bytes;

        m1_in_bytes = int(remained * l1_ratio / 100.0);
        m2_in_bytes = int(remained * (100 - l1_ratio) / 100.0);
        L1 = new Filter<DATA_TYPE, uint8_t>(d1, m1_in_bytes / d1 / (BITSIZE + sizeof(uint8_t)));
        L2 = new Filter<DATA_TYPE, uint8_t>(d2, m2_in_bytes / d2 / (BITSIZE + sizeof(uint8_t)));
    }


    void init_L3(HotStorage<DATA_TYPE, COUNT_TYPE>* l3){
        this->L3 = l3;
    }

    ~Filters()
    {}

    void Insert(const DATA_TYPE item, const COUNT_TYPE window){
        uint32_t hash = item % bucket_num;
        for(int i = 0; i < entry_num; i++){
            if(ID[hash][i] == 0){
                ID[hash][i] = item;
                return;
            }else if(ID[hash][i] == item){
                return;
            }
        }
        insert(item);
    }

    void insert(const DATA_TYPE item){
        if(L1->Insert(item, l1_threshold)) return;

        if(L2->Insert(item, l2_threshold)) return;

        L3->Insert(item, 0);
    }

    int getThreshold(){
        return l2_threshold + l1_threshold;
    }

    COUNT_TYPE Query(const DATA_TYPE item){
        COUNT_TYPE v1 = L1->Query(item);
        if(v1 != l1_threshold) {
            return v1;
        }
        COUNT_TYPE v2 = L2->Query(item);
        return v1 + v2;
    }

    void clearCache(){
        for(auto & bucket : ID){
            for(unsigned int & item : bucket){
                if(item != 0){
                    insert(item);
                }
            }
            memset(bucket, 0, data_size * entry_num);
        }
    }

    void NewWindow(const COUNT_TYPE window){
        clearCache();
        L1->NewWindow(window);
        L2->NewWindow(window);
        L3->NewWindow(window);
    }

    void reset(){
        L1->reset();
        L2->reset();
        for(auto & bucket : ID){
            memset(bucket, 0, data_size * entry_num);
        }
    }

private:
    int memory_sum;
    double memory_ratio;
    int memory_in_bytes;
    int l1_threshold;
    int l2_threshold;
    int l1_ratio;

    int remained;
    int m1_in_bytes;
    int m2_in_bytes;

    Filter<DATA_TYPE, uint8_t>* L1;
    Filter<DATA_TYPE, uint8_t>* L2;
    HotStorage<DATA_TYPE, COUNT_TYPE>* L3;
    static constexpr int data_size = 4;
    static constexpr int cache_max_size_in_bytes = cache_size * 1024;

    static constexpr int entry_num = 16;
    static constexpr int bucket_num = cache_max_size_in_bytes / data_size / entry_num;
    uint32_t ID[bucket_num][entry_num];
};

template<typename DATA_TYPE,typename COUNT_TYPE, int d1, int d2, int cache_size>
class Ours : public Abstract<DATA_TYPE, COUNT_TYPE>{
public:

    Ours(int memorySum, double ratio, int l1_thres, int l2_thres, int l1_ratio): memory_sum(memorySum){
        filters = new Filters<DATA_TYPE, COUNT_TYPE, d1, d2, cache_size>(memorySum, ratio, l1_thres, l2_thres, l1_ratio);
        L3 = new HotStorage<DATA_TYPE, COUNT_TYPE>(memory_sum, 1 - ratio);
        filters->init_L3(L3);
    }

    ~Ours(){
    }

    void Insert(const DATA_TYPE item, const COUNT_TYPE window){
        filters->Insert(item, window);
    }

    COUNT_TYPE Query(const DATA_TYPE item){
        COUNT_TYPE ret = filters->Query(item);
        if(ret >= filters->getThreshold()){
            ret += L3->Query(item);
        }
        return ret;
    }

    void NewWindow(const COUNT_TYPE window){
        filters->NewWindow(window);
    }

    std::string getName(){
        return "Ours";
    }

    void reset(){
        filters->reset();
        L3->reset();
    }

private:
    Filters<DATA_TYPE, COUNT_TYPE, d1, d2, cache_size>* filters;
    HotStorage<DATA_TYPE, COUNT_TYPE>* L3;
    int memory_sum;
};
#endif //BENCH_OURS_H
