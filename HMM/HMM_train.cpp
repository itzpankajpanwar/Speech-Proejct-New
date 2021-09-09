#define _USE_MATH_DEFINES
#include<bits/stdc++.h>
#include<fstream>
#include<cmath>
#define ld long double
using namespace std;

struct Model{
    vector<ld> pi;
    vector<vector<ld>> A;
    vector<vector<ld>> B;
};

string base_path = ".\\";
int FRAME_SIZE = 320;
int inter_frame_gap = 80;
int steThreshold = 5000;
int n_CC = 12;
int codebook_size = 32;
int n_states = 5;
int T = 85;
int n_epochs = 20;
int n_words = 16;
int n_train = 50;
vector<int> stateSequence(T);

vector<vector<ld>> readFile(string file_name){
    ifstream ifs(base_path + file_name);
    if(!ifs.is_open()){ 
        cout << "Could not open the file " << file_name << "!\n";
        exit(0);
    }

    vector<ld> samples;
    int n_samples = 0;
    while(!ifs.eof()){
        ld sample;
        ifs >> sample;
        samples.push_back(sample);
        n_samples++;
    }
    ifs.close();

    int start = 0;
    vector<vector<ld>> file;
    while(start + FRAME_SIZE < n_samples){
        vector<ld> frame;
        for(int i=start; i<start+FRAME_SIZE; ++i) frame.push_back(samples[i]);
        file.push_back(frame);
        start += inter_frame_gap;
    }

    return file;
}

vector<vector<ld>> getSteadyFrames(vector<vector<ld>> file){
    int file_size = file.size();
    vector<vector<ld>> stableFrames;
    ld maxSTE = 0;
    int maxSTE_idx = -1;
    for(int i=0; i<file_size; ++i){
        ld STE = 0.0;
        for(auto sample:file[i]) STE = sample * sample;
        if(STE > maxSTE){
            maxSTE = STE;
            maxSTE_idx = i;
        }
    }

    int startFrame = maxSTE_idx - T/2;
    int endFrame = maxSTE_idx + T/2 + 1;
    if(startFrame < 0){
        startFrame = 0;
        endFrame = T;
    }

    else if(endFrame >= file_size){
        startFrame = file_size - T;
        endFrame = file_size;
    }

    for(int i=startFrame; i<endFrame; ++i) stableFrames.push_back(file[i]);
    return stableFrames;
}

vector<vector<ld>> readCodebook(string file_name){
    vector<vector<ld>> codebook;
    ifstream ifs(base_path + file_name);
    if(!ifs.is_open()){ 
        cout << "Could not open the file " << file_name << "!\n";
        exit(0);
    }

    while(!ifs.eof()){
        vector<ld> codeVector;
        for(int i=0; i<n_CC; ++i){
            ld CC;
            ifs >> CC;
            codeVector.push_back(CC);
        }
        codebook.push_back(codeVector);
    }
    ifs.close();
    
    return codebook;
}

void normalizeValues(vector<vector<ld>>& file){
    ld maxVal = 1e-300;
    for(auto frame:file)
        for(auto sample:frame)
            maxVal = max(maxVal, abs(sample));
    
    ld scalingFactor = 10000.0/maxVal;

    for(auto frame:file)
        for(auto sample:frame)
            sample *= scalingFactor;
}

void applyHammingWindow(vector<ld>& frame){
	for(int m=0; m<FRAME_SIZE; ++m){
		ld wm = 0.54 - 0.46 * cos(2*M_PI*m /(FRAME_SIZE-1));
		frame[m] *= wm;
	}
}

vector<ld> getAutoCorrelationTerms(vector<ld> frame){
	vector<ld> r;
	for(int k=0; k<=n_CC; ++k){
		ld rk = 0;
		for(int m=0; m < FRAME_SIZE-k; ++m) rk += frame[m] * frame[m+k];
		r.push_back(rk);
		if(k==0 && rk <= 0) break;
	}
	return r;
}

