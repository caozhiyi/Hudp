#ifndef HEADER_COMMON_CBITSTREAMPOOL
#define HEADER_COMMON_CBITSTREAMPOOL

#include "CommonType.h"
#include "BitStream.h"
#include "TSQueue.h"
#include "Single.h"

namespace hudp {
    
    // pool size at initialization.
    static const uint16_t __expand_pool_size = 100;
    class CBitStreamPool : public base::CSingle<CBitStreamPool> {
    public:    
        CBitStreamPool();
        ~CBitStreamPool();

        // push __init_pool_size net msg to free queue.
        void ExpendFree();
        // reduce half of free queue every time 
        void ReduceFree();

        CBitStream* GetBitStream();

        void FreeBitStream(CBitStream* bit_stream);
        
    private:
        base::CTSQueue<CBitStream*>    _free_queue;
    };
}
#endif
