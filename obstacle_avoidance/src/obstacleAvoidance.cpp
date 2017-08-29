#include "obstacleAvoidance.h"

namespace iarcRplidar
{
ObstacleAvoidance::ObstacleAvoidance(ros::NodeHandle nh):nh_(nh),nh_param("~")
{
  /*  ros::param::get("~image_subscribeName", imageSubscribeName);
    ros::param::get("~Convexlength", Markerlength);
    ros::param::get("~Convexwidth", Markerwidth);*/
    
    initialize();
    ros::Rate loop_rate(30);
    while(ros::ok())
    {
	ros::spinOnce();
	loop_rate.sleep();
    }
}
    
ObstacleAvoidance::~ObstacleAvoidance()
{
    ROS_INFO("Destroying irobotDetect......");
}
    
void ObstacleAvoidance::initialize()
{
    laser_sub = nh_.subscribe("/scan",1,&ObstacleAvoidance::LaserScanCallback,this);
    velocity_position_sub = nh_.subscribe("/obstacle/velocityPosition",1,&ObstacleAvoidance::VelocityPositionCallback,this); 
    ned_drone_position_sub = nh_.subscribe("/dji_sdk/velocityPosition",10,&ObstacleAvoidance::droneLocalPositionCallback,this); 
    OA_tar_velocity_pub = nh_.advertise<std_msgs::Float32MultiArray>("/obstacleAvoidance/target_velocity", 1);
    //OA_vel_visualize_pub = nh_.advertise<geometry_msgs::PoseStamped>("/obstacleAvoidance/OA_vel_visualize", 1);
}

void ObstacleAvoidance::LaserScanCallback(const sensor_msgs::LaserScanConstPtr& msg)
{
    std::vector<double> angles;
    std::vector<double> ranges;
    std_msgs::Float32MultiArray velocity_msg;
    float obstacle_emergency_flag = 0;
    std::pair<double, double> tarV;
   // int laser_num = 0;
    int num_rays = msg->ranges.size();
    bool usescan = false;
    //cout << "num_rays:" << num_rays << endl; 
    
    if (num_rays > 0)
	usescan =true;
    
    angles.reserve(num_rays);
    ranges.reserve(num_rays);
    
    float ang = msg-> angle_min; //min angle in rplidar laser_detect_node
    /*********get the laser ranges and angles*********/
    for (int i = 0; i< num_rays; i++)
    {
	if (msg->ranges[i] < 50)
	{
	    ranges.push_back(msg->ranges[i]);
	}
	else
	    ranges.push_back(999999999999);
	//std::cout  << ranges[i] << std::endl;
	//ranges.push_back(msg->ranges[i]);
	angles.push_back(ang);
	ang += msg->angle_increment; //angle distance between measurements[rad]
	/*if(ranges[i] < 999 && ranges[i] > 0)
	    laser_num++;*/
	//std::cout  << laser_num << std::endl;
    }
    // std:: cout << "here" << std::endl;out 
    double t = msg->header.stamp.toSec();//(double)sec+1e-9*(double) nsec
    if(usescan)
    {
	rplidara2.grab_data(angles,ranges,num_rays);
	
	/*velocityPosition_msg.CurTheta = 0;
	velocityPosition_msg.CurMag = 1;
	velocityPosition_msg.TarTheta = 0;
	velocityPosition_msg.TarDist = 2000;*/
	/*std::cout << "-------------------------------------input-----------------------" << std::endl;
	std::cout <<"input_ned_vx    " << ned_velocity[0] <<"input_ned_vy     "  << ned_velocity[1] << std::endl;
	std::cout << "input_ned_tar_position_x    " << ned_tar_position[0] << "input_ned_tar_position_x  " << ned_tar_position[1] << std::endl;
	std::cout << "-------------------------------------end input-----------------------" << std::endl;*/
	
	Ned_to_Body(ned_velocity,  body_velocity, ned_tar_position, body_tar_position);
	std::pair<double, double> curV = std::make_pair(body_velocity[0], body_velocity[1]);
	std::pair<double, double> tarP = std::make_pair(body_tar_position[0], body_tar_position[1]);

	std::vector<std::pair<double, double> > obstacles = rplidara2.obstaclesPos();
	float temp = 99999.0;
	//std::cout  << (int)obstacles.size() << std::endl;
	for(int i = 0;i < (int)obstacles.size();i++)
	{
	    if(temp > obstacles[i].second) temp=obstacles[i].second;
	}
	//std::cout << obstacles[0].second <<std::endl;
	temp/=1000.0;
	/*for(int i = 0; i < (int)obstacles.size(); i++)
	{
	    std::cout << "-----obstacle body position----"  << std::endl;
	    std::cout << " Obstacles theta: " << obstacles[i].first << " Obstacle Dist " << obstacles[i].second <<std::endl;
	    std::cout << "-----obstacle body position end-----"  << std::endl;
	}*/
	rplidara2.checkdoAvoidance(obstacles);
	if(rplidara2.doAvoidance)
	{
	    obstacle_emergency_flag = 1;
	    tarV = rplidara2.velocityPlanner(curV, tarP, obstacles);
	  /*  std::cout << "--------output-----" << std::endl;
	    std:: cout << "velocity theta from body: " << tarV.first  << " velocity from body: " << tarV.second << std::endl;
            std:: cout << "position theta from body: " << tarP.first  << " position from body: " << tarP.second << std::endl;
	    std::cout << "------endoutput---" << std::endl;*/
	}
	else
	{
	    tarV = curV;
	}
	body_polar_velocity[0] = tarV.first;  //theta
	body_polar_velocity[1] = tarV.second; //man
	Body_to_Ned(body_polar_velocity, ned_velocity);
	
	
	//std:: cout << ned_velocity[0] << " y=" << ned_velocity[1] << std::endl;
        velocity_msg.data.push_back(ned_velocity[0]);
	velocity_msg.data.push_back(ned_velocity[1]);
	velocity_msg.data.push_back(obstacle_emergency_flag);
	velocity_msg.data.push_back(temp);
	    
	//std::cout  << temp << std::endl;
	//if(dronePosition[2] == ned_tar_position[3])
	//{
	  //  CDJIDrone->attitude_control((uint8_t)0x80, ned_velocity[0], ned_velocity[1], ned_tar_position[2], 0);
	//}
	/*
	float orientation;
	orientation = tarV.first*PI/180.0f + PI;
	velocity_msg.data.push_back(tarV.first);
	velocity_msg.data.push_back(tarV.second);
	// visualize orientation:
	Eigen::AngleAxisf rotation_vector(orientation, Eigen::Vector3f::UnitZ());
	Eigen::Quaternionf q(rotation_vector);
	geometry_msgs::PoseStamped avoidance_orientation_msg;
	avoidance_orientation_msg.header = msg->header;
	avoidance_orientation_msg.pose.orientation.w = q.w();
	avoidance_orientation_msg.pose.orientation.x = q.x();
	avoidance_orientation_msg.pose.orientation.y = q.y();
	avoidance_orientation_msg.pose.orientation.z = q.z();
	avoidance_orientation_msg.pose.position.x = 0.0;
	avoidance_orientation_msg.pose.position.y = 0.0;
	avoidance_orientation_msg.pose.position.z = 0.0;
	OA_vel_visualize_pub.publish(avoidance_orientation_msg);*/
	//velocity_msg[0] = tarV.first;   //degree clockwise positive,range:0-360
	//velocity_msg.data[1] = tarV.second;   //magnitude:m/s
	usescan = false;
	OA_tar_velocity_pub.publish(velocity_msg);
    }
    else
    {
	ROS_WARN("No laser scan data output...");
    }
}
void ObstacleAvoidance::VelocityPositionCallback(const std_msgs::Float32MultiArrayPtr& msg)
{
    quater[0] = msg->data[0];
    quater[1] = msg->data[1];
    quater[2] = msg->data[2];
    quater[3] = msg->data[3];
    ned_velocity[0] = msg->data[4];
    ned_velocity[1] = msg->data[5];
    ned_velocity[2] = msg->data[6];
    ned_tar_position[0] = msg->data[7];
    ned_tar_position[1] = msg->data[8];
    ned_tar_position[2] = msg->data[9];
    //std::cout << ned_tar_position[0] << " " << ned_tar_position[1] << " " << ned_tar_position[2] << std::endl;
}

void ObstacleAvoidance:: droneLocalPositionCallback(const dji_sdk::LocalPositionConstPtr& msg)
{
    dronePosition[0] = msg->x;
    dronePosition[1] = msg->y;
    dronePosition[2] = msg->z;
}


void ObstacleAvoidance::Ned_to_Body(const float ned_v[], float body_polar_v[], const float ned_p[], float body_polar_p[])
{
    float body_coor_v[3], body_coor_p[3];
    
    if(fabs(quater[0]*quater[0]+quater[1]*quater[1]+quater[2]*quater[2]+quater[3]*quater[3] - 1)  > 0.2)
    {
        quater[0] = Quater_last[0];
	quater[1] = Quater_last[1];
	quater[2] = Quater_last[2];
	quater[3] = Quater_last[3];
	//count_quater = count_quater+1;
	//ROS_INFO("count= %d", count_quater);
    }
    //ROS_INFO("Quater=%4.2f,%4.2f,%4.2f,%4.2f",Quater[0],Quater[1],Quater[2],Quater[3]);
    Quater_last[0] = quater[0];
    Quater_last[1] = quater[1];
    Quater_last[2] = quater[2];
    Quater_last[3] = quater[3];
    
    Eigen::VectorXd Body_velocity_vector(3),  Body_position_vector(3), Ned_velocity_vector(3),  Ned_position_vector(3);
    
    Ned_velocity_vector(0) = ned_v[0];
    Ned_velocity_vector(1) = ned_v[1];
    Ned_velocity_vector(2) = ned_v[2];
    
    Ned_position_vector(0) = ned_p[0];
    Ned_position_vector(1) = ned_p[1];
    Ned_position_vector(2) = ned_p[2];
    
    //std::cout <<"ned_x" << Ned_position_vector(0) << "ned_y" << Ned_position_vector(1) << "   " << Ned_position_vector(2) << std::endl;
    
    Eigen::MatrixXd Rotate(3, 3); 
    Rotate(0,0) = quater[0] * quater[0] + quater[1] * quater[1] - quater[2] * quater[2] - quater[3] * quater[3];
    Rotate(0,1) = 2 * ( quater[1] * quater[2] - quater[0] * quater[3]);
    Rotate(0,2) = 2 * ( quater[1] * quater[3] + quater[0] * quater[2]);
    Rotate(1,0) = 2 * ( quater[1] * quater[2] + quater[0] * quater[3]);
    Rotate(1,1) = quater[0] * quater[0] - quater[1] * quater[1] + quater[2] * quater[2] - quater[3] * quater[3];
    Rotate(1,2) = 2 * ( quater[2] * quater[3] - quater[0] * quater[1]);
    Rotate(2,0) = 2 * ( quater[1] * quater[3] - quater[0] * quater[2]);
    Rotate(2,1) = 2 * ( quater[2] * quater[3] + quater[0] * quater[1]);
    Rotate(2,2) = quater[0] * quater[0] - quater[1] * quater[1] - quater[2] * quater[2] + quater[3] * quater[3];
    
    Rotate.transposeInPlace();
    /*std::cout << "....................." << std::endl;
    std::cout <<" " << Rotate(0,0) << " " << Rotate(0,1) << " " << Rotate(0,2) << std::endl;
    std::cout <<" " << Rotate(1,0) << " " << Rotate(1,1) << " " << Rotate(1,2) << std::endl;
    std::cout <<" " << Rotate(2,0) << " " << Rotate(2,1) << " " << Rotate(2,2) << std::endl;*/
    
    Body_velocity_vector = Rotate * Ned_velocity_vector; // velocity ned to body 
    Body_position_vector = Rotate * Ned_position_vector; // velocity ned to body 
    //std::cout <<"body_x" << Body_position_vector(0) << "body_y" << Body_position_vector(1) << std::endl;
    body_coor_v[0] = Body_velocity_vector(0);
    body_coor_v[1] = Body_velocity_vector(1);
    body_coor_v[2] = Body_velocity_vector(2);
    
    body_coor_p[0] = Body_position_vector(0);
    body_coor_p[1] = Body_position_vector(1);
    body_coor_p[2] = Body_position_vector(2);
    
   // printf("tar_position_b:[%5.4f, %5.4f]",body_coor_p[0], body_coor_p[1]);
    if((body_coor_v[0] == 0) && (body_coor_v[1] = 0))
      body_polar_v[0] = 0;
    else
      body_polar_v[0] = atan2(body_coor_v[1], body_coor_v[0]) / PI *180.0;
    if(body_polar_v[0] < 0) body_polar_v[0] += 360.0;
    body_polar_v[1] = sqrt(body_coor_v[0] * body_coor_v[0] + body_coor_v[1] * body_coor_v[1]);
    
    if((body_coor_p[0] == 0) && (body_coor_p[1] = 0))
      body_polar_p[0] = 0;
    else
      body_polar_p[0] =  atan2(body_coor_p[1], body_coor_p[0]) / PI *180.0;
    if(body_polar_p[0] < 0) body_polar_p[0] += 360.0;
    body_polar_p[1] = 1000 * sqrt(body_coor_p[0] * body_coor_p[0] + body_coor_p[1] * body_coor_p[1]);
    
    //std::cout <<"polar_ned_x" << body_polar_p[0] << "polar_ned_y" << body_polar_p[1] << std::endl;
     
}

void ObstacleAvoidance::Body_to_Ned(const float body_polar_v[], float ned_v[])
{
    float body_v[3] = {0};
    if(fabs(quater[0]*quater[0]+quater[1]*quater[1]+quater[2]*quater[2]+quater[3]*quater[3] - 1)  > 0.2)
    {
        quater[0] = Quater_last[0];
	quater[1] = Quater_last[1];
	quater[2] = Quater_last[2];
	quater[3] = Quater_last[3];
	//count_quater = count_quater+1;
	//ROS_INFO("count= %d", count_quater);
    }
    //ROS_INFO("Quater=%4.2f,%4.2f,%4.2f,%4.2f",Quater[0],Quater[1],Quater[2],Quater[3]);
    Quater_last[0] = quater[0];
    Quater_last[1] = quater[1];
    Quater_last[2] = quater[2];
    Quater_last[3] = quater[3];
    
    body_v[0] = body_polar_v[1] * cos(body_polar_v[0]*PI/180.0);
    body_v[1] = body_polar_v[1] * sin(body_polar_v[0]*PI/180.0);
    //printf("result_velocity_b:[%5.4f, %5.4f]",body_v[0], body_v[1]);
    Eigen::VectorXd Ned_vector(3),  Result_vector(3), Body_vector(3);
    Body_vector(0) = body_v[0];
    Body_vector(1) = body_v[1];
    Body_vector(2) = 0;
    Eigen::MatrixXd Rotate(3, 3); 
    Rotate(0,0) = quater[0] * quater[0] + quater[1] * quater[1] - quater[2] * quater[2] - quater[3] * quater[3];
    Rotate(0,1) = 2 * ( quater[1] * quater[2] - quater[0] * quater[3]);
    Rotate(0,2) = 2 * ( quater[1] * quater[3] + quater[0] * quater[2]);
    Rotate(1,0) = 2 * ( quater[1] * quater[2] + quater[0] * quater[3]);
    Rotate(1,1) = quater[0] * quater[0] - quater[1] * quater[1] + quater[2] * quater[2] - quater[3] * quater[3];
    Rotate(1,2) = 2 * ( quater[2] * quater[3] - quater[0] * quater[1]);
    Rotate(2,0) = 2 * ( quater[1] * quater[3] - quater[0] * quater[2]);
    Rotate(2,1) = 2 * ( quater[2] * quater[3] + quater[0] * quater[1]);
    Rotate(2,2) = quater[0] * quater[0] - quater[1] * quater[1] - quater[2] * quater[2] + quater[3] * quater[3];
    //Rotate = Rotate.inverse();
    Ned_vector = Rotate * Body_vector; //转换成NED坐标系下机体相对于小车的x y坐标的增量
  
   /* ned_v[0] = Global_vector(0);  //x ned velocity
    ned_v[1] = Global_vector(1);   //y ned velocity
    ned_v[2] = Global_vector(2);*/
    ned_v[0] = Ned_vector[0];
    ned_v[1] = Ned_vector[1];
    ned_v[2] = 0;
    //结果向量包含了机体相对于小车的x y坐标的增量和大地坐标系下的theta角
}
}
