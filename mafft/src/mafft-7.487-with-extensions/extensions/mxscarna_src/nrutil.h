/*
  McCaskill's Algorithm -- The algorithm calculates a base paring probability matrix from the input of one sequence.

  $Id: nrutil.h,v 1.0 2005/10/20 14:22 $;

  Copyright (C) 2005 Yasuo Tabei <tabei@cb.k.u-tokyo.ac.jp>

  This is free software with ABSOLUTELY NO WARRANTY.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	USA
*/

#ifndef _NR_UTIL_H_
#define _NR_UTIL_H_
#include <string>
#include <cmath>
#include <complex>
#include <iostream>
#include <cstdlib> // by katoh

using namespace std;

typedef double DP;

template<class T>
inline const T SQR(const T a) {return a*a;}

template<class T>
inline const T MAX(const T &a, const T &b)
{return b > a ? (b) : (a);}

inline float MAX(const double &a, const float &b)
{return b > a ? (b) : float(a);}

inline float MAX(const float &a, const double &b)
{return b > a ? float(b) : (a);}

template<class T>
inline const T MIN(const T &a, const T &b)
{return b < a ? (b) : (a);}

inline float MIN(const double &a, const float &b)
{return b < a ? (b) : float(a);}

inline float MIN(const float &a, const double &b)
{return b < a ? float(b) : (a);}

template<class T>
inline const T SIGN(const T &a, const T &b)
{return b >= 0 ? (a >= 0 ? a : -a) : (a >= 0 ? -a : a);}

inline float SIGN(const float &a, const double &b)
{return b >= 0 ? (a >= 0 ? a : -a) : (a >= 0 ? -a : a);}

inline float SIGN(const double &a, const float &b)
{return b >= 0 ? (a >= 0 ? a : -a) : (a >= 0 ? -a : a);}

template<class T>
inline void SWAP(T &a, T &b)
{T dum=a; a=b; b=dum;}
namespace NR {
    inline void nrerror(const string error_text)
// Numerical Recipes standard error handler
	{
	    cerr << "Numerical Recipes run-time error..." << endl;
	    cerr << error_text << endl;
	    cerr << "...now exiting to system..." << endl;
	    exit(1);
	}
}

template <class T>
class NRVec {
 private:
    int nn; // size of array. upper index is nn-1
    T *v;
 public:
    NRVec();
    explicit NRVec(int n);    // Zero-based array
    NRVec(const T &a, int n); //initialize to constant value
    NRVec(const T *a, int n); // Initialize to array
    NRVec(const NRVec &rhs);  // Copy constructor
    NRVec & operator=(const NRVec &rhs); //assignment
    NRVec & operator=(const T &a); //assign a to every element
    inline T & operator[](const int i); //i¡Çth element
    inline const T & operator[](const int i) const;
    void Allocator(int i = 0); // by Steffen, mpg
    inline int size() const;
    ~NRVec();
};

template <class T>
NRVec<T>::NRVec() : nn(0), v(0) {}

template <class T>
NRVec<T>::NRVec(int n) : nn(n), v(new T[n]) {}

template <class T>
NRVec<T>::NRVec(const T& a, int n) : nn(n), v(new T[n])
{
    for(int i=0; i<n; i++)
	v[i] = a;
}

template <class T>
NRVec<T>::NRVec(const T *a, int n) : nn(n), v(new T[n])
{
for(int i=0; i<n; i++)
    v[i] = *a++;
}

template <class T>
void NRVec<T>::Allocator(int n) // by Steffen
{
   v = new T[n];
}

template <class T>
NRVec<T>::NRVec(const NRVec<T> &rhs) : nn(rhs.nn), v(new T[nn])
{
    for(int i=0; i<nn; i++)
	v[i] = rhs[i];
}

template <class T>
NRVec<T> & NRVec<T>::operator=(const NRVec<T> &rhs)
// postcondition: normal assignment via copying has been performed;
// if vector and rhs were different sizes, vector
// has been resized to match the size of rhs
{
    if (this != &rhs)
{
    if (nn != rhs.nn) {
	if (v != 0) delete [] (v);
	nn=rhs.nn;
	v= new T[nn];
    }
    for (int i=0; i<nn; i++)
	v[i]=rhs[i];
}
    return *this;
}