vector<ld> applyDurbins(vector<ld> r){
	vector<ld> alpha;
	ld E = r[0];

	for(int i=1; i<=n_CC; ++i){
		ld sum = 0;
		for(int j=1;j<i; ++j) sum += alpha[j] * r[i-j];
		ld k = (r[i] - sum) / E;
		
		vector<ld> alpha_new(i+1);
		alpha_new[i] = k;
		for(int j=1; j<i; ++j) alpha_new[j] = alpha[j] - k * alpha[i-j];
		alpha = alpha_new;
		E = (1 - k*k) * E;
	}

	return alpha;
}

vector<ld> performLPCAnalysis(vector<ld> frame){
	applyHammingWindow(frame);
	vector<ld> r,a;
	r = getAutoCorrelationTerms(frame);
	if(r[0] <= 0) return a;
	a = applyDurbins(r);
	return a;
}

vector<ld> performLinearTransformation(vector<ld> a){
	// for(int i=0; i<=n_CC; ++i) a[i] = -a[i];
	vector<ld> C(n_CC+1);
	for(int m=1; m<=n_CC; ++m){
		ld sum = 0;
		for(int k=1; k<m; ++k) sum += ((ld)k/(ld)m) * C[k] * a[m-k];
		C[m] = a[m] + sum;
	}

	return C;
}

void applyRaisedSineWindow(vector<ld>& C){
	for(int m=1; m<=n_CC; ++m){
		ld wm = 1 + (n_CC/2) * sin(M_PI*m/n_CC);
		C[m] *= wm;
	}
}

vector<vector<ld>> getCepstralCoefficients(vector<vector<ld>> frames){
    vector<vector<ld>> CC;
    for(auto frame:frames){
        vector<ld> lpc = performLPCAnalysis(frame);
        vector<ld> cc = performLinearTransformation(lpc);
        applyRaisedSineWindow(cc);
        CC.push_back(cc);
    }
    return CC;
}

int getMinTokhuraDistance(vector<vector<ld>> codebook, vector<ld> CC){
    int min_idx = -1;
    int cur_idx = 0;
    ld minDistance = 1e300;
    ld w[] = {1.0, 3.0, 7.0, 13.0, 19.0, 22.0, 25.0, 33.0, 42.0, 50.0, 56.0, 61.0};
    for(auto codeVector:codebook){
        ld TokhuraDistance = 0.0;
        for(int i=0; i<n_CC; ++i)
            TokhuraDistance += w[i] * (CC[i+1] - codeVector[i]) * (CC[i+1] - codeVector[i]); 
        if(minDistance > TokhuraDistance){
            min_idx = cur_idx;
            minDistance = TokhuraDistance;
        }
        cur_idx++;
    }

    return min_idx;
}

vector<int> getObservationSequence(string file_name, vector<vector<ld>> codebook){
    vector<vector<ld>> file = readFile(file_name);
    vector<vector<ld>> steadyFrames = getSteadyFrames(file);
    normalizeValues(steadyFrames);
    vector<vector<ld>> CC = getCepstralCoefficients(steadyFrames);
    vector<int> observationSequence;
    for(auto cc:CC) observationSequence.push_back(getMinTokhuraDistance(codebook,cc));
    return observationSequence;
}

void initialize_Bayks_Model(vector<ld>& pi, vector<vector<ld>>& A, vector<vector<ld>>& B){
    pi[0] = 1.0;

    for(int i=0; i<n_states-1; ++i){
        A[i][i] = 0.8;
        A[i][i+1] = 0.2;
    }
    A[n_states-1][n_states-1] = 1.0;
}

vector<vector<ld>> forwardAlgorithm(vector<ld> pi, vector<vector<ld>> A, vector<vector<ld>> B, vector<int> O){
    vector<vector<ld>> alpha(T,vector<ld>(n_states));
    for(int i=0; i<n_states; ++i) alpha[0][i] = pi[i] * B[i][O[0]];
    
    for(int t=0; t<T-1; ++t){
        for(int j=0; j<n_states; ++j){
            ld sum = 0.0;
            for(int i=0; i<n_states; ++i) sum += alpha[t][i]*A[i][j];
            alpha[t+1][j] = sum * B[j][O[t+1]];
        }
    }

    return alpha;
}

