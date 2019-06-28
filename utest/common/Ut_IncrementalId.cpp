#include "IncrementalId.h"
#include "../Test.h"
using namespace hudp;

void UtestCIncrementalId() {
    CIncrementalId id;

    while (id.GetNextId()) {
        
    }

    uint16_t a = 0xFFFF;
    uint16_t b = 1;
    uint16_t c = 0;

    Expect_True(CIncrementalId::IsNextId(a, c));
    Expect_False(CIncrementalId::IsBiggerThan(a, b));
    Expect_True(CIncrementalId::IsSmallerThan(a, b));

}