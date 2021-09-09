#define _USE_MATH_DEFINES
#include<bits/stdc++.h>
#define ld long double
using namespace std;

vector<vector<ld>> readFile(string file_name);
vector<vector<ld>> getSteadyFrames(vector<vector<ld>> file);
vector<vector<ld>> readCodebook(string file_name);
void normalizeValues(vector<vector<ld>>& file);
void applyHammingWindow(vector<ld>& frame);
vector<ld> getAutoCorrelationTerms(vector<ld> frame);
vector<ld> applyDurbins(vector<ld> r);
vector<ld> performLPCAnalysis(vector<ld> frame);
vector<ld> performLinearTransformation(vector<ld> a);
void applyRaisedSineWindow(vector<ld>& C);
vector<vector<ld>> getCepstralCoefficients(vector<vector<ld>> frames);
int getMinTokhuraDistance(vector<vector<ld>> codebook, vector<ld> CC);
vector<int> getObservationSequence(string file_name, vector<vector<ld>> codebook);
ld forwardAlgorithm(vector<ld> pi, vector<vector<ld>> A, vector<vector<ld>> B, vector<int> O);
vector<struct Model> readModels();
char predictDigit(string file_name, vector<vector<ld>> codebook, vector<struct Model> digitModels);
void testModel();
char recognizeWord();
void printModel(struct Model model);
struct Model readModel(string file_name);