vector<vector<ld>> backwardAlgorithm(vector<vector<ld>> A, vector<vector<ld>> B, vector<int> O){
    vector<vector<ld>> beta(T,vector<ld>(n_states));
    for(int i=0; i<n_states; ++i) beta[T-1][i] = 1.0;

    for(int t=T-2; t>=0; --t){
        for(int i=0; i<n_states; ++i){
            ld sum = 0.0;
            for(int j=0; j<n_states; ++j) sum += A[i][j] * B[j][O[t+1]] * beta[t+1][j];
            beta[t][i] = sum;
        }
    }

    return beta;
}

void ViterbiAlgorithm(vector<ld> pi, vector<vector<ld>> A, vector<vector<ld>> B, vector<int> O){
    vector<vector<int>> si;
    si.push_back(vector<int>(n_states,-1));
    vector<ld> delta(n_states);
    for(int i=0; i<n_states; ++i) delta[i] = pi[i] * B[i][O[0]];
    for(int t=1; t<T; ++t){
        vector<ld> delta_new(n_states);
        vector<int> si_t(n_states);
        for(int j=0; j<n_states; ++j){
            ld maxVal = -1e300;
            int max_idx = -1;
            for(int i=0; i<n_states; ++i){
                if(maxVal < delta[i]*A[i][j]){
                    maxVal = delta[i]*A[i][j];
                    max_idx = i;
                }
            } 
            delta_new[j] = maxVal * B[j][O[t]];
            si_t[j] = max_idx; 
        }
        delta = delta_new; 
        si.push_back(si_t);
    }

    ld p = -1e300;
    int q = -1;
    for(int i=0; i<n_states; ++i){
        if(p < delta[i]){
            p = delta[i];
            q = i;
        }
    }

    // vector<int> stateSequence(T);
    stateSequence[T-1] = q;
    for(int t=T-2; t>=0; --t)
        stateSequence[t] = si[t+1][stateSequence[t+1]];
    
    // cout << "Viterbi p*: " << p << endl;
}

vector<vector<vector<ld>>> calculate_zeta(vector<vector<ld>> A, vector<vector<ld>> B, vector<int> O, vector<vector<ld>> alpha, vector<vector<ld>> beta){
    vector<vector<vector<ld>>> zeta(T-1, vector<vector<ld>>(n_states, vector<ld>(n_states)));
    for(int t=0; t<T-1; ++t){
        ld sum = 0.0;
        for(int i=0; i<n_states; ++i)
            for(int j=0; j<n_states; ++j)
                sum += alpha[t][i] * A[i][j] * B[j][O[t+1]] * beta[t+1][j];
            
        for(int i=0; i<n_states; ++i)
            for(int j=0; j<n_states; ++j)
                zeta[t][i][j] = alpha[t][i] * A[i][j] * B[j][O[t+1]] * beta[t+1][j] / sum; 
    }

    return zeta;
}

vector<vector<ld>> calculate_gamma(vector<vector<ld>> alpha, vector<vector<ld>> beta){
    vector<vector<ld>> gamma(T, vector<ld>(n_states));
    for(int t=0; t<T; ++t){
        ld sum = 0.0;
        for(int i=0; i<n_states; ++i) sum += alpha[t][i] * beta[t][i];
        for(int i=0; i<n_states; ++i) gamma[t][i] = alpha[t][i] * beta[t][i] / sum;
    }

    return gamma;
}

void parameterReestimation(vector<ld>& pi, vector<vector<ld>>& A, vector<vector<ld>>& B, vector<int> O, vector<vector<ld>> alpha, vector<vector<ld>> beta){
    vector<vector<vector<ld>>> zeta = calculate_zeta(A, B, O , alpha, beta);
    vector<vector<ld>> gamma = calculate_gamma(alpha, beta);

    for(int i=0; i<n_states; ++i) pi[i] = gamma[0][i];

    for(int i=0; i<n_states; ++i){
        for(int j=0; j<n_states; ++j){
            ld numerator = 0.0;
            ld denominator = 0.0;
            for(int t=0; t<T-2; ++t){
                numerator += zeta[t][i][j];
                denominator += gamma[t][i];
            }
            A[i][j] = numerator / denominator;
        }
    }

    for(int i=0; i<n_states; ++i){
        for(int k=0; k<codebook_size; ++k){
            ld numerator = 0.0;
            ld denominator = 0.0;
            for(int t=0; t<T; ++t) if(O[t] == k) numerator += gamma[t][i];
            for(int t=0; t<T-1; ++t) denominator += gamma[t][i];
            B[i][k] = max((ld)1e-30, numerator / denominator); 
        }
    }
}

