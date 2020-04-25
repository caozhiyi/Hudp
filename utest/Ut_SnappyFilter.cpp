
#include <algorithm>
#include "IFilter.h"
#include "gtest/gtest.h"
#include "SnappyFilter.h"

using namespace hudp;

//  test snappy filter
class CTestSnappyFilter : public CFilter {
public:
    CTestSnappyFilter() {}
    virtual ~CTestSnappyFilter() {}
    // when send msg filter process
    bool FilterProcess(const HudpHandle& handle, uint16_t flag, std::string& body, uint32_t upper_id = 0) {
        _msg = body;
        return true;
    }
    // when recv msg filter process
    bool RelieveFilterProcess(const HudpHandle& handle, uint16_t flag, std::string& body) {
        _msg = body;
        return true;
    }
    // get msg
    std::string& GetMsg() { return _msg; }
private:
    std::string _msg;
};

std::string msg;
CSnappyFilter snappy_filter;
std::shared_ptr<CTestSnappyFilter> test_snappy_filter(new CTestSnappyFilter());

TEST(SnappyFilter, case1) {
    // build msg
    for (int i = 0; i < 1000; i++) {
        msg.append(std::to_string(i));
    }
    snappy_filter.SetNextFilter(test_snappy_filter);
    snappy_filter.SetPrevFilter(test_snappy_filter);
    EXPECT_TRUE(snappy_filter.FilterProcess("", 0, msg, 0));
}

TEST(SnappyFilter, case2) {
    auto& msg = test_snappy_filter->GetMsg();
    EXPECT_TRUE(snappy_filter.RelieveFilterProcess("", 0, msg));
    
}

TEST(SnappyFilter, case3) {
    EXPECT_TRUE(msg == test_snappy_filter->GetMsg());
}
