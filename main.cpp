#include <iostream>
#include <fstream>
#include <Windows.h>
#include <malloc.h>
#include <type_traits>
#include <typeinfo>
#include <iomanip>
#include <exception>

using namespace std;

typedef long long int64_t;
typedef unsigned long long uint64_t;
typedef long int32_t;
typedef unsigned long uint32_t;
typedef short int16_t;

class Timer {
	int64_t _nano_stamp;
	int64_t _win_stamp;
public:
	Timer() : _nano_stamp(0), _win_stamp(0) {}
	static uint64_t nano_clock() {
		//EDX:EAX 64-битное количество тактов с момента последнего сброса процессора.
		//rdtsc
		uint32_t high_part = 0;
		uint32_t low_part = 0;
		uint64_t time_stamp = 0;
		__asm {
			rdtsc
			mov low_part, eax
			mov high_part, edx
		}
		time_stamp = high_part;
		return (time_stamp <<= 32) |= low_part;
	}
	static uint64_t _nano_clock() {	return __rdtsc();	}
	
	void nano_start() {		_nano_stamp = __rdtsc();	}
	uint64_t nano_clock_diff() { return __rdtsc() - _nano_stamp;	}

	static uint64_t win_clock() {
		LARGE_INTEGER time_stamp;
		QueryPerformanceCounter(&time_stamp);
		return time_stamp.QuadPart;
	}

	void win_start() {	_win_stamp = win_clock();	}

	uint64_t win_clock_diff() {	return win_clock() - _win_stamp;	}

	void start(){
		nano_start();
		win_start();
	}

	friend ostream& operator << (ostream& s, Timer& t) {
		s << "Time stamp -> nano: " << t.nano_clock_diff() << "  win: " << t.win_clock_diff();
		return s;
	}
};



template<typename T>
struct Mat {
	size_t width;
	size_t height;
	T** ptr;
	Mat() : width(0), height(0), ptr(nullptr){}
	Mat(size_t height,size_t width) : ptr(nullptr){
		this->width = width;
		this->height = height;
		_alloc_alligned_mat(width,height);
	}
	~Mat() {
		if (ptr != nullptr)
			_delete_alligned_mat();
	}
	void copy(Mat& mat) {
		this->width = mat.width;
		this->height = mat.height;
		ptr = mat.ptr;
	}
	Mat(Mat& mat) {
		copy(mat);
		mat.ptr = nullptr;
	}
	//move constr
	
	Mat(Mat&& mat){
		copy(mat);
		mat.ptr = nullptr;
	}
	
	Mat& operator = (Mat& mat) {
		if (this == &mat) {
			return *this;
		}
		copy(mat);
		mat.ptr = nullptr;
		return *this;
	}
	void _alloc_alligned_mat(size_t width, size_t height,size_t alignment = 16) {
		ptr = (T**)_aligned_malloc(sizeof(T*) * height, alignment);
		for (int i = 0; i < height; i++)
			ptr[i] = (T*)_aligned_malloc(sizeof(T) * width, alignment);
	}

	void _delete_alligned_mat() {
		for (size_t i = 0; i < height; i++)
			_aligned_free(ptr[i]);
		_aligned_free(ptr);
	}

	Mat<T> transposit() {
		Mat<T> tr_mat(width,height);
		for (int i = 0; i < height; i++)
			for (int j = 0; j < width; j++)
				tr_mat.ptr[j][i] = ptr[i][j];
		return tr_mat;
	}

	void int_randomize(int low_border = 0, int high_border = 100) {
		
		for (size_t i = 0; i < height; i++)
			for (size_t j = 0; j < width; j++)
				ptr[i][j] = rand() % 100;
	}

	void float_randomize(float low_border = 0.0, float high_border = 1.0) {
		
			for (size_t i = 0; i < height; i++)
				for (size_t j = 0; j < width; j++)
					ptr[i][j] = ((double)(rand() % 100)) / 100;
	}

	friend ostream& operator << (ostream& s, Mat& mat) {
		cout << "Mat" << "<"<< typeid(T).name() << "> " << "(" << mat.height << "," << mat.width << ")" << endl;
		for (size_t i = 0; i < mat.height; i++) {
			for (size_t j = 0; j < mat.width; j++)
				s << setw(10) << left << setprecision(3) << mat.ptr[i][j] << " ";
			cout << endl;
		}
		return s;
	}
	
	

	Mat auto_vectless_mul(Mat& mat) {
		Timer t;
		if (width != mat.height)
			throw runtime_error("mutrix sizes are not suitable");
		Mat res(height,mat.width);
		Mat tr_mat = mat.transposit();
		T* res_row;
		T* tr_row;
		T* cur_row;
		t.start();
		for (int i = 0; i < height; i++)
			for (int k = 0; k < tr_mat.height; k++) {
				res_row = res.ptr[i];
				cur_row = ptr[i];
				tr_row = tr_mat.ptr[k];
				res_row[k] = 0;
				//#pragma loop(no_vector)
				#pragma novector
				for (int j = 0; j < width; j++)
					res_row[k] += cur_row[j] * tr_row[j];
				}
		cout << t << endl;
		return res;
	}

	Mat auto_vec_mul(Mat& mat){
		Timer t;
		if (width != mat.height)
			throw runtime_error("mutrix sizes are not suitable");
		Mat res(height,mat.width);
		Mat tr_mat = mat.transposit();
		T* res_row;
		T* tr_row;
		T* cur_row;
		t.start();
		for (int i = 0; i < height; i++)
			for (int k = 0; k < tr_mat.height; k++) {
				res_row = res.ptr[i];
				cur_row = ptr[i];
				tr_row = tr_mat.ptr[k];
				res_row[k] = 0;
				for (int j = 0; j < width; j++)
					res_row[k] += cur_row[j] * tr_row[j];
				}
		cout << t << endl;
		return res;
	}

    Mat  manual_mul(Mat& mat) {
        Timer t;
        if (width != mat.height)
            throw runtime_error("mutrix sizes are not suitable");
        Mat res(height, mat.width);
        Mat tr_mat = mat.transposit();
		T* row_ptr1,row_ptr2;
		T* row_last;
		T result;
        t.nano_start();
        for (int i = 0; i < height; i++)
            for (int k = 0; k < tr_mat.height; k++) {
				row_ptr1 = & ptr[i];
				row_ptr2 = & tr_mat.ptr[j];
				row_last = & ptr[i][tr_mat.height - 1];
				
            }
    }
};


int main() {
	srand(time(NULL));
	Mat<float> m1(500, 500);
	m1.float_randomize();
	//cout << m1 << endl;
	Mat<float> m2(500, 500);
	m2.float_randomize();
	//cout << m2 << endl;
	
	

	try {
		cout << setw(15) << left << "auto vect:";
		Mat<float> result1 = m1.auto_vec_mul(m2);
		cout << setw(15) << left << "vect less:";
		Mat<float> result2 = m1.auto_vectless_mul(m2);
}
	catch (runtime_error e) {
		cout << e.what() << endl;
	}
	
	cout << endl;
	system("pause");
	return 0;
}