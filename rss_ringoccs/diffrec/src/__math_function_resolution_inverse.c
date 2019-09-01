#include "__math_functions.h"

double Resolution_Inverse_Float(float x){
    float P1, P2;
    if (x <= 1.0){
        return nanf("0");
    }
    else if (x < INFINITY) {
        P1 = x/(1.0-x);
        P2 = P1*expf(P1);
        return LambertW_Float(P2)-P1;
    }
    else {
        return 0.0;
    }
}

double Resolution_Inverse_Double(double x){
    double P1, P2;
    if (x <= 1.0){
        return nan("0");
    }
    else if (x < INFINITY) {
        P1 = x/(1.0-x);
        P2 = P1*exp(P1);
        return LambertW_Double(P2)-P1;
    }
    else {
        return 0.0;
    }
}

long double Resolution_Inverse_Long_Double(long double x){
    double P1, P2;
    if (x <= 1.0){
        return nanl("0");
    }
    else if (x < INFINITY) {
        P1 = x/(1.0-x);
        P2 = P1*expl(P1);
        return LambertW_Long_Double(P2)-P1;
    }
    else {
        return 0.0;
    }
}