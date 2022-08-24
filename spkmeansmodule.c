#define PY_SSIZE_T_CLEAN
#include "spkmeans.h"
#include <Python.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// Helper functions for get_matrix

static PyObject* make_list_from_matrix(double** matrix, int n, int k){
    PyObject *lst = PyList_New(0);
    if(!lst)
        return NULL;
    for(i = 0; i < n; i++){
        row = PyList_New(0);
        if(!row){
            Py_DECREF(lst);
            return NULL;
        }
        for(j = 0; j < k; j++){
            PyList_Append(row,PyFloat_FromDouble((double)matrix[i][j]));
            if (!num){
                Py_DECREF(lst);
                return NULL;
            }
        }
        PyList_Append(lst, row);
    }
    return lst;      
}

static PyObject* make_list_from_array(double* array, int n){
    PyObject *lst = PyList_New(0);
    if(!lst)
        return NULL;
    for(i = 0; i < n; i++){
        PyObject* num = PyFloat_FromDouble((double)array[i]);
            if (!num){
                Py_DECREF(lst);
                return NULL;
            }
        PyList_Append(lst, num);
    }
    return lst;  
}

//function built for python usage

static PyObject* get_matrix(PyObject *self, PyObject *args){
    PyObject *python_matrix;
    int n;
    int k;
    int vector_length;
    char *func;
    
    if (!PyArg_ParseTuple(args, "Oiiis", &python_matrix, &k, &n, &vector_length, &func))
        error_occurred();
    final_ret = PyList_New(0);
    if (final_ret == NULL)
        error_occurred();
    original_matrix = allocateMem(n, vector_length);
    if (matrix == NULL)
        error_occurred();
    for(i = 0; i < n; i++){
        for (j = 0; j < vector_length ; j++) {
            PyObject* num = PyList_GetItem(python_matrix, (vector_length*i) + j);
            original_matrix[i][j] = PyFloat_AsDouble(num);
        }
    }
    
    //check which goal to choose

    if(strcmp(goal, "wam") == 0){
        matrix = wam_calc(original_matrix, n, vector_length);
    }

    else if (strcmp(goal, "ddg") == 0){
        matrix = ddg_calc(original_matrix, n, vector_length);
    }

    else if (strcmp(goal, "lnorm") == 0){
        matrix = lnorm_calc(original_matrix, n, vector_length);
    }

    else if (strcmp(goal, "jacobi") == 0){
        eigens = jacobi(original_matrix, n, vector_length);
        matrix = jacobi_mat_for_print(eigens,n,vector_length);
        PyList_Append(final_ret, make_list_from_matrix(matrix, n,vector_length));
        eigens_arr = (double *)calloc(n, sizeof(double));
        if (eigens_arr == NULL)
            error_occurred();
        for(int i=0; i < n; i++){
            eigens_arr[i] = eigens.value[i];
        }
        PyList_Append(final_ret, make_list_from_array(eigens_arr,n));
        for(i = 0; i < n ; i++)
            free(eigens[i]);
        free(eigens);
        free(eigens_arr);
    }

    else if (strcmp(goal, "spk") == 0){
        w_matrix = wam_calc(original_matrix, n, vec_length);
        d_matrix = ddg_calc(w_matrix, n, vec_length);
        l_matrix = lnorm_calc(d_matrix, n, vec_length);
        eigens_arr = jacobi(l_matrix, n, vec_length);
        if(k == 0){
            k = eigengap_heuristic(l_matrix, n, vec_length);
        } 
        matrix = build_matrix_t_eigen(eigens_arr, n, k);
        PyList_Append(final_ret, make_list_from_matrix(matrix, n, k));
        for(i = 0; i < n ; i++){
            free(w_matrix[i]);
            free(d_matrix[i]);
            free(l_matrix[i]);
            free(eigens_arr[i]);
        }
        free(w_matrix);
        free(d_matrix);
        free(l_matrix);
        free(eigens_arr);
    }

    if (strcmp(goal, "spk") != 0 && strcmp(goal, "jacobi") != 0)
        PyList_Append(final_ret, make_list_from_matrix(matrix, n, vector_length));

    for(i=0; i<n; i++) {
        free(original_matrix[i]);
        free(matrix[i]);
    }
    free(original_matrix);
    free(matrix);

    return final_ret;
}

