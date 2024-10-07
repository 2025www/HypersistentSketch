#ifndef BENCH_WS_H
#define BENCH_WS_H

/*
 * Waving-Sketch
 */

#include "bitset.h"
#include "Abstract.h"

#include <ctime>
#include <unistd.h>

using namespace std;
#define factor 1
template<typename DATA_TYPE, typename COUNT_TYPE, uint32_t slot_num, uint32_t counter_num, bool only_heavy>
class WS : public Abstract<DATA_TYPE, COUNT_TYPE> {
public:

    struct Bucket {
        DATA_TYPE items[slot_num];
        int counters[slot_num];
        int16_t incast[counter_num];

        void insert(const DATA_TYPE item, uint32_t seed_s, uint32_t seed_incast, bool& is_empty){
            uint32_t choice = Hash::BOBHash32((uint8_t*)&item, sizeof(DATA_TYPE), seed_s) & 1;
            uint32_t whichcast = Hash::BOBHash32((uint8_t*)&item, sizeof(DATA_TYPE), seed_incast) % counter_num;
            COUNT_TYPE min_num = INT_MAX;
            uint32_t min_pos = -1;
            is_empty = false;
            for (uint32_t i = 0; i < slot_num; ++i) {
                if (counters[i] == 0) {
                    items[i] = item;
                    counters[i] = -1;
                    is_empty = true;
                    return;
                }
                else if (items[i] == item) {
                    if (counters[i] < 0)
                        counters[i]--;
                    else {
                        counters[i]++;
                        incast[whichcast] += COUNT[choice];
                    }
                    return;
                }

                COUNT_TYPE counter_val = std::abs(counters[i]);
                if (counter_val < min_num) {
                    min_num = counter_val;
                    min_pos = i;
                }
            }

            if (incast[whichcast] * COUNT[choice] >= int(min_num * factor)) {
                if (counters[min_pos] < 0) {
                    uint32_t min_choice = Hash::BOBHash32((uint8_t*)&items[min_pos], sizeof(DATA_TYPE), seed_s) & 1;
                    incast[Hash::BOBHash32((uint8_t*)&items[min_pos], sizeof(DATA_TYPE), seed_incast) % counter_num] -=
                            COUNT[min_choice] * counters[min_pos];
                }
                items[min_pos] = item;
                counters[min_pos] = min_num + 1;
            }
            incast[whichcast] += COUNT[choice];
        }

        int Query(const DATA_TYPE item, uint32_t seed_s, uint32_t seed_incast) {
            uint32_t choice = Hash::BOBHash32((uint8_t*)&item, sizeof(DATA_TYPE), seed_s) & 1;
            uint32_t whichcast = Hash::BOBHash32((uint8_t*)&item, sizeof(DATA_TYPE), seed_incast) % counter_num;
            COUNT_TYPE retv = only_heavy ? 0 : std::max(incast[whichcast] * COUNT[choice], 0);

            for (uint32_t i = 0; i < slot_num; ++i) {
                if (items[i] == item) {
                    return std::abs(counters[i]);
                }
            }
            return retv;
        }
    };

    WS(uint64_t memory) : length(memory * 1024 * 8 / 2){
        bucket_num = (memory * 1024 - length * BITSIZE) / sizeof(Bucket);
        cnt = 0;
        bitset = new BitSet(length);
        buckets = new Bucket[bucket_num];
        memset(buckets, 0, bucket_num * sizeof(Bucket));
        seed_choice = Hash::generateRandomNumber();
        seed_incast = Hash::generateRandomNumber();
        seed_s = Hash::generateRandomNumber();
    }

    ~WS(){
        delete [] buckets;
    }

    std::string getName() {
        return "WS";
    }

    void Insert(const DATA_TYPE item, const COUNT_TYPE window){
        bool init = true;
        for(int i = 0; i < hash_num; i++){
            uint32_t pos = this->hash(item, i) % length;
            if(!bitset->Get(pos)){
                init = false;
                bitset->Set(pos);
            }
        }
        if(!init) Init(item);
    }

    void Init(const DATA_TYPE item){
        uint32_t bucket_pos = this->hash(item, seed_choice) % bucket_num;
        bool is_empty = 0;
        buckets[bucket_pos].insert(item, seed_s, seed_incast, is_empty);
        cnt += is_empty;
    }

    COUNT_TYPE Query(const DATA_TYPE item){
        uint32_t bucket_pos = this->hash(item, seed_choice) % bucket_num;
        return buckets[bucket_pos].Query(item, seed_s, seed_incast);
    }

    void NewWindow(const COUNT_TYPE window){
        bitset->Clear();
    }

    void reset(){
        bitset->Clear();
        memset(buckets, 0, bucket_num * sizeof(Bucket));
        seed_choice = Hash::generateRandomNumber();
        seed_incast = Hash::generateRandomNumber();
        seed_s = Hash::generateRandomNumber();
    }

private:
    const uint32_t length;
    const uint32_t hash_num = 3;

    Bucket* buckets;
    uint32_t bucket_num;
    BitSet* bitset;
    uint32_t seed_choice;
    uint32_t seed_s;
    uint32_t seed_incast;
    int cnt;
};

#endif //BENCH_WS_H
