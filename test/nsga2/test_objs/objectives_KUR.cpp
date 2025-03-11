// code by svg
#include <math.h>
#include <functional>
#include <vector>

using namespace std;

double KUR_F1(vector<double> X)
{
    double ret = 0.0;
    for (int i=0;i<2;i++)
    {
        ret += (-10.0 * exp(-0.2 * sqrt(X[i] * X[i] + X[i+1] * X[i+1])));
    }
    return ret;
}

double KUR_F2(vector<double> X)
{
    double ret = 0.0;
    for (int i=0;i<3;i++)
    {
        ret += (pow(fabs(X[i]), 0.8) + 5.0 * sin(X[i] * X[i] * X[i]));
    }
    return ret;
}

vector<function<double(vector<double>)> > get_objectives()
{
    vector<function<double(vector<double>)> > ret;
    ret.push_back(KUR_F1);
    ret.push_back(KUR_F2);
    return ret;
}
