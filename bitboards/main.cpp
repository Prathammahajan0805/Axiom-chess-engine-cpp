#include "uci.h"
#include<windows.h>
using namespace std;
int main(){
    std::setvbuf(stdout, nullptr, _IONBF, 0);//output buffer
    initUCI();
    return 0;
}