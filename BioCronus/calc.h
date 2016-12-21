#include <iostream>
#include <string>
#include<fstream>
#include <iomanip>
#include <map>
#include <random>
#include <cmath>
#include "gcapi.h"

#define MaxJitter		15//The amplitude for jitter
#define L_RR			8 // low threshold for breathing rate
#define H_RR			20// high threshold for breathing rate

#define HRV 1
#define HR 2
#define RR 3
extern bool Break;
extern float intensity;
extern double JitterDegree;
extern double Jitter;
extern bool ControlActive;

using namespace std;
double RESP2STRESS(double resp);//Calculate stress index from respiration rate
double HRV2STRESS(double hrv_relax, double hrv_stress, int hrv);//Calculate stress index from hrv
void Jittered_Mod(int8_t output[], float STRESS);//Add jitter to the report data in Cronus
void Speed_Only(int8_t output[], float STRESS);
void Speed_Decrease(int8_t output[], float STRESS);
void Speed_Increase(int8_t output[], float STRESS);
void Break_Mod(int8_t output[], float STRESS);
void Sensitive_Mod(int8_t output[], float STRESS, int sensitive);