template <class T>
NRVec<T> & NRVec<T>::operator=(const T &a) //assign a to every element
{
    for (int i=0; i<nn; i++)
	v[i]=a;
    return *this;
}

template <class T>
inline T & NRVec<T>::operator[](const int i) //subscripting
{
    return v[i];
}

template <class T>
inline const T & NRVec<T>::operator[](const int i) const //subscripting
{
    return v[i];
}

template <class T>
inline int NRVec<T>::size() const
{
    return nn;
}

template <class T>
NRVec<T>::~NRVec()
{
    if (v != 0)
	delete[] (v);
}

template <class T>
class NRMat {
 private:
    int nn;
    int mm;
    T **v;
 public:
    NRMat();
    NRMat(int n, int m); // Zero-based array
    NRMat(const T &a, int n, int m); //Initialize to constant
    NRMat(const T *a, int n, int m); // Initialize to array
    NRMat(const NRMat &rhs); // Copy constructor
    void Allocator(int n, int m);
    void Allocator(const T &a, int n, int m);
    void Allocator(const T *a, int n, int m);
    NRMat & operator=(const NRMat &rhs); //assignment
    NRMat & operator=(const T &a); //assign a to every element
    inline T* operator[](const int i); //subscripting: pointer to row i
    inline const T* operator[](const int i) const;
    inline T &  ref(const int i, const int j);
    inline const T ref(const int i, const int j) const;
    inline int nrows() const;
    inline int ncols() const;
    ~NRMat();
};

template <class T>
NRMat<T>::NRMat() : nn(0), mm(0), v(0) {}

template <class T>
NRMat<T>::NRMat(int n, int m) : nn(n), mm(m), v(new T*[n])
{
    v[0] = new T[m*n];
    for (int i=1; i< n; i++)
	v[i] = v[i-1] + m;
}

template <class T>
NRMat<T>::NRMat(const T &a, int n, int m) : nn(n), mm(m), v(new T*[n])
{
    int i,j;
    v[0] = new T[m*n];
    for (i=1; i< n; i++)
	v[i] = v[i-1] + m;
    for (i=0; i< n; i++)
	for (j=0; j<m; j++)
	    v[i][j] = a;
}

template <class T>
NRMat<T>::NRMat(const T *a, int n, int m) : nn(n), mm(m), v(new T*[n])
{
    int i,j;
    v[0] = new T[m*n];
    for (i=1; i< n; i++)
	v[i] = v[i-1] + m;
    for (i=0; i< n; i++)
	for (j=0; j<m; j++)
	    v[i][j] = *a++;
}

template <class T> 
void NRMat<T>::Allocator(int n, int m)
{
    if( v != 0 ) {
	delete[] (v[0]); delete (v);
    }

    nn = n; mm = m; v = new T*[n];
    
    v[0] = new T[m*n];
    for (int i=1; i< n; i++)
	v[i] = v[i-1] + m;
}

template <class T>
void NRMat<T>::Allocator(const T &a, int n, int m)
{
    if( v != 0 ) {
	delete[] (v[0]); delete (v);
    }

    int i,j;

    nn = n; mm = m; v = new T*[n];

    v[0] = new T[m*n];
    for (i=1; i< n; i++)
	v[i] = v[i-1] + m;
    for (i=0; i< n; i++)
	for (j=0; j<m; j++)
	    v[i][j] = a;
}

template <class T>
void NRMat<T>::Allocator(const T *a, int n, int m)
{
    if( v != 0 ) {
	delete[] (v[0]); delete (v);
    }

    int i,j;

    nn = n; mm = m; v = new T*[n];
    
    v[0] = new T[m*n];
    for (i=1; i< n; i++)
	v[i] = v[i-1] + m;
    for (i=0; i< n; i++)
	for (j=0; j<m; j++)
	    v[i][j] = *a++;
}

