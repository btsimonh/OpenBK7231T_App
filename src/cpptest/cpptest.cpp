#include "../logging/logging.h"


class cFred {
public:
    cFred(){
        ADDLOG_INFO(LOG_FEATURE_LFS, "test from cpp class");
    }
    ~cFred(){

    }

    void test(const char *str){
        ADDLOG_INFO(LOG_FEATURE_LFS, "test from cpp class2 %s", str);
    }

};

extern "C" int testcpp(){
    cFred fred;

    fred.test("here");

    return 0;
}