/**
 * function called from python.
 * preform the kmeans++ algorithm base on the centroid list python sends.
 * return the centroids of the clusters.
 */
static PyObject* fit(PyObject *self, PyObject *args){
    PyObject *items_of_py;
    PyObject *ret;
    PyObject *pylist;
    PyObject *centroids_from_py, *vectors_from_py;
    PyObject *pyCent;
    PyObject *pyCentArr;
    int n, k, d, i, j;
    int bit;
    int iteration_number;
    int min_index;
    int flag;
    double sum;
    double min;
    double** centroids;
    double** elements;
    double *our_final_centroids;
    double cordinate;
    
    if (!PyArg_ParseTuple(args, "iiiOO", &k, &n, &d, &centroids_from_py, &vectors_from_py))
        error_occurred();

    elements = (double**)calloc(n, sizeof(double*));
    if (!elements)
        error_occurred();

    for (i=0;i<n;i++){
        elements[i]=(double*) calloc(d,sizeof(double));
            if (!elements[i])
                error_occurred();
    }

    for (i = 0; i < n; i++) {
        for (j = 0; j < d; j++) {
            items_of_py = PyList_GetItem(vectors_from_py, i * d + j);
            elements[i][j] = PyFloat_AsDouble(items_of_py);
        }
    }

    // init centroids
    centroids = (double**)calloc(k, sizeof(double*));
    if (!centroids)
        error_occurred();

    for (i=0;i<k;i++){
        centroids[i]=(double*) calloc(d,sizeof(double));
            if (!centroids[i])
                error_occurred();
    }

     for (i = 0; i < k; i++) {
        for (j = 0; j < d; j++) {
            items_of_py = PyList_GetItem(centroids_from_py, i * d + j);
            centroids[i][j] = PyFloat_AsDouble(items_of_py);
        }
    }

    /*now we have first centroids from python in C array */

    our_final_centroids = getFinalCentroids(centroids, elements, k, d, n, 300, 0);
    /* creating final centroids array for python in matrix form */
    pyCentArr = PyList_New(0);

    for (i = 0; i < k; i++){
        pyCent = PyList_New(0);
        for (j = 0; j < k; j++)
            PyList_Append(pyCent, PyFloat_FromDouble(our_final_centroids[i * k + j]));
        PyList_Append(pyCentArr, pyCent);
    }

    for(i=0; i<n; i++)
        free(elements[i]);
    free(elements);
    for(i=0; i<k; i++)
        free(centroids[i]);
    free(centroids);

    return Py_BuildValue("O", pyCentArr);
}

/**** KMEANS ****/
double *getFinalCentroids(double *centroids, double *elements, int k, int d, int n, int max_iter, double epsilone)
{
    int bit=1;
    int i;
    int iteration_number=0;
    int min_index = 0;
    int flag = 0;
    double sum = 0.0;
    double min = 0.0;
    int* elements_location;
    int* items_number_clusters;
    double** old_centroids;

    elements_location = (int*)calloc(n, sizeof(int));
    if (!elements_location)
        error_occurred();

    items_number_clusters = (int*)calloc(k, sizeof(int));
    if (!items_number_clusters)
        error_occurred();

    old_centeroids = (double**)calloc(k, sizeof(double*));
    if (!old_centeroids)
        error_occurred();

    for (i=0;i<k;i++){
        old_centeroids[i]=(double*) calloc(d,sizeof(double));
            if (!old_centeroids[i])
                error_occurred();
    }

    while (bit==1 && max_iter>iteration_number)
    {
        iteration_number++;
        initClusters(elements_location, items_number_clusters, n, k);
        assignCentroids(elements, centroids, items_number_clusters,elements_location,k,d,n);
        saveCentroids(old_centroids, centroids, k, d);
        resetCentroids(centroids, k, d);
        updateCentroids(centroids,elements,items_number_clusters,elements_location,d,n,k);
        bit = Convergence(old_centroids, centroids, k, d, epsilone)
    }

    free(elements_location);
    free(items_number_clusters);
    for(i=0; i<k; i++)
        free(old_centeroids[i]);
    free(old_centroids);

    return centroids;
}