template <class T>
NRMat<T>::NRMat(const NRMat &rhs) : nn(rhs.nn), mm(rhs.mm), v(new T*[nn])
{
    int i,j;
    v[0] = new T[mm*nn];
    for (i=1; i< nn; i++)
	v[i] = v[i-1] + mm;
    for (i=0; i< nn; i++)
	for (j=0; j<mm; j++)
	    v[i][j] = rhs[i][j];
}
template <class T>
NRMat<T> & NRMat<T>::operator=(const NRMat<T> &rhs)
// postcondition: normal assignment via copying has been performed;
// if matrix and rhs were different sizes, matrix
// has been resized to match the size of rhs
{
    if (this != &rhs) {
	int i,j;
	if (nn != rhs.nn || mm != rhs.mm) {
	    if (v != 0) {
		delete[] (v[0]);
		delete[] (v);
	    }
	    nn=rhs.nn;
	    mm=rhs.mm;
	    v = new T*[nn];
	    v[0] = new T[mm*nn];
	}
	for (i=1; i< nn; i++)
	    v[i] = v[i-1] + mm;
	for (i=0; i< nn; i++)
	    for (j=0; j<mm; j++)
		v[i][j] = rhs[i][j];
    }
    return *this;
}

template <class T>
NRMat<T> & NRMat<T>::operator=(const T &a) //assign a to every element
{
    for (int i=0; i< nn; i++)
	for (int j=0; j<mm; j++)
	    v[i][j] = a;
    return *this;
}

template <class T>
inline T* NRMat<T>::operator[](const int i) //subscripting: pointer to row i
{
    return v[i];
}

template <class T>
inline const T* NRMat<T>::operator[](const int i) const
{
    return v[i];
}

template <class T>
inline T &  NRMat<T>::ref(const int i, const int j)
{
    return v[i][j];
}

template <class T>
inline const T NRMat<T>::ref(const int i, const int j) const
{
    return v[i][j];
}

template <class T>
inline int NRMat<T>::nrows() const
{
    return nn;
}

template <class T>
inline int NRMat<T>::ncols() const
{
    return mm;
}

template <class T>
NRMat<T>::~NRMat()
{
    if (v != 0) {
	delete[] (v[0]);
	delete[] (v);
    }
}

template <class T>
class NRMat3d {
 private:
    int nn;
    int mm;
    int kk;
    T ***v;
 public:
    NRMat3d();
    NRMat3d(int n, int m, int k);
    inline void Allocator(int n, int m, int k);
    inline T** operator[](const int i); //subscripting: pointer to row i
    inline const T* const * operator[](const int i) const;
    inline int dim1() const;
    inline int dim2() const;
    inline int dim3() const;
    ~NRMat3d();
};

template <class T>
NRMat3d<T>::NRMat3d(): nn(0), mm(0), kk(0), v(0) {}
template <class T>
NRMat3d<T>::NRMat3d(int n, int m, int k) : nn(n), mm(m), kk(k), v(new T**[n])
{
    int i,j;
    v[0] = new T*[n*m];
    v[0][0] = new T[n*m*k];
    for(j=1; j<m; j++)
	v[0][j] = v[0][j-1] + k;
    for(i=1; i<n; i++) {
	v[i] = v[i-1] + m;
	v[i][0] = v[i-1][0] + m*k;
	for(j=1; j<m; j++)
	    v[i][j] = v[i][j-1] + k;
    }
}

template <class T>
inline void NRMat3d<T>::Allocator(int n, int m, int k) 
{
    int i,j;
    v[0] = new T*[n*m];
    v[0][0] = new T[n*m*k];
    for(j=1; j<m; j++)
	v[0][j] = v[0][j-1] + k;
    for(i=1; i<n; i++) {
	v[i] = v[i-1] + m;
	v[i][0] = v[i-1][0] + m*k;
	for(j=1; j<m; j++)
	    v[i][j] = v[i][j-1] + k;
    }
}

template <class T>
inline T** NRMat3d<T>::operator[](const int i) //subscripting: pointer to row i
{
    return v[i];
}

template <class T>
inline const T* const * NRMat3d<T>::operator[](const int i) const
{
    return v[i];
}

template <class T>
inline int NRMat3d<T>::dim1() const
{
    return nn;
}

template <class T>
inline int NRMat3d<T>::dim2() const
{
    return mm;
}

