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
#include <functional>

#include <multiplex.h>
#include <nsga2.h>
#include <vector_cmp.h>

#define DPRINTC(C) printf(#C " = %c\n", (C))
#define DPRINTS(S) printf(#S " = %s\n", (S))
#define DPRINTD(D) printf(#D " = %d\n", (D))
#define DPRINTLLD(LLD) printf(#LLD " = %lld\n", (LLD))
#define DPRINTLF(LF) printf(#LF " = %.5lf\n", (LF))

using namespace std;
typedef unsigned int uint;
typedef long long lld;
typedef unsigned long long llu;

HMMChainMultiplex::HMMChainMultiplex(int obs, int L) : obs(obs), L(L)
{
    this -> layers.resize(L);
    for (int i=0;i<L;i++)
    {
        this -> layers[i] = new SimpleChainGMHMM(obs);
    }
    
    this -> omega = new double*[L];
    for (int i=0;i<L;i++)
    {
        this -> omega[i] = new double[L];
        for (int j=0;j<L;j++)
        {
            this -> omega[i][j] = (i == j) ? 1.0 : 0.0;
        }
    }
}

HMMChainMultiplex::~HMMChainMultiplex()
{
    for (int i=0;i<L;i++) delete layers[i];
    for (int i=0;i<L;i++) delete[] omega[i];
    delete[] omega;
}

void HMMChainMultiplex::set_omega(double **omega)
{
    for (int i=0;i<L;i++)
    {
        double sum = 0.0;
        for (int j=0;j<L;j++)
        {
            this -> omega[i][j] = omega[i][j];
            sum += omega[i][j];
        }
        for (int j=0;j<L;j++)
        {
            this -> omega[i][j] /= sum;
        }
    }
}

void HMMChainMultiplex::train(vector<vector<vector<double> > > &train_set)
{
    // Train all the layers individually (as before)
    for (int l=0;l<L;l++)
    {
        vector<vector<double> > curr_set(train_set.size(), vector<double>(obs));
        for (uint i=0;i<train_set.size();i++)
        {
            for (int j=0;j<obs;j++)
            {
                curr_set[i][j] = train_set[i][j][l];
            }
        }
        layers[l] -> train(curr_set);
    }
    
    // Define the lambdas that calculate likelihoods for a given omega
    objectives.resize(train_set.size());
    for (uint t=0;t<train_set.size();t++)
    {
        objectives[t] = [this, t, &train_set] (vector<double> X) -> double
        {
            double **temp_omega = new double*[L];
            for (int i=0;i<L;i++)
            {
                temp_omega[i] = new double[L];
                for (int j=0;j<L;j++)
                {
                    temp_omega[i][j] = X[i*L + j];
                }
            }
            
            set_omega(temp_omega);
            
            for (int i=0;i<L;i++) delete[] temp_omega[i];
            delete[] temp_omega;
            
            return -log_likelihood(train_set[t]);
        };
    }
    
    // Prepare the input parameters for NSGA-II
    nsga2_params params;
    params.pop_size = 100;
    params.ft_size = L * L;
    params.obj_size = train_set.size();
    params.generations = 250;
    params.p_crossover = 0.9;
    params.p_mutation = 1.0 / params.ft_size;
    params.di_crossover = 20.0;
    params.di_mutation = 20.0;
    params.var_lims.resize(params.ft_size);
    for (int i=0;i<params.ft_size;i++)
    {
        params.var_lims[i] = make_pair(1e-6, 1.0);
    }
    
    // Run the algorithm
    NSGAII nsga2;
    vector<chromosome> candidates = nsga2.optimise(params, objectives);
    
    // Evaluate the best choice of omega
    int best = -1;
    double min_sum = -1.0;
    for (uint i=0;i<candidates.size();i++)
    {
        sort(candidates[i].values.begin(), candidates[i].values.end());
        double curr_sum = 0.0;
        for (uint j=0;j<train_set.size();j++)
        {
            curr_sum += (j + 1) * candidates[i].values[j];
        }
        if (best == -1 || curr_sum < min_sum)
        {
            best = i;
            min_sum = curr_sum;
        }
    }
    
    // Adjust the parameters accordingly
    double **fin_omega = new double*[L];
    for (int i=0;i<L;i++)
    {
        fin_omega[i] = new double[L];
        for (int j=0;j<L;j++)
        {
            fin_omega[i][j] = candidates[best].features[i*L + j];
        }
    }
    
    set_omega(fin_omega);
    
    for (int i=0;i<L;i++) delete[] fin_omega[i];
    delete[] fin_omega;
    
    printf("TRANSITION MATRIX:\n");
    for (int i=0;i<L;i++)
    {
        for (int j=0;j<L;j++)
        {
            printf("%lf ", omega[i][j]);
        }
        printf("\n");
    }
}

