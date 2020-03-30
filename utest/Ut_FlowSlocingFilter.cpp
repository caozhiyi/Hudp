
#include <algorithm>
#include "IFilter.h"
#include "gtest/gtest.h"
#include "FlowSlicingFilter.h"

using namespace hudp;

// the flow slicing filter
class CTestSlicingFlowFilter : public CFilter {
public:
    CTestSlicingFlowFilter() {}
    virtual ~CTestSlicingFlowFilter() {}
    // when send msg filter process
    bool FilterProcess(const HudpHandle& handle, uint16_t flag, std::string& body) {
        _msg_vec.push_back(body);
        return true;
    }
    // when recv msg filter process
    bool RelieveFilterProcess(const HudpHandle& handle, uint16_t flag, std::string& body) {
        _big_msg = body;
        return true;
    }

    // get msg vec
    std::vector<std::string>& GetMsgVec() { return _msg_vec; }
    // get big msg
    std::string GetBigMsg() { return _big_msg; }
private:
    std::vector<std::string> _msg_vec;
    std::string _big_msg;
};

std::string big_msg;
CFlowSlicingFilter slicing_filter; 
std::shared_ptr<CTestSlicingFlowFilter> test_slicingflow_filter(new CTestSlicingFlowFilter());

TEST(FlowSlicingFilter, case1) {
    // build big msg
    for (int i = 0; i < 1000; i++) {
        for (int j = 0; j < 1000; j++) {
            big_msg.append(std::to_string(j));
        }
    }
    slicing_filter.SetNextFilter(test_slicingflow_filter);
    slicing_filter.SetPrevFilter(test_slicingflow_filter);
    EXPECT_TRUE(slicing_filter.FilterProcess("", 0, big_msg));
}

TEST(FlowSlicingFilter, case2) {
    auto& msg_vec = test_slicingflow_filter->GetMsgVec();
    std::random_shuffle(msg_vec.begin(), msg_vec.end());
    for (size_t i = 0; i < msg_vec.size(); i++) {
        EXPECT_TRUE(slicing_filter.RelieveFilterProcess("", 0, msg_vec[i]));
    }
}

TEST(FlowSlicingFilter, case3) {
    EXPECT_TRUE(big_msg == test_slicingflow_filter->GetBigMsg());
}