template <class T>
inline int NRMat3d<T>::dim3() const
{
    return kk;
}

template <class T>
NRMat3d<T>::~NRMat3d()
{
    if (v != 0) {
	delete[] (v[0][0]);
	delete[] (v[0]);
	delete[] (v);
    }
}

//The next 3 classes are used in artihmetic coding, Huffman coding, and
//wavelet transforms respectively. This is as good a place as any to put them!
class arithcode {
 private:
    NRVec<unsigned long> *ilob_p,*iupb_p,*ncumfq_p;
 public:
    NRVec<unsigned long> &ilob,&iupb,&ncumfq;
    unsigned long jdif,nc,minint,nch,ncum,nrad;
    arithcode(unsigned long n1, unsigned long n2, unsigned long n3)
	: ilob_p(new NRVec<unsigned long>(n1)),
	iupb_p(new NRVec<unsigned long>(n2)),
	ncumfq_p(new NRVec<unsigned long>(n3)),
	ilob(*ilob_p),iupb(*iupb_p),ncumfq(*ncumfq_p) {}
    ~arithcode() {
	if (ilob_p != 0) delete ilob_p;
	if (iupb_p != 0) delete iupb_p;
	if (ncumfq_p != 0) delete ncumfq_p;
    }
};

class huffcode {
 private:
    NRVec<unsigned long> *icod_p,*ncod_p,*left_p,*right_p;
 public:
    NRVec<unsigned long> &icod,&ncod,&left,&right;
    int nch,nodemax;
    huffcode(unsigned long n1, unsigned long n2, unsigned long n3,
	     unsigned long n4) :
	icod_p(new NRVec<unsigned long>(n1)),
	ncod_p(new NRVec<unsigned long>(n2)),
	left_p(new NRVec<unsigned long>(n3)),
	right_p(new NRVec<unsigned long>(n4)),
	icod(*icod_p),ncod(*ncod_p),left(*left_p),right(*right_p) {}
    ~huffcode() {
	if (icod_p != 0) delete icod_p;
	if (ncod_p != 0) delete ncod_p;
	if (left_p != 0) delete left_p;
	if (right_p != 0) delete right_p;
    }
};

class wavefilt {
 private:
    NRVec<DP> *cc_p,*cr_p;
 public:
    int ncof,ioff,joff;
    NRVec<DP> &cc,&cr;
    wavefilt() : cc(*cc_p),cr(*cr_p) {}
    wavefilt(const DP *a, const int n) : //initialize to array
	cc_p(new NRVec<DP>(n)),cr_p(new NRVec<DP>(n)),
	ncof(n),ioff(-(n >> 1)),joff(-(n >> 1)),cc(*cc_p),cr(*cr_p) {
	int i;
	for (i=0; i<n; i++)
	    cc[i] = *a++;
	DP sig = -1.0;
	for (i=0; i<n; i++) {
	    cr[n-1-i]=sig*cc[i];
	    sig = -sig;
	}
    }
    ~wavefilt() {
	if (cc_p != 0) delete cc_p;
	if (cr_p != 0) delete cr_p;
    }
};


/* Triangle Matrix Class
   ---------------------------------------------------------
   |v[0][0]|v[0][1]|      |            |         |v[0][n-1]|
   |-------|-------|------|------------|---------|---------|
   |v[1][1]|v[1][2]|      |            |v[1][n-2]|         |
   |-------|-------|------|------------|---------|---------|
   |       |       |      |            |         |         |
   |-------|-------|------|------------|---------|---------|
   |       |                                               |     
   |       |                                               |
   |       |                                               |
   |-------|-----------------------------------------------|
   |       |                                               |
   |-------|-----------------------------------------------|
   |v[n-2][0]|v[n-2][1]|                                   |                         
   |-------|-----------------------------------------------|
   |v[n-1][0]|                                             |  
   |-------------------------------------------------------|
 */
