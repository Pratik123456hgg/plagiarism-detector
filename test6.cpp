#include <iostream>

using namespace std;

int main() {
  int total = 0;
  for (int j = 0; j < 10; ++j) {
    total += j;
  }
  cout << "Total: " << total << endl;
  return 0;
}
