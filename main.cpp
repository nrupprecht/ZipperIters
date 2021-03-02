#include "Iter.h"

#include <vector>
#include <array>
#include <numeric>

// A little demonstration of how to sort with Iter.
int main() {
    std::vector<double> x(10);
    std::vector<int> y(10);
    std::vector<std::pair<float, float>> z(10);

    std::for_each(x.begin(), x.end(), [](auto& v) { v = drand48(); });
    std::iota(y.rbegin(), y.rend(), 0);
    std::for_each(z.begin(), z.end(), [](auto& v) {
        v = std::pair<float, float>(drand48(), drand48());
    });

    std::sort(zip::Begin(z, x, y), zip::End(z, x, y));

    return 0;
}
