#include <iostream>
#include <fstream>
#include <Windows.h>
#include <malloc.h>
#include <type_traits>
#include <typeinfo>
#include <iomanip>
#include <exception>
#include <map>
#include <string>
#include <algorithm>
#include <tuple>
#include <vector>

#define NOMINMAX	//windows.h -> define min,max
using namespace std;

typedef long long int64_t;
typedef unsigned long long uint64_t;
typedef long int32_t;
typedef unsigned long uint32_t;
typedef short int16_t;
typedef std::tuple< std::string,uint64_t,float,uint64_t,float > stat_row;

class Timer {
	std::string _label;
	int64_t _nano_stamp1;
	int64_t _nano_stamp2;
	int64_t _win_stamp1;
	int64_t _win_stamp2;
public:
	Timer(const char* label) : _label(label) ,_nano_stamp1(0),_nano_stamp2(0), _win_stamp1(0),_win_stamp2(0) {}
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
	
	void nano_start() {		_nano_stamp1 = __rdtsc();	}
	uint64_t nano_clock_diff() { 
		_nano_stamp2 = __rdtsc();
		return	_nano_stamp2- _nano_stamp1;	
	}

	static uint64_t win_clock() {
		LARGE_INTEGER time_stamp;
		QueryPerformanceCounter(&time_stamp);
		return time_stamp.QuadPart;
	}

	void win_start() {	_win_stamp1 = win_clock();	}

	uint64_t win_clock_diff() {	
		_win_stamp2 = win_clock();
		return _win_stamp2 - _win_stamp1;	
	}

	void start(){
		nano_start();
		win_start();
	}

	stat_row get_stat_row(){
	return std::make_tuple(_label ,nano_clock_diff(),0.0 , win_clock_diff() ,0.0);
	}

	friend ostream& operator << (ostream& s, Timer& t) {
		s << "Time stamp -> nano: " << t.nano_clock_diff() << "  win: " << t.win_clock_diff();
		return s;
	}
};

class Stat{
private:
	std::vector<stat_row> _stat; 

	stat_row min_row(){
		return (*std::min_element(_stat.begin(),_stat.end(),[](stat_row row1,stat_row row2){return std::get<1>(row1) < std::get<1>(row2);}));
	}
	
public:
	Stat(int init_size = 10){_stat.reserve(init_size);}
	void add(stat_row row){
		_stat.push_back(row);
	}
	std::vector<stat_row> operator()(){
		std::vector<stat_row> full_stat(_stat);
		//calc statistic data
		stat_row min_r = min_row();
		for(vector<stat_row>::iterator it = full_stat.begin(); it != full_stat.end();++it){
			//for nano clock
			std::get<2>((*it)) = (double)std::get<1>((*it)) / std::get<1>(min_r);
			//for win clock
			std::get<4>((*it)) = (double)std::get<3>((*it)) / std::get<3>(min_r);
		}
		return full_stat;
	}

