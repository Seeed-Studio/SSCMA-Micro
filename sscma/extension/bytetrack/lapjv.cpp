#include "lapjv.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>

#define LARGE 1000000

#define SWAP_INDICES(a, b) \
    {                      \
        a = a ^ b;         \
        b = a ^ b;         \
        a = a ^ b;         \
    }

#define NEW(x, t, n)                 \
    if ((x = new t[n]) == nullptr) { \
        return -1;                   \
    }

#define FREE(x)         \
    if (x != nullptr) { \
        delete[] x;     \
        x = nullptr;    \
    }

int _ccrrt_dense(const unsigned int n, double* cost[], int* free_rows, int* x, int* y, double* v) {
    int   n_free_rows;
    char* unique;

    for (unsigned int i = 0; i < n; i++) {
        x[i] = -1;
        v[i] = LARGE;
        y[i] = 0;
    }
    for (unsigned int i = 0; i < n; i++) {
        for (unsigned int j = 0; j < n; j++) {
            const double c = cost[i][j];
            if (c < v[j]) {
                v[j] = c;
                y[j] = i;
            }
        }
    }

    NEW(unique, char, n);
    std::memset(unique, 1, n);
    {
        int j = n;
        do {
            j--;
            const int i = y[j];
            if (x[i] < 0) {
                x[i] = j;
            } else {
                unique[i] = 0;
                y[j]      = -1;
            }
        } while (j > 0);
    }
    n_free_rows = 0;
    for (unsigned int i = 0; i < n; i++) {
        if (x[i] < 0) {
            free_rows[n_free_rows++] = i;
        } else if (unique[i]) {
            const int j   = x[i];
            double    min = LARGE;
            for (unsigned int j2 = 0; j2 < n; j2++) {
                if (j2 == (unsigned int)j) {
                    continue;
                }
                const double c = cost[i][j2] - v[j2];
                if (c < min) {
                    min = c;
                }
            }

            v[j] -= min;
        }
    }
    FREE(unique);
    return n_free_rows;
}

int _carr_dense(
  const unsigned int n, double* cost[], const unsigned int n_free_rows, int* free_rows, int* x, int* y, double* v) {
    unsigned int current       = 0;
    int          new_free_rows = 0;
    unsigned int rr_cnt        = 0;

    while (current < n_free_rows) {
        int    i0;
        int    j1, j2;
        double v1, v2, v1_new;
        char   v1_lowers;

        rr_cnt++;

        const int free_i = free_rows[current++];
        j1               = 0;
        v1               = cost[free_i][0] - v[0];
        j2               = -1;
        v2               = LARGE;
        for (unsigned int j = 1; j < n; j++) {
            const double c = cost[free_i][j] - v[j];
            if (c < v2) {
                if (c >= v1) {
                    v2 = c;
                    j2 = j;
                } else {
                    v2 = v1;
                    v1 = c;
                    j2 = j1;
                    j1 = j;
                }
            }
        }
        i0        = y[j1];
        v1_new    = v[j1] - (v2 - v1);
        v1_lowers = v1_new < v[j1];

        if (rr_cnt < current * n) {
            if (v1_lowers) {
                v[j1] = v1_new;
            } else if (i0 >= 0 && j2 >= 0) {
                j1 = j2;
                i0 = y[j2];
            }
            if (i0 >= 0) {
                if (v1_lowers) {
                    free_rows[--current] = i0;
                } else {
                    free_rows[new_free_rows++] = i0;
                }
            }
        } else {
            if (i0 >= 0) {
                free_rows[new_free_rows++] = i0;
            }
        }
        x[free_i] = j1;
        y[j1]     = free_i;
    }
    return new_free_rows;
}

