// code by svg
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <iostream>
#include <vector>
#include <list>
#include <string>
#include <algorithm>
#include <deque>
#include <queue>
#include <stack>
#include <set>
#include <map>
#include <complex>

#include <classifier.h>
#include <hmm.h>

#define DPRINTC(C) printf(#C " = %c\n", (C))
#define DPRINTS(S) printf(#S " = %s\n", (S))
#define DPRINTD(D) printf(#D " = %d\n", (D))
#define DPRINTLLD(LLD) printf(#LLD " = %lld\n", (LLD))
#define DPRINTLF(LF) printf(#LF " = %.5lf\n", (LF))

using namespace std;
typedef unsigned int uint;
typedef long long lld;
typedef unsigned long long llu;

GMHMMClassifier::GMHMMClassifier(int gene_count, int param_id) : gene_count(gene_count), param_id(param_id)
{
    // bounds on state space inspired by image processing papers
    patient_model = new GMHMM(min(gene_count, 4), gene_count);
    normal_model = new GMHMM(min(gene_count, 4), gene_count);
    thresholds.clear();
}

GMHMMClassifier::~GMHMMClassifier()
{
    delete patient_model;
    delete normal_model;
}

void GMHMMClassifier::train(vector<pair<vector<vector<double> >, bool> > &training_set)
{
    vector<vector<double> > train_patient, train_normal;
    int patient_cnt = 0, normal_cnt = 0;
    
    for (uint i=0;i<training_set.size();i++)
    {
        if (training_set[i].second)
        {
            train_patient.push_back(vector<double>(gene_count));
            for (int j=0;j<gene_count;j++)
            {
                train_patient[patient_cnt][j] = training_set[i].first[j][param_id];
            }
            patient_cnt++;
        }
        else
        {
            train_normal.push_back(vector<double>(gene_count));
            for (int j=0;j<gene_count;j++)
            {
                train_normal[normal_cnt][j] = training_set[i].first[j][param_id];
            }
            normal_cnt++;
        }
    }
    
    patient_model -> train(train_patient);
    normal_model -> train(train_normal);
    
    thresholds.clear();
}

bool GMHMMClassifier::classify(vector<vector<double> > &test_data)
{
    vector<double> extracted_subvec;
    extracted_subvec.resize(test_data.size());
    for (uint i=0;i<test_data.size();i++) extracted_subvec[i] = test_data[i][param_id];
    
    double lhood1 = patient_model -> log_likelihood(extracted_subvec);
    double lhood0 = normal_model -> log_likelihood(extracted_subvec);
    
    thresholds.push_back(lhood1 - lhood0);
    
    return (lhood1 > lhood0);
}

vector<double> GMHMMClassifier::get_thresholds()
{
    return thresholds;
}