	friend ostream& operator << (ostream& s, Stat& stat){
		vector<stat_row> full_stat = stat();
		s.fill('.');
		for(vector<stat_row>::iterator it = full_stat.begin(); it != full_stat.end();++it){

			s << setw(20) << left << std::get<0>((*it)) <<
			     setw(20) << right << " nano clock: " <<std::get<1>((*it)) << 
				 setw(20) << right << " ratio: " <<std::get<2>((*it)) << 
				 setw(20) << right << " win clock: " << std::get<3>((*it)) << 
				 setw(20) << right << " ratio: " << std::get<4>((*it)) << endl;
		}
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
	
	Mat auto_vectless_mul(Mat& mat,Stat* stat = nullptr) {
		if (width != mat.height)
			throw runtime_error("mutrix sizes are not suitable");
		Mat res(height,mat.width);
		Mat tr_mat = mat.transposit();
		T* res_row;
		T* tr_row;
		T* cur_row;
		Timer t("vectless mult");
		if(stat != nullptr){
			t.start();
		}
		for (int i = 0; i < height; i++){
			res_row = res.ptr[i];
			cur_row = ptr[i];
			for (int k = 0; k < tr_mat.height; k++) {
				tr_row = tr_mat.ptr[k];
				res_row[k] = 0;
				//#pragma loop(no_vector)
				#pragma novector
				for (int j = 0; j < width; j++)
					res_row[k] += cur_row[j] * tr_row[j];
				}
		}
			if(stat != nullptr){
				stat->add(t.get_stat_row());
			}
		return res;
	}

	Mat auto_vec_mul(Mat& mat,Stat* stat = nullptr){
		
		if (width != mat.height)
			throw runtime_error("mutrix sizes are not suitable");
		Mat res(height,mat.width);
		Mat tr_mat = mat.transposit();
		T* res_row;
		T* tr_row;
		T* cur_row;
		Timer t("auto mult");
		if(stat != NULL){
			t.start();
		}
		for (int i = 0; i < height; i++){
			res_row = res.ptr[i];
			cur_row = ptr[i];
			for (int k = 0; k < tr_mat.height; k++) {
				tr_row = tr_mat.ptr[k];
				res_row[k] = 0;
				for (int j = 0; j < width; j++)
					res_row[k] += cur_row[j] * tr_row[j];
				}
		}
		if(stat != nullptr){
				stat->add(t.get_stat_row());
		}
		return res;
	}

    Mat  manual_mul(Mat& mat,Stat* stat = nullptr) {
        if(typeid(T) != typeid(float))
			throw runtime_error("incorrect mat type");
		if (width != mat.height)
            throw runtime_error("mutrix sizes are not suitable");
        Mat res(height, mat.width);
        Mat tr_mat = mat.transposit();
		float* res_row;
		float* tr_row;
		float* cur_row;
		float val;
		//exec vec instructions
		bool fl_vec = true;
		//for asm
		size_t n_vec_loops = width / 4;
		size_t n_ord_loops = width % 4;
		//n_ord_loops++;
		Timer t("asm mult");
		if(stat != NULL){
			t.start();
		}
        for (int i = 0; i < height; i++){
			res_row = res.ptr[i];
			cur_row = ptr[i];
            for (int k = 0; k < tr_mat.height; k++) {
				tr_row = tr_mat.ptr[k];
				val = 0;
				__asm{
					mov eax,cur_row
					mov ebx,tr_row

					xorps xmm0,xmm0
					xorps xmm3,xmm3

					mov ecx,n_vec_loops
					test ecx,ecx
					jz __ord_loop
					
					__j_loop:
					//развёрнутая часть цикла
					dec ecx

					movaps xmm1,[eax]
					movaps xmm2,[ebx]

					mulps xmm1,xmm2
					addps xmm0,xmm1

					add eax,16
					add ebx,16

					test ecx,ecx
					jnz __j_loop

					haddps xmm0,xmm0
					haddps xmm0,xmm0

					__ord_loop:
					mov ecx,n_ord_loops
					test ecx,ecx
					jz __end

					__j1_loop:
					//обычная часть цикла
					dec ecx
					movss xmm1,[eax]
					movss xmm2,[ebx]

					mulss xmm1,xmm2
					addss xmm3,xmm1

					add eax,4
					add ebx,4

					test ecx,ecx
					jnz __j1_loop

					addss xmm0,xmm3
					movss val,xmm0
					__end:
				}
				res_row[k] = val;
            }
		}
		if(stat != nullptr){
			stat->add(t.get_stat_row());
		}
		return res;
	}
};


int main() {
	srand(time(NULL));
	Stat stat;
	Mat<float> m1(500, 500);
	m1.float_randomize();
	Mat<float> m2(500, 500);
	m2.float_randomize();	
	try {
		m1.auto_vec_mul(m2,&stat);
		m1.auto_vectless_mul(m2,&stat);
		m1.manual_mul(m2,&stat);
		cout<<stat;
	}
	catch (runtime_error e) {
		cout << e.what() << endl;
	}
	
	cout << endl;
	system("pause");
	return 0;
}