template <class T>
class Trimat {
 private:
    int nn;
    T **v;
    inline T* operator[](const int i); //subscripting: pointer to row i
    inline const T* operator[](const int i) const;
 public:
    Trimat();
    Trimat(int n); // Zero-based array
    Trimat(const T &a, int n); //Initialize to constant
    Trimat(const T *a, int n); // Initialize to array
    Trimat(const Trimat &rhs); // Copy constructor
    void Allocator(int n);
    void Allocator(const T &a, int n);
    void Allocator(const T *a, int n);
    Trimat & operator=(const Trimat &rhs); //assignment
    Trimat & operator=(const T &a); //assign a to every element
    inline T & ref(const int i, const int j);
    inline T * getPointer(const int i, const int j);
    inline T * begin() const;
    inline T * end() const;
    inline const T ref(const int i, const int j) const;
    inline int nrows() const;
    ~Trimat();
};

template <class T>
Trimat<T>::Trimat() : nn(0), v(0) {}

template <class T>
Trimat<T>::Trimat(int n) : nn(n), v(new T*[n])
{
    v[0] = new T[n*(n+1)/2];
    for (int i=1; i< n; i++) 
	v[i] = v[i-1] + (n-i+1);

    for (int i=0; i< n; i++)
	for (int j=0; j<(n-i); j++)
	    v[i][j] = 0;
}
template <class T>
Trimat<T>::Trimat(const T &a, int n) : nn(n), v(new T*[n])
{
    int i,j;
    v[0] = new T[n*(n+1)/2];
    for (i=1; i< n; i++)
	v[i] = v[i-1] + (n-i+1);
    for (i=0; i< n; i++)
	for (j=0; j<(n-i); j++)
	    v[i][j] = a;
}

template <class T>
Trimat<T>::Trimat(const T *a, int n) : nn(n), v(new T*[n])
{
    int i,j;
    v[0] = new T[n*(n+1)/2];
    for (i=1; i< n; i++)
	v[i] = v[i-1] + (n-i+1);
    for (i=0; i< n; i++)
	for (j=0; j<(n-i); j++)
	    v[i][j] = *a++;
}


template <class T>
void Trimat<T>::Allocator(int n)
{
    nn = n; v = new T*[n];

    v[0] = new T[n*(n+1)/2];
    for (int i=1; i< n; i++)
	v[i] = v[i-1] + (n-i+1);
}

template <class T>
void Trimat<T>::Allocator(const T &a, int n)
{
    nn = n; v = new T*[n];

    int i,j;
    v[0] = new T[n*(n+1)/2];
    for (i=1; i < n; i++)
	v[i] = v[i-1] + (n-i+1);
    for (i=0; i < n; i++)
	for (j=0; j < (n-i); j++)
	    v[i][j] = a;
}

template <class T>
void Trimat<T>::Allocator(const T *a, int n)
{
    nn = n; v = new T*[n];
    int i,j;
    v[0] = new T[n*(n+1)/2];
    for (i=1; i< n; i++)
	v[i] = v[i-1] + (n-i+1);
    for (i=0; i< n; i++)
	for (j=0; j<(n-i); j++)
	    v[i][j] = *a++;
}


template <class T>
Trimat<T>::Trimat(const Trimat &rhs) : nn(rhs.nn), v(new T*[nn])
{
    int i,j;
    v[0] = new T[nn*(nn+1)/2];
    for (i=1; i< nn; i++)
	v[i] = v[i-1] + (nn-i+1);
    for (i=0; i< nn; i++)
	for (j=0; j<(nn-i); j++)
	    v[i][j] = rhs[i][j];
}
template <class T>
Trimat<T> & Trimat<T>::operator=(const Trimat<T> &rhs)
// postcondition: normal assignment via copying has been performed;
// if matrix and rhs were different sizes, matrix
// has been resized to match the size of rhs
{
    if (this != &rhs) {
	int i,j;
	if (nn != rhs.nn) {
	    if (v != 0) {
		delete[] (v[0]);
		delete[] (v);
	    }
	    nn=rhs.nn;
	    v = new T*[nn];
	    v[0] = new T[nn*(nn+1)/2];
	}
	for (i=1; i< nn; i++)
	    v[i] = v[i-1] + (nn-i+1);
	for (i=0; i< nn; i++)
	    for (j=0; j<(nn-i); j++)
		v[i][j] = rhs[i][j];
    }
    return *this;
}

