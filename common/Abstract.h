#ifndef ABSTRACT_H
#define ABSTRACT_H

#include "hash.h"

static const int COUNT[2] = { 1, -1 };
template<typename DATA_TYPE,typename COUNT_TYPE>
class Abstract{
public:
    Abstract(){}
    virtual ~Abstract(){};

    virtual void Insert(const DATA_TYPE item, const COUNT_TYPE window) = 0;
    virtual COUNT_TYPE Query(const DATA_TYPE item) = 0;
    virtual void NewWindow(const COUNT_TYPE window) = 0;
    virtual std::string getName() = 0;
    virtual void reset() = 0;

    inline uint32_t hash(DATA_TYPE data) {
        return Hash::BOBHash32((uint8_t*)&data, sizeof(DATA_TYPE), 0);
    }

    inline uint32_t hash(DATA_TYPE data, uint32_t seed){
        return Hash::BOBHash32((uint8_t*)&data, sizeof(DATA_TYPE), seed);
    }

    inline uint64_t hash64(DATA_TYPE data, uint32_t seed){
        return Hash::BOBHash64((uint8_t*)&data, sizeof(DATA_TYPE), seed);
    }
};

#endif //ABSTRACT_H