double HMMChainMultiplex::log_likelihood(vector<vector<double> > &test_data)
{
    vector<pair<vector<double>, int> > sorted_data;
    sorted_data.resize(obs);
    for (int i=0;i<obs;i++) sorted_data[i] = make_pair(test_data[i], i);
    sort(sorted_data.begin(), sorted_data.end(), compare_euclidean);
    
    double ret = 0.0;
    
    double **A = new double*[obs];
    for (int i=0;i<obs;i++) A[i] = new double[L];
    
    double *pi = new double[L];
    double pi_sum = 0.0;
    for (int i=0;i<L;i++) pi_sum += omega[i][i];
    for (int i=0;i<L;i++) pi[i] = omega[i][i] / pi_sum;
    
    int first_gene = sorted_data[0].second;
    double init_sum = 0.0;
    for (int i=0;i<L;i++)
    {
        double curr_type_val = sorted_data[0].first[i];
        double curr_g = layers[i] -> get_G(0, first_gene);
        double curr_prob = layers[i] -> get_probability(first_gene, curr_type_val);
        A[0][i] = pi[i] * curr_g * curr_prob;
        init_sum += A[0][i];
    }
    
    for (int i=0;i<L;i++)
    {
        A[0][i] /= init_sum;
    }
    ret += log(init_sum);
    
    for (int t=1;t<obs;t++)
    {
        double fullsum = 0.0;
        int curr_gene = sorted_data[t].second;
        
        for (int j=0;j<L;j++)
        {
            double sum = 0.0;
            double curr_type_val = sorted_data[t].first[j];
            double curr_g = layers[j] -> get_G(t, curr_gene);
            double curr_prob = layers[j] -> get_probability(curr_gene, curr_type_val);
            
            for (int i=0;i<L;i++)
            {
                sum += A[t-1][i] * omega[i][j] * curr_g * curr_prob;
            }
            
            A[t][j] = sum;
            fullsum += sum;
        }
        
        for (int j=0;j<L;j++)
        {
            A[t][j] /= fullsum;
        }
        
        ret += log(fullsum);
    }
    
    for (int i=0;i<obs;i++) delete[] A[i];
    delete[] A;
    delete[] pi;
    
    return ret;
}

void HMMChainMultiplex::dump_muxviz_data(char *nodes_filename, char *base_layers_filename)
{
    FILE *f = fopen(nodes_filename, "w");
    
    fprintf(f, "nodeID nodeX nodeY\n");
    
    for (int i=1;i<=obs;i++)
    {
        fprintf(f, "%d %d %d\n", i, i, 0);
    }
    
    fclose(f);
    
    printf("Node data successfully written to %s.\n", nodes_filename);
    
    for (int i=0;i<L;i++)
    {
        char curr_lyr_filename[150];
        sprintf(curr_lyr_filename, "%s_%d", base_layers_filename, i+1);
        FILE *g = fopen(curr_lyr_filename, "w");
        for (int j=1;j<obs;j++)
        {
            fprintf(g, "%d %d %lf\n", j, j+1, omega[i][i]);
        }
        fclose(g);
        printf("Layer %d data successfully written to %s.\n", i+1, curr_lyr_filename);
    }
    
    printf("Done.\n");
}