template <class T>
Trimat<T> & Trimat<T>::operator=(const T &a) //assign a to every element
{
    for (int i=0; i< nn; i++)
	for (int j=0; j<nn-i; j++)
	    v[i][j] = a;
    return *this;
}

template <class T>
inline T &  Trimat<T>::ref(const int i, const int j)
{
    return v[i][j-i];
}

template <class T>
inline const T Trimat<T>::ref(const int i, const int j) const
{
    return v[i][j-i];
}

template <class T>
inline T * Trimat<T>::getPointer(const int i, const int j) 
{
    return &v[i][j-i];
}

template <class T>
inline T * Trimat<T>::begin() const
{
    return &v[0][0];
}

template <class T>
inline T * Trimat<T>::end() const
{
    return (&v[nn-1][0] + 1);
}

template <class T>
inline int Trimat<T>::nrows() const
{
    return nn;
}

template <class T>
Trimat<T>::~Trimat()
{
    if (v != 0) {
	delete[] (v[0]);
	delete[] (v);
    }
}


/* Triangle Vertical Matrix Class
   ---------------------------------------------------------
   |v[0][0]|v[1][0]|      |            |         |v[n-1][0]|
   |-------|-------|------|------------|---------|---------|
   |v[0][1]|v[1][1]|      |            |v[n-2][1]|         |
   |-------|-------|------|------------|---------|---------|
   |       |       |      |            |         |         |
   |-------|-------|------|------------|---------|---------|
   |       |                                               |     
   |       |                                               |
   |       |                                               |
   |-------|-----------------------------------------------|
   |       |                                               |
   |-------|-----------------------------------------------|
   |v[0][n-2]|v[n-2][n-2]|                                   |                         
   |-------|-----------------------------------------------|
   |v[0][n-1]|                                             |  
   |-------------------------------------------------------|
 */
template <class T>
class TriVertMat {
 private:
    int nn;
    T **v;
    inline T* operator[](const int i); //subscripting: pointer to row i
    inline const T* operator[](const int i) const;
 public:
    TriVertMat();
    TriVertMat(int n); // Zero-based array
    TriVertMat(const T &a, int n); //Initialize to constant
    TriVertMat(const T *a, int n); // Initialize to array
    TriVertMat(const TriVertMat &rhs); // Copy constructor
    void Allocator(int n);
    void Allocator(const T &a, int n);
    void Allocator(const T *a, int n);
    TriVertMat & operator=(const TriVertMat &rhs); //assignment
    TriVertMat & operator=(const T &a); //assign a to every element
    inline T & ref(const int i, const int j);
    inline T * getPointer(const int i, const int j);
    inline const T ref(const int i, const int j) const;
    inline int nrows() const;
    ~TriVertMat();
};

template <class T>
TriVertMat<T>::TriVertMat() : nn(0), v(0) {}

template <class T>
TriVertMat<T>::TriVertMat(int n) : nn(n), v(new T*[n])
{
    v[0] = new T[n*(n+1)/2];
    for (int i=1; i< n; i++)
	v[i] = v[i-1] + i;
}

template <class T>
TriVertMat<T>::TriVertMat(const T &a, int n) : nn(n), v(new T*[n])
{
    int i,j;
    v[0] = new T[n*(n+1)/2];
    for (i=1; i< n; i++)
	v[i] = v[i-1] + i;
    for (i=0; i< n; i++)
	for (j=0; j<(n-i); j++)
	    v[i][j] = a;
}

template <class T>
TriVertMat<T>::TriVertMat(const T *a, int n) : nn(n), v(new T*[n])
{
    int i,j;
    v[0] = new T[n*(n+1)/2];
    for (i=1; i< n; i++)
	v[i] = v[i-1] + i;
    for (i=0; i< n; i++)
	for (j=0; j<(n-i); j++)
	    v[i][j] = *a++;
}


template <class T>
void TriVertMat<T>::Allocator(int n)
{
    nn = n; v = new T*[n];

    v[0] = new T[n*(n+1)/2];
    for (int i=1; i< n; i++)
	v[i] = v[i-1] + i;
}

