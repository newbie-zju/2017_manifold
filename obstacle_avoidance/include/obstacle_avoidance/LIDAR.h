#include <iostream>
#include "ros/ros.h"
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "nanoflann.hpp"

#define PI 3.14159265358979
/*
#define MINDISTANCE 150.0
#define MAXDISTANCE 5000.0
#define MAXCONNECTDISTANCE 250.0
#define OBSTACLEMAXSIZE 300.0
#define DOAVOIDANCEDIST 3250.0

#define SAFERADIUS 1250.0
#define AVOIDANCEVELOCITYMAG 0.6

//debug parameters
#define MINDISTANCE 150.0
#define MAXDISTANCE 5000.0
#define MAXCONNECTDISTANCE 0.0
#define OBSTACLEMAXSIZE 1000.0
#define DOAVOIDANCEDIST 3250.0

#define SAFERADIUS 1250.0
#define AVOIDANCEVELOCITYMAG 0.6
*/

//ֱ������ϵ�µĶ�ά���ƣ��������kdTree
struct PointCloud
{
	struct Point
	{
		float  x, y;
	};
	std::vector<Point>  pts;
	inline size_t kdtree_get_point_count() const { return pts.size(); }
	inline float kdtree_distance(const float *p1, const size_t idx_p2, size_t /*size*/) const
	{
		const float d0 = p1[0] - pts[idx_p2].x;
		const float d1 = p1[1] - pts[idx_p2].y;
		return sqrt(d0*d0 + d1*d1);
	}
	inline float kdtree_get_pt(const size_t idx, int dim) const
	{
		if (dim == 0) return pts[idx].x;
		else return pts[idx].y;
	}
	template <class BBOX>
	bool kdtree_get_bbox(BBOX&) const { return false; }
};


typedef float PCfloattype[720];
typedef int PCinttype[720];
typedef nanoflann::KDTreeSingleIndexAdaptor<
	nanoflann::L2_Simple_Adaptor<float, PointCloud >,
	PointCloud,
	2 /* ά�� */
> my_kd_tree_t;

class LIDAR
{
public:
	LIDAR() = default;
	~LIDAR()
	{
		delete[] theta;
		delete[] dist;
	}

	float *theta = new PCfloattype;
	float *dist = new PCfloattype;
	size_t count;
	bool doAvoidance=false;
	void grab_data(const std::vector<double> thetaAngle, const std::vector<double> distance, const int counts);
	std::vector<std::vector<int> > calcObstacles(void);//���ر���Ϊ���ϰ���ĵ���±꣬ÿ����������һ���ϰ��ÿ��������������ɸ��ϰ���ĵ���±�
	std::vector<std::pair<double,double> > obstaclesPos(void);//����ÿ���ϰ�������ĵ�theta��dist
	std::pair<double, double> velocityPlanner(std::pair<double,double> &curV, std::pair<double, double> &tarP, std::vector<std::pair<double, double> > &obstacles);//ͨ����������ϵ��ǰ�ٶ�curV<theta,mag>��Ŀ��λ��tarP<theta,dist>�������ǰ��Ҫ���ٵ��ٶ�tarV<theta,mag>
 	std::vector<std::pair<double, double> > checkdoAvoidance(std::vector<std::pair<double, double> > &obstacles);
private:
	PointCloud points;
	std::vector<int> RadiusSearch(float x, float y, float radius, my_kd_tree_t &Myindex);//������(x,y)����С��radius���±��vector
	void thetablockedMerge(std::vector<std::pair<double, double> > &thetablocked);
	float  _maxdistance, _mindistance, _maxconnectdistance, _obstaclemaxsize, _doavoidancedist, _saferadius, _avoidancevelocitymag;	
};