/*HELPING FUNCTIONS IN K MEANS*/

void resetCentroids(double* centroids,int k, int d){
    int i;
    int j;
    for (i=0;i<k;i++)
        for(j=0;j<d;j++)
            centroids[i][j]=0.0;                
}

void initClusters(int* elements_loc, int* items_number_clusters, int n, int k){
    int i;
    for (i=0;i<n;i++)
        elements_loc[i]=0;
    for (i=0;i<k;i++)
        items_number_clusters[i]=0;
}

void saveCentroids(double* old_centroids, double* centroids, int k, int d){
    int i;
    int j;
    for (i=0;i<k;i++){
            for (j=0;j<d;j++)
                old_centeroids[i][j] = centeroids[i][j];
        }
}


void assignCentroids(double *ele,double *cntrds,int* in_clstrs,int* ele_loc,int k,int d,int n)
{
    int i;
    int j;
    int l;
    double sum=0.0;
    int flag;
    int min_index;
    for (i=0; i < n; i++){
        min_index = 0;
        flag = 0;
        for (j=0; j < k; j++){
            sum = 0.0;
                for (l = 0; l < d; l++)
                    sum += pow((elements[i][l] - centeroids[j][l]),2);
            sum = pow(sum,0.5);
            if (flag == 0){
                min = sum;
                flag = 1;
            }
            else if (sum < min){
                min = sum;
                min_index = j;
            }
        }
        in_clstrs[min_index]+=1;
        ele_loc[i]=min_index;
    }
}

void updateCentroids(double* cntrds,double* ele,int* in_clstrs,int *ele_loc,int d,int n,int k)
{
    int m;
    int i;
    int q;
    for (i=0;i<k;i++){
            for (m=0;m<n;m++){
                if (ele_loc[m]==i){
                    for (q=0;q<d;q++)
                        cntrds[i][q]+=ele[m][q];
                }
            }
            for (q=0;q<d;q++){
                if (cntrds[i][q]==0){
                    continue;
                }
                cntrds[i][q]=cntrds[i][q]/(double)in_clstrs[i];
            }
        }
}

int Convergence(double* old_centroids,double* centroids,int k,int d,int eps){
    int bit = 0;
    int i;
    double sum;
    for(i=0; i<k; i++){
        sum = 0.0;
        for (j = 0; j<d; j++)
            sum += pow((old_centeroids[i][j] + centeroids[i][j]), 2);
        sum = pow(sum, 0.5);
        if (sum >= eps)
            bit = 1;
    }
    return bit;
}

 //Python C-API functions

static PyMethodDef spkmeansmoduleMethods[] = 
{
        {"fit", (PyCFunction) fit, METH_VARARGS, PyDoc_STR("Kmeans++")},
        {"get_matrix", (PyCFunction) get_matrix, METH_VARARGS, PyDoc_STR("c usage for getting the matrixes")},
        {NULL,  NULL, 0, NULL}
};

static struct PyModuleDef _moduledef = 
{
        PyModuleDef_HEAD_INIT,
        "spkmeansmodule",
        NULL,
        -1,
        spkmeansmoduleMethods
};


PyMODINIT_FUNC PyInit_spkmeansmodule(void)
{
    PyObject *m;
    m = PyModule_Create(&_moduledef);
    if(!m){
        return NULL;
    }
    return m;
}
