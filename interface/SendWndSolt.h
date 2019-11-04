#ifndef HEADER_INTERFACE_SENDWNDSOLT
#define HEADER_INTERFACE_SENDWNDSOLT

namespace hudp {
    class CMsg;
    class CSendWndSolt
    {
    public:
        CSendWndSolt() {}
        virtual ~CSendWndSolt() {}

        // to send a bag call back
        virtual void ToSend(CMsg* msg) = 0;

        // ack call back
        virtual void AckDone(CMsg* msg) = 0;
    };
}
#endif