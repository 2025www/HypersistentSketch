#ifndef BENCH_FILTER_H
#define BENCH_FILTER_H

#include "bitset.h"
#include "Abstract.h"

template<typename DATA_TYPE,typename COUNT_TYPE>
class Filter{
public:

    Filter(uint32_t _hash_num, uint32_t _length):
            hash_num(_hash_num), length(_length){
        counters = new COUNT_TYPE* [hash_num];
        bitsets = new BitSet* [hash_num];
        for(uint32_t i = 0;i < hash_num;++i){
            counters[i] = new COUNT_TYPE [length];
            bitsets[i] = new BitSet(length);
            memset(counters[i], 0, length * sizeof(COUNT_TYPE));
        }
        hash_seed = new int[hash_num];
        for (int i = 0; i < hash_num; i++){
            hash_seed[i] = Hash::generateRandomNumber();
        }
    }

    ~Filter(){
        for(uint32_t i = 0;i < hash_num;++i){
            delete [] counters[i];
            delete bitsets[i];
        }
        delete [] counters;
        delete [] bitsets;
    }

    COUNT_TYPE Query(const DATA_TYPE item){
        int ret = INT32_MAX;
        for(uint32_t i = 0;i < hash_num;++i){
            uint32_t pos = Hash::BOBHash32((uint8_t*)&item, sizeof(DATA_TYPE), hash_seed[i]) % length;
            ret = MIN(ret, counters[i][pos]);
        }
        return ret;
    }

    bool Insert(const DATA_TYPE item, COUNT_TYPE threshold){
        int pos[hash_num];
        int min = INT32_MAX;
        bool flag = false;//todo 改成是否有插入 has_inserted

        for(uint32_t i = 0; i < hash_num; i++){
            pos[i] = Hash::BOBHash32((uint8_t*)&item, sizeof(DATA_TYPE), hash_seed[i]) % length;
            int cur = counters[i][pos[i]];
            min = MIN(cur, min);
            if(!bitsets[i]->Get(pos[i])) flag = true; //有未插入的
        }

        //当前窗口已经出现
        if(!flag) return true;

        //大于当前阈值，不插入，给后面插入，并且把窗口置为off
        if(min >= threshold) {
            for(int i = 0; i < hash_num; i++){
                bitsets[i]->Set(pos[i]);
            }
            return false;
        }

        for(int i = 0; i < hash_num; i++){
            if(counters[i][pos[i]] == min){
                counters[i][pos[i]] += (!bitsets[i]->SetNGet(pos[i]));
            }
        }
        return true;
    }

    void NewWindow(const COUNT_TYPE window){
        for(uint32_t i = 0;i < hash_num;++i){
            bitsets[i]->Clear();
        }
    }

    void reset(){
        for(uint32_t i = 0;i < hash_num;++i){
            bitsets[i]->Clear();
            memset(counters[i], 0, length * sizeof(COUNT_TYPE));
            hash_seed[i] = Hash::generateRandomNumber();
        }
    }

private:
    const uint32_t hash_num;
    const uint32_t length;

    BitSet** bitsets;
    COUNT_TYPE** counters;
    int* hash_seed;
};

#endif //BENCH_FILTER_H
