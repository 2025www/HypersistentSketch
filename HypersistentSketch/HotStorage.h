#ifndef BENCH_HOTSTORAGE_H
#define BENCH_HOTSTORAGE_H
#define SLOT 3
#include "bitset.h"
#include "Abstract.h"

template<typename DATA_TYPE,typename COUNT_TYPE>
class HotStorage{
public:

    HotStorage(uint32_t _memory, double ratio){

        buckets = new Bucket *[2];
        bitsets = new BitSet *[2];
        length = (double) _memory * ratio * 1024 / 2 / sizeof(Bucket);
        for (uint32_t i = 0; i < 2; ++i) {
            buckets[i] = new Bucket[length];
            bitsets[i] = new BitSet(length * SLOT);
            memset(buckets[i], 0, length * (sizeof(Bucket)));
        }
    }

    ~HotStorage(){
        for(uint32_t i = 0;i < 2;++i){
            delete [] buckets[i];
            delete bitsets[i];
        }
        delete [] buckets;
        delete [] bitsets;
    }

    void Insert(const DATA_TYPE item, const COUNT_TYPE window) {
        uint32_t hash = Hash::BOBHash32((uint8_t*)&item, sizeof(DATA_TYPE), 0) % (length * (length - 1));
        uint32_t hash0 = hash % length, hash1 = hash / length;

        int min = INT32_MAX, minPos = -1, minEntryNum = -1;

        for(int i = 0; i < SLOT; i++) {
            if (buckets[0][hash0].item[i] == item) {
                buckets[0][hash0].count[i] += (!bitsets[0]->SetNGet(length * i + hash0));
                return;
            } else if (buckets[0][hash0].count[i] < min) {
                min = buckets[0][hash0].count[i];
                minPos = 0;
                minEntryNum = i;
            }
        }
        for(int i = 0; i < SLOT; i++) {
            if(buckets[1][hash1].item[i] == item){
                buckets[1][hash1].count[i] += (!bitsets[1]->SetNGet(length * i + hash1));
                return;
            }else if(buckets[1][hash1].count[i] < min){
                min = buckets[1][hash1].count[i];
                minPos = 1;
                minEntryNum = i;
            }
        }
        if(minPos != -1) {
            int pos = minPos == 0 ? hash0 : hash1;
            if (hash % (min + 1) == 0) {
                buckets[minPos][pos].item[minEntryNum] = item;
                buckets[minPos][pos].count[minEntryNum] += (!bitsets[minPos]->SetNGet(length * minEntryNum + pos));
            }
        }
    }

    COUNT_TYPE Query(const DATA_TYPE item){
        uint32_t hash = Hash::BOBHash32((uint8_t*)&item, sizeof(DATA_TYPE), 0) % (length * (length - 1));
        uint32_t hash0 = hash % length, hash1 = hash / length;
        int min = INT32_MAX;
        for(int i = 0; i < SLOT; i++){
            if(buckets[0][hash0].item[i] == item){
                min = MIN(buckets[0][hash0].count[i], min);
            }
            if(buckets[1][hash1].item[i] == item){
                min = MIN(buckets[1][hash1].count[i], min);
            }
        }
        return min == INT32_MAX ? 0 : min;
    }

    void NewWindow(const COUNT_TYPE window){
        for(int i = 0; i < 2; i++){
            bitsets[i]->Clear();
        }
    }

    void reset(){
        for (uint32_t i = 0; i < 2; ++i) {
            memset(buckets[i], 0, length * (sizeof(Bucket)));
            bitsets[i]->Clear();
        }
    }

private:
    uint32_t length;

    BitSet** bitsets;
    struct Bucket {
        DATA_TYPE item[SLOT];
        COUNT_TYPE count[SLOT];
    };
    Bucket ** buckets;
};

#endif //BENCH_HOTSTORAGE_H
