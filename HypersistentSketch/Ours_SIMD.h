#ifndef BENCH_OURS_SIMD_H
#define BENCH_OURS_SIMD_H

#include "bitset.h"
#include "Abstract.h"
#include "Filter.h"
#include "HotStorage.h"
template<typename DATA_TYPE,typename COUNT_TYPE, int d1, int d2, int cache_size>
class Filters_SIMD
{
public:

    Filters_SIMD(int _memory_sum, double _memory_ratio, int _l1_thres, int _l2_thres, int _l1_ratio): memory_sum(_memory_sum)
    {
        memory_ratio = _memory_ratio;
        memory_in_bytes = memory_ratio * memory_sum * 1024 * 1.0;

        l1_threshold = _l1_thres;
        l2_threshold = _l2_thres;
        l1_ratio = _l1_ratio;

        remained = memory_in_bytes - cache_max_size_in_bytes;

        m1_in_bytes = int(remained * l1_ratio / 100.0);
        m2_in_bytes = int(remained * (100 - l1_ratio) / 100.0);
        L1 = new Filter<DATA_TYPE, uint8_t>(d1, m1_in_bytes / d1 / (BITSIZE + sizeof(uint8_t)));
        L2 = new Filter<DATA_TYPE, uint8_t>(d2, m2_in_bytes / d2 / (BITSIZE + sizeof(uint8_t)));
        for(auto & bucket : ID){
            memset(bucket, 0, data_size * entry_num);
        }
    }


    void init_L3(HotStorage<DATA_TYPE, COUNT_TYPE>* l3){
        this->L3 = l3;
    }

    ~Filters_SIMD()
    {}

    void Insert(const DATA_TYPE item, const COUNT_TYPE window){
        int bucket_id = item % bucket_num;
        __m128i key = _mm_set1_epi32((int) item);

        int matched;
        if(entry_num == 16){
            __m128i *keys_p = (__m128i *)ID[bucket_id];

            __m128i a_comp = _mm_cmpeq_epi32(key, keys_p[0]);
            __m128i b_comp = _mm_cmpeq_epi32(key, keys_p[1]);
            __m128i c_comp = _mm_cmpeq_epi32(key, keys_p[2]);
            __m128i d_comp = _mm_cmpeq_epi32(key, keys_p[3]);

            a_comp = _mm_packs_epi32(a_comp, b_comp);
            c_comp = _mm_packs_epi32(c_comp, d_comp);
            a_comp = _mm_packs_epi32(a_comp, c_comp);

            matched = _mm_movemask_epi8(a_comp);
        }else{
            __m128i *keys_p = (__m128i *)ID[bucket_id];
            __m128i a_comp = _mm_cmpeq_epi32(key, keys_p[0]);
            matched = _mm_movemask_ps(*(__m128 *)&a_comp);
        }
        //匹配到，即已经存入L0
        if(matched != 0) {
            return;
        }
        //没匹配到，且桶没满
        int cur_pos_now = cur_pos[bucket_id];
        if (cur_pos_now != entry_num) {
            ID[bucket_id][cur_pos_now] = item;
            ++cur_pos[bucket_id];
            return;
        }
        //桶满了，直接插入L1
        insert(item);
    }

    void insert(DATA_TYPE item){
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
        memset(cur_pos, 0, bucket_num * sizeof(int));
    }

    void NewWindow(const COUNT_TYPE window){
        clearCache();
        L1->NewWindow(window);
        L2->NewWindow(window);
        L3->NewWindow(window);
    }

    std::string getName(){
        return "SC_SIMD";
    }

    void reset(){
        L1->reset();
        L2->reset();
        memset(cur_pos, 0, bucket_num * sizeof(int));
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
    int remained = memory_in_bytes;

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
    int cur_pos[bucket_num];
};

template<typename DATA_TYPE,typename COUNT_TYPE, int d1, int d2, int cache_size>
class Ours_SIMD : public Abstract<DATA_TYPE, COUNT_TYPE>{
public:

    Ours_SIMD(int memorySum, double ratio, int l1_thres, int l2_thres, int l1_ratio): memory_sum(memorySum){
        filters = new Filters_SIMD<DATA_TYPE, COUNT_TYPE, d1, d2, cache_size>(memorySum, ratio, l1_thres, l2_thres, l1_ratio);
        L3 = new HotStorage<DATA_TYPE, COUNT_TYPE>(memory_sum, 1 - ratio);
        filters->init_L3(L3);
    }

    ~Ours_SIMD(){
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
        return "Ours_SIMD";
    }

    void reset(){
        filters->reset();
        L3->reset();
    }

private:
    Filters_SIMD<DATA_TYPE, COUNT_TYPE, d1, d2, cache_size>* filters;
    HotStorage<DATA_TYPE, COUNT_TYPE>* L3;
    int memory_sum;
};
#endif //BENCH_OURS_SIMD_H