struct Model Bayks_Model(string file_name, vector<vector<ld>> codebook){
    vector<int> O = getObservationSequence(file_name, codebook);
    vector<ld> pi(n_states,0.0);
    vector<vector<ld>> A(n_states, vector<ld>(n_states, 0.0));
    vector<vector<ld>> B(n_states, vector<ld>(codebook_size, (ld)1/(ld)codebook_size));
    
    initialize_Bayks_Model(pi, A, B);
    for(int epoch=0; epoch<n_epochs; ++epoch){
        vector<vector<ld>> alpha = forwardAlgorithm(pi, A, B, O);
        vector<vector<ld>> beta = backwardAlgorithm(A, B, O);
        ViterbiAlgorithm(pi, A, B, O);
        parameterReestimation(pi, A, B, O, alpha, beta);
    }

    struct Model model;
    model.pi = pi;
    model.A = A;
    model.B = B;
    return model;
}

void writeModel(struct Model model, char digit){
    string file_name = base_path + "\\Models\\Model_" + digit + ".txt";
    ofstream ofs(file_name);
    for(auto p:model.pi) ofs << p << " ";
    ofs << endl;

    for(auto x:model.A){
        for(auto y:x) ofs << y << " ";
        ofs << endl;
    }

    for(auto x:model.B){
        for(auto y:x) ofs << y << " ";
        ofs << endl;
    } 

    ofs.close();
}

void trainModel(){
    cout << "Training the model..." << endl;
    string codebook_file_name = base_path + "Codebook.txt";
    vector<vector<ld>> codebook = readCodebook(codebook_file_name);

    // vector<char> digits = {'1','2','3','4','5','6','7','8'};
    vector<char> digits = {'1','2','3','4','5','6','7','8',
                            'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};

    string file_base_name = base_path + "Sound\\204101041_";
    for(auto digit : digits){
        cout << "Training model for digit " << digit << endl;
        struct Model model;
        model.pi = vector<ld>(n_states,0.0);
        model.A = vector<vector<ld>>(n_states,vector<ld>(n_states,0.0));
        model.B = vector<vector<ld>>(n_states,vector<ld>(codebook_size,0.0));

        for(int d=1; d<=n_train; ++d){ 
            string file_name = file_base_name + digit + "_" + to_string(d) + ".txt";
            struct Model m = Bayks_Model(file_name, codebook);

            for(int i=0; i<n_states; ++i) model.pi[i] += m.pi[i]/n_train;
            for(int i=0; i<n_states; ++i)
                for(int j=0; j<n_states; ++j)
                    model.A[i][j] += m.A[i][j]/n_train;
            for(int i=0; i<n_states; ++i)
                for(int j=0; j<codebook_size; ++j)
                    model.B[i][j] += m.B[i][j]/n_train;
        }
        
        writeModel(model, digit);
    }

    cout << "Model training done!" << endl;
}

struct Model trainSingleModel(){
    string codebook_file_name = base_path + "Codebook.txt";
    vector<vector<ld>> codebook = readCodebook(codebook_file_name);
    string file_name = base_path + "temp.txt";
    struct Model model = Bayks_Model(file_name, codebook);
    return model;
}

void printModel(struct Model model){
    cout << "pi:" << endl;
    for(auto p:model.pi) cout << p << " ";
    cout << "\n\n";
    for(auto x:model.A){
        for(auto y:x) cout << y << " ";
        cout << endl;
    }
    cout << endl;
    for(auto x:model.B){
        for(auto y:x) cout << y << " ";
        cout << endl;
    }
    cout << "----------------------------------------------------------------------------------------------------------\n\n";
}

int main(){
    trainModel();
    // trainSingleModel();
    return 0;
}