unsigned int _find_dense(const unsigned int n, unsigned int lo, double* d, int* cols, int* y) {
    unsigned int hi   = lo + 1;
    double       mind = d[cols[lo]];
    for (unsigned int k = hi; k < n; k++) {
        int j = cols[k];
        if (d[j] <= mind) {
            if (d[j] < mind) {
                hi   = lo;
                mind = d[j];
            }
            cols[k]    = cols[hi];
            cols[hi++] = j;
        }
    }
    return hi;
}

int _scan_dense(const unsigned int n,
                double*            cost[],
                unsigned int*      plo,
                unsigned int*      phi,
                double*            d,
                int*               cols,
                int*               pred,
                int*               y,
                double*            v) {
    unsigned int lo = *plo;
    unsigned int hi = *phi;
    double       h, cred_ij;

    while (lo != hi) {
        int          j    = cols[lo++];
        const int    i    = y[j];
        const double mind = d[j];
        h                 = cost[i][j] - v[j] - mind;

        for (unsigned int k = hi; k < n; k++) {
            j       = cols[k];
            cred_ij = cost[i][j] - v[j] - h;
            if (cred_ij < d[j]) {
                d[j]    = cred_ij;
                pred[j] = i;
                if (cred_ij == mind) {
                    if (y[j] < 0) {
                        return j;
                    }
                    cols[k]    = cols[hi];
                    cols[hi++] = j;
                }
            }
        }
    }
    *plo = lo;
    *phi = hi;
    return -1;
}

int find_path_dense(const unsigned int n, double* cost[], const int start_i, int* y, double* v, int* pred) {
    unsigned int lo = 0, hi = 0;
    int          final_j = -1;
    unsigned int n_ready = 0;
    int*         cols;
    double*      d;

    NEW(cols, int, n);
    NEW(d, double, n);

    for (unsigned int i = 0; i < n; i++) {
        cols[i] = i;
        pred[i] = start_i;
        d[i]    = cost[start_i][i] - v[i];
    }

    while (final_j == -1) {
        if (lo == hi) {
            n_ready = lo;
            hi      = _find_dense(n, lo, d, cols, y);

            for (unsigned int k = lo; k < hi; k++) {
                const int j = cols[k];
                if (y[j] < 0) {
                    final_j = j;
                }
            }
        }
        if (final_j == -1) {
            final_j = _scan_dense(n, cost, &lo, &hi, d, cols, pred, y, v);
        }
    }

    {
        const double mind = d[cols[lo]];
        for (unsigned int k = 0; k < n_ready; k++) {
            const int j = cols[k];
            v[j] += d[j] - mind;
        }
    }

    FREE(cols);
    FREE(d);

    return final_j;
}

int _ca_dense(
  const unsigned int n, double* cost[], const unsigned int n_free_rows, int* free_rows, int* x, int* y, double* v) {
    int* pred;

    NEW(pred, int, n);

    for (int* pfree_i = free_rows; pfree_i < free_rows + n_free_rows; pfree_i++) {
        int          i = -1, j;
        unsigned int k = 0;

        j = find_path_dense(n, cost, *pfree_i, y, v, pred);

        if (j < 0 || j >= n) {
            FREE(pred);
            return -1;
        }

        while (i != *pfree_i) {
            i = pred[j];

            y[j] = i;

            SWAP_INDICES(j, x[i]);
            k++;
            if (k >= n) {
                FREE(pred);
                return -1;
            }
        }
    }

    FREE(pred);
    return 0;
}

int lapjv_internal(const unsigned int n, double* cost[], int* x, int* y) {
    int     ret;
    int*    free_rows;
    double* v;

    NEW(free_rows, int, n);
    NEW(v, double, n);
    ret   = _ccrrt_dense(n, cost, free_rows, x, y, v);
    int i = 0;
    while (ret > 0 && i < 2) {
        ret = _carr_dense(n, cost, ret, free_rows, x, y, v);
        i++;
    }
    if (ret > 0) {
        ret = _ca_dense(n, cost, ret, free_rows, x, y, v);
    }
    FREE(v);
    FREE(free_rows);

    return ret;
}