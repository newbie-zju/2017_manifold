#ifndef __DPTOUCHING_H__
#define __DPTOUCHING_H__
#include <ros/ros.h>
#include <vector>
#include <opencv2/opencv.hpp>
#include <geometry_msgs/Point32.h>
#include <tf/transform_listener.h>
#include <irobot_detect//Pose3D.h>
#include <dji_sdk/LocalPosition.h>
#include "iarc_mission/TG.h"
#include <iarc_tf/Velocity.h>
#include <eigen3/Eigen/Dense>
#include <sensor_msgs/LaserScan.h>

#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/foreach.hpp> 
#include <boost/regex.hpp>
#include <boost/thread/mutex.hpp>
//����
/*
Dp_pos quadrotorPos = Dp_pos(1.5,1.5,2),//������λ��
	irobotPos = Dp_pos(2,3,0);//С��λ��
float irobotDir = 0;//С������
//���
Dp_pos outputPos;//���λ��
*/
//DPstate quadrotorState = APPROACH;//������״̬			//ƫ����#8.28

//#define tarV 1.0			//Ѳ���ٶȴ�С#8.28


using namespace cv;
using namespace std;

enum TGState_{TG_CRUISE,TG_TRACK,TG_TRACK_APP,TG_WANDER,TG_APP_HEAD,TG_APP_TOP,TG_TEST};

//λ����
struct Dp_pos
{
	float x,y,z;
	Dp_pos()
	{
		x=0.0f;
		y=0.0f;
		z=0.0f;
	}
	Dp_pos(float x0,float y0,float z0)
	{
		x=x0;
		y=y0;
		z=z0;
	}
};


class DpTouching
{
public:
	#define  DP_N 50//����
	float dT;//discrete system sample time
	vector<Mat> q;//�ɻ�λ�����ٶ�
	Mat q_ugv;//Ŀ��
	float p_x[DP_N];
	float p_y[DP_N];
	float p_z[DP_N];
	//DPstate state;//�ɻ�״̬
	//float cruise_height,track_height,approach_height;//Ѳ�������١��ӽ��ĸ߶�����
	dji_sdk::LocalPosition quadrotorPosNED;
	dji_sdk::LocalPosition quadrotorPos;
	//goal_detected::Pose3D irobotPos;
	geometry_msgs::Point32 outputPos;
	double xMax;
	double yMax;
	double tarV;
	float tarX;
	float tarY;
	float tarZ;
	float wanderCenterX;
	float wanderCenterY;
	float tarVx;
	float tarVy;
	float tarX_ob;
	float tarY_ob;
	float guidancerang;
	float obstcleNEDx;
	float obstcleNEDy;
	bool guidance_emergency;
	//bool hokuyo_emergency;
	
	float wanderR;
	float dxy;
	float cruise_dist;
	enum VelState{NED,GROUND};
	ros::NodeHandle nh;
	ros::NodeHandle nh_param;
	//Ĭ�Ϲ��캯��
	DpTouching(ros::NodeHandle nh_);
	ros::Subscriber quadrotorPosNED_sub;
	ros::Subscriber quadrotorPosGround_sub;
	//ros::Subscriber hokuyoBody_sub;
	ros::Subscriber guidance_distance_sub;
	ros::ServiceServer TG_server;
	ros::ServiceClient tf_client;
	tf::TransformListener listener;
	//����滮��ʼλ��
	//��������
	void runMethod(void);
	void initialize();
	bool calculateTrajectoryCallback(iarc_mission::TG::Request &req, iarc_mission::TG::Response &res);
	void quadrotorPosNEDCallback(const dji_sdk::LocalPosition::ConstPtr &msg);
	void quadrotorPosGroundCallback(const geometry_msgs::PointStamped::ConstPtr &msg);
	void guidanceObstacleCallback(const sensor_msgs::LaserScanConstPtr& msg);
	bool insideRec(float tx,float ty,float x1,float y1,float x2,float y2);
	
	// for obstacle avoidance
private:
	Eigen::Vector2f attractiveVec;
	Eigen::Vector2f repulsiveVec;	//repulsiveVec(0) = range; repulsiveVec(1) = ang;
	Eigen::Vector2f attractiveForce;
	Eigen::Vector2f repulsiveForce;
	Eigen::Vector2f joinForce;		//joinForce = attractiveForce + repulsiveForce;
public:
	int number_obstacle;   
	double Kr;
	double avoidanceV;
	double fattractive;
	float obstacle_ranges[5] = {0.0,0.0,0.0,0.0,0.0};
	float obstacle_angles[5] = {0.0,0.0,0.0,0.0,0.0};
	void setAttVec(Eigen::Vector2f attVec);	//repVec(0) = range; repVec(1) = ang;
	void setRepVec(Eigen::Vector2f repVec);
	void getForce(Eigen::Vector2f &attForce, Eigen::Vector2f &repForce);
	void calcAttractiveForce();
	void calcRepulsiceForce();
	//void hokuyo_dataCallback(const obstacle_avoidance::Hokuyo::ConstPtr &msg);
	void doAvoidance(Eigen::Vector2f attVec);
	float getLength2f(float x, float y);
};



#endif