template <class T>
void TriVertMat<T>::Allocator(const T &a, int n)
{
    nn = n; v = new T*[n];

    int i,j;
    v[0] = new T[n*(n+1)/2];
    for (i=1; i< n; i++)
	v[i] = v[i-1] + i;
    for (i=0; i< n; i++)
	for (j=0; j<(n-i); j++)
	    v[i][j] = a;
}

template <class T>
void TriVertMat<T>::Allocator(const T *a, int n)
{
    nn = n; v = new T*[n];
    int i,j;
    v[0] = new T[n*(n+1)/2];
    for (i=1; i< n; i++)
	v[i] = v[i-1] + i;
    for (i=0; i< n; i++)
	for (j=0; j<(n-i); j++)
	    v[i][j] = *a++;
}


template <class T>
TriVertMat<T>::TriVertMat(const TriVertMat &rhs) : nn(rhs.nn), v(new T*[nn])
{
    int i,j;
    v[0] = new T[nn*(nn+1)/2];
    for (i=1; i< nn; i++)
	v[i] = v[i-1] + i;
    for (i=0; i< nn; i++)
	for (j=0; j<(nn-i); j++)
	    v[i][j] = rhs[i][j];
}
template <class T>
TriVertMat<T> & TriVertMat<T>::operator=(const TriVertMat<T> &rhs)
// postcondition: normal assignment via copying has been performed;
// if matrix and rhs were different sizes, matrix
// has been resized to match the size of rhs
{
    if (this != &rhs) {
	int i,j;
	if (nn != rhs.nn) {
	    if (v != 0) {
		delete[] (v[0]);
		delete[] (v);
	    }
	    nn=rhs.nn;
	    v = new T*[nn];
	    v[0] = new T[nn*(nn+1)/2];
	}
	for (i=1; i< nn; i++)
	    v[i] = v[i-1] + i;
	for (i=0; i< nn; i++)
	    for (j=0; j<(nn-i); j++)
		v[i][j] = rhs[i][j];
    }
    return *this;
}

template <class T>
TriVertMat<T> & TriVertMat<T>::operator=(const T &a) //assign a to every element
{
    for (int i=0; i< nn; i++)
	for (int j=0; j<nn-i; j++)
	    v[i][j] = a;
    return *this;
}

template <class T>
inline T &  TriVertMat<T>::ref(const int i, const int j)
{
    return v[j][i];
}

template <class T>
inline const T TriVertMat<T>::ref(const int i, const int j) const
{
    return v[j][i];
}

template <class T>
inline T * TriVertMat<T>::getPointer(const int i, const int j) 
{
    return &v[j][i];
}

template <class T>
inline int TriVertMat<T>::nrows() const
{
    return nn;
}

template <class T>
TriVertMat<T>::~TriVertMat()
{
    if (v != 0) {
	delete[] (v[0]);
	delete[] (v);
    }
}


//Overloaded complex operations to handle mixed float and double
//This takes care of e.g. 1.0/z, z complex<float>
inline const complex<float> operator+(const double &a,
				      const complex<float> &b) { return float(a)+b; }
inline const complex<float> operator+(const complex<float> &a,
				      const double &b) { return a+float(b); }
inline const complex<float> operator-(const double &a,
				      const complex<float> &b) { return float(a)-b; }
inline const complex<float> operator-(const complex<float> &a,
				      const double &b) { return a-float(b); }
inline const complex<float> operator*(const double &a,
				      const complex<float> &b) { return float(a)*b; }
inline const complex<float> operator*(const complex<float> &a,
				      const double &b) { return a*float(b); }
inline const complex<float> operator/(const double &a,
				      const complex<float> &b) { return float(a)/b; }
inline const complex<float> operator/(const complex<float> &a,
				      const double &b) { return a/float(b); }
//some compilers choke on pow(float,double) in single precision. also atan2
inline float pow (float x, double y) {return pow(double(x),y);}
inline float pow (double x, float y) {return pow(x,double(y));}
inline float atan2 (float x, double y) {return atan2(double(x),y);}
inline float atan2 (double x, float y) {return atan2(x,double(y));}

#endif /* _NR_UTIL_H_ */
