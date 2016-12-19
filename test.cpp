#include <iostream>
using namespace std;
extern int is_power_of_two(int);

int main(int argc, char *argv[]){
    cout << "is_power_of_two("<< 65536 <<"): returned " << is_power_of_two(65536) << "\n";
}
