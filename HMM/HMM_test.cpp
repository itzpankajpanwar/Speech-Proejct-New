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
int n_words = 16;
int T = 85;

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

ld forwardAlgorithm(vector<ld> pi, vector<vector<ld>> A, vector<vector<ld>> B, vector<int> O){
    vector<vector<ld>> alpha(T,vector<ld>(n_states));
    for(int i=0; i<n_states; ++i) alpha[0][i] = pi[i] * B[i][O[0]];
    
    for(int t=0; t<T-1; ++t){
        for(int j=0; j<n_states; ++j){
            ld sum = 0.0;
            for(int i=0; i<n_states; ++i) sum += alpha[t][i]*A[i][j];
            alpha[t+1][j] = sum * B[j][O[t+1]];
        }
    }

    ld p = 0.0;
    for(int i=0; i<n_states; ++i) p += alpha[T-1][i];
    return p;
}

vector<struct Model> readModels(){
    // vector<char> digits = {'1','2','3','4','5','6','7','8'};
    vector<char> digits = {'1','2','3','4','5','6','7','8', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
    vector<struct Model> digitModels;

    for(char digit:digits){
        string file_name = base_path + "\\Models\\Model_" + digit + ".txt";
        ifstream ifs(file_name);
        struct Model model;

        for(int i=0; i<n_states; ++i){
            ld p;
            ifs >> p;
            model.pi.push_back(p);
        }

        for(int i=0; i<n_states; ++i){
            vector<ld> temp(n_states);
            for(int j=0; j<n_states; ++j){
                ld a;
                ifs >> a;
                temp[j] = a;
            }
            model.A.push_back(temp);
        }

        for(int i=0; i<n_states; ++i){
            vector<ld> temp(codebook_size);
            for(int j=0; j<codebook_size; ++j){
                ld a;
                ifs >> a;
                temp[j] = a;
            }
            model.B.push_back(temp);
        }

        digitModels.push_back(model);
    }

    return digitModels;
}

int predictDigit(string file_name, vector<vector<ld>> codebook, vector<struct Model> digitModels){
    vector<char> digits = {'1','2','3','4','5','6','7','8', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
    vector<int> O = getObservationSequence(file_name, codebook);
    ld maxProb = 0.0;
    int predicted = -1;
    for(int i=0; i<n_words; ++i){
        ld curProb = forwardAlgorithm(digitModels[i].pi, digitModels[i].A, digitModels[i].B, O);
        if(curProb > maxProb){
            maxProb = curProb;
            predicted = i+1;
        }
        cout << "Probability of " << digits[i] << " is: " << curProb << endl;
    }

    return predicted;
}

void testModel(){
    cout << "Testing the model..." << endl;
    string codebook_file_name = base_path + "Codebook.txt";
    vector<vector<ld>> codebook = readCodebook(codebook_file_name);
    vector<struct Model> digitModels = readModels();

    int correct = 0;
    vector<char> digits = {'1','2','3','4','5','6','7','8'};
    string file_base_name = base_path + "Sound\\204101041_";
    for(auto digit:digits){
        for(int d=21; d<=30; ++d){ 
            string file_name = file_base_name + digit + "_" + to_string(d) + ".txt";
            cout << "Testing file: " << file_name << endl;
            int predicted = predictDigit(file_name, codebook, digitModels);
            if(digit == '0'+predicted) correct++;
            // cout << predicted << "\t";
        }
        // cout << endl;
    }

    cout << "Testing done!" << endl;
    cout << "Accuracy of the model: " << correct << "/100" << endl;;
}

void testModelRealtime(){
    string codebook_file_name = base_path + "Codebook.txt";
    vector<vector<ld>> codebook = readCodebook(codebook_file_name);
    vector<struct Model> digitModels = readModels();
    string file_name = base_path + "Sample.txt";
    system("Recording_Module.exe 3 Sample.wav Sample.txt");
    int predicted = predictDigit(file_name, codebook, digitModels);
    cout << "The predicted digit is: " << predicted << endl;
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

struct Model readModel(string file_name){
    ifstream ifs(file_name);
    struct Model model;
    for(int i=0; i<n_states; ++i){
        ld p;
        ifs >> p;
        model.pi.push_back(p);
    }

    for(int i=0; i<n_states; ++i){
        vector<ld> temp(n_states);
        for(int j=0; j<n_states; ++j){
            ld a;
            ifs >> a;
            temp[j] = a;
        }
        model.A.push_back(temp);
    }

    for(int i=0; i<n_states; ++i){
        vector<ld> temp(codebook_size);
        for(int j=0; j<codebook_size; ++j){
            ld a;
            ifs >> a;
            temp[j] = a;
        }
        model.B.push_back(temp);
    }

    return model;
}

int main(){
    // testModel();
    testModelRealtime();    
    return 0;
}
