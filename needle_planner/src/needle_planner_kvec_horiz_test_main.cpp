/* 
 * File:   needle_planner_kvec_horiz_test_main.cpp
 * Author: wsn
 *
 * Created Feb 24, 2016
 */
#include <needle_planner/needle_planner.h>
//example use of needle-planner library

Eigen::Affine3d g_affine_lcamera_to_psm_one, g_affine_lcamera_to_psm_two; //, affine_gripper_wrt_base;
Eigen::Affine3d g_psm1_start_pose,g_psm2_start_pose;
Eigen::Affine3d transformTFToEigen(const tf::Transform &t) {
    Eigen::Affine3d e;
    for (int i = 0; i < 3; i++) {
        e.matrix()(i, 3) = t.getOrigin()[i];
        for (int j = 0; j < 3; j++) {
            e.matrix()(i, j) = t.getBasis()[i][j];
        }
    }
    // Fill in identity in last row
    for (int col = 0; col < 3; col++)
        e.matrix()(3, col) = 0;
    e.matrix()(3, 3) = 1;
    return e;
}

void init_poses() {
   ROS_INFO("getting transforms from camera to PSMs");
    tf::TransformListener tfListener;
    tf::StampedTransform tfResult_one, tfResult_two;
    
    bool tferr = true;
    int ntries = 0;
    ROS_INFO("waiting for tf between base and camera...");
    while (tferr) {
        if (ntries > 5) break; //give up and accept default after this many tries
        tferr = false;
        try {
            //The direction of the transform returned will be from the target_frame to the source_frame. 
            //Which if applied to data, will transform data in the source_frame into the target_frame. See tf/CoordinateFrameConventions#Transform_Direction
            tfListener.lookupTransform("left_camera_optical_frame", "one_psm_base_link", ros::Time(0), tfResult_one);
            tfListener.lookupTransform("left_camera_optical_frame", "two_psm_base_link", ros::Time(0), tfResult_two);
        } catch (tf::TransformException &exception) {
            ROS_WARN("%s", exception.what());
            tferr = true;
            ros::Duration(0.5).sleep(); // sleep for half a second
            ros::spinOnce();
            ntries++;
        }
    }
    //default transform: need to match this up to camera calibration!
    if (tferr) {
        g_affine_lcamera_to_psm_one.translation() << -0.155, -0.03265, 0.0;
        Eigen::Vector3d nvec, tvec, bvec;
        nvec << -1, 0, 0;
        tvec << 0, 1, 0;
        bvec << 0, 0, -1;
        Eigen::Matrix3d R;
        R.col(0) = nvec;
        R.col(1) = tvec;
        R.col(2) = bvec;
        g_affine_lcamera_to_psm_one.linear() = R;
        g_affine_lcamera_to_psm_two.linear() = R;
        g_affine_lcamera_to_psm_two.translation() << 0.145, -0.03265, 0.0;
        ROS_WARN("using default transform");
    } else {

        ROS_INFO("tf is good");

        //g_affine_lcamera_to_psm_one is the position/orientation of psm1 base frame w/rt left camera link frame
        // need to extend this to camera optical frame
        g_affine_lcamera_to_psm_one = transformTFToEigen(tfResult_one);
        g_affine_lcamera_to_psm_two = transformTFToEigen(tfResult_two);
    }
    ROS_INFO("transform from left camera to psm one:");
    cout << g_affine_lcamera_to_psm_one.linear() << endl;
    cout << g_affine_lcamera_to_psm_one.translation().transpose() << endl;
    ROS_INFO("transform from left camera to psm two:");
    cout << g_affine_lcamera_to_psm_two.linear() << endl;
    cout << g_affine_lcamera_to_psm_two.translation().transpose() << endl;  
    
    //now get initial poses:
    ROS_INFO("waiting for tf between base and grippers...");
    while (tferr) {
        if (ntries > 5) break; //give up and accept default after this many tries
        tferr = false;
        try {
            //The direction of the transform returned will be from the target_frame to the source_frame. 
            //Which if applied to data, will transform data in the source_frame into the target_frame. See tf/CoordinateFrameConventions#Transform_Direction
            tfListener.lookupTransform("left_camera_optical_frame", "one_tool_tip_link", ros::Time(0), tfResult_one);
            tfListener.lookupTransform("left_camera_optical_frame", "two_tool_tip_link", ros::Time(0), tfResult_two);
        } catch (tf::TransformException &exception) {
            ROS_WARN("%s", exception.what());
            tferr = true;
            ros::Duration(0.5).sleep(); // sleep for half a second
            ros::spinOnce();
            ntries++;
        }
    }
    //default start pose, if can't get tf:
    if (tferr) {
        g_psm1_start_pose.translation() << -0.02, 0, 0.04;
        g_psm2_start_pose.translation() << 0.02, 0, 0.04;
        Eigen::Vector3d nvec, tvec, bvec;
        nvec << -1, 0, 0;
        tvec << 0, 1, 0;
        bvec << 0, 0, -1;
        Eigen::Matrix3d R;
        R.col(0) = nvec;
        R.col(1) = tvec;
        R.col(2) = bvec;
        g_psm1_start_pose.linear() = R;
        g_psm2_start_pose.linear() = R;
        ROS_WARN("using default start poses");
    } else {

        ROS_INFO("tf is good");

        //g_affine_lcamera_to_psm_one is the position/orientation of psm1 base frame w/rt left camera link frame
        // need to extend this to camera optical frame
        g_psm1_start_pose = transformTFToEigen(tfResult_one);
        g_psm2_start_pose = transformTFToEigen(tfResult_two);
    }
    ROS_INFO("psm1 gripper start pose:");
    cout << g_psm1_start_pose.linear() << endl;
    cout << g_psm1_start_pose.translation().transpose() << endl;
    ROS_INFO("psm2 gripper start pose:");
    cout << g_psm2_start_pose.linear() << endl;
    cout << g_psm2_start_pose.translation().transpose() << endl;      
    
}

int main(int argc, char** argv) 
{
    // ROS set-ups:
    ros::init(argc, argv, "needle_planner_test_main"); //node name

    ros::NodeHandle nh; // create a node handle; need to pass this to the class constructor
    
    init_poses();    
     ROS_INFO("main: instantiating an object of type NeedlePlanner");
    NeedlePlanner needlePlanner;  
    needlePlanner.set_affine_lcamera_to_psm_one(g_affine_lcamera_to_psm_one);
    needlePlanner.set_affine_lcamera_to_psm_two(g_affine_lcamera_to_psm_two);     
    
    Eigen::Vector3d O_needle;
    O_needle<< 0.0,0.0,0.092; //we can hard-code O_needle(2) (z-value) for known tissue height
    double r_needle = 0.012; //0.0254/2.0;
    double kvec_yaw = 0.0;
    cout<<"enter kvec_yaw: (e.g. 0-2pi): ";
    cin>>kvec_yaw;
    vector <Eigen::Affine3d> gripper_affines_wrt_camera;  //put answers here  
    double needle_x,needle_y,needle_z;
    cout<<"enter needle center x coord (e.g. 0): ";
    cin>>needle_x;
    cout<<"enter needle center y coord (e.g. 0): ";
    cin>>needle_y;  
    cout<<"enter needle center z coord (e.g. 0.112): ";
    cin>>needle_z;      
    O_needle(0) = needle_x;
    O_needle(1) = needle_y;
    O_needle(2) = needle_z;
    
  
    //compute the tissue frame in camera coords, based on point-cloud selections:
    
    needlePlanner.simple_horiz_kvec_motion(O_needle, r_needle, kvec_yaw, gripper_affines_wrt_camera);
    int nposes = gripper_affines_wrt_camera.size();
    ROS_INFO("computed %d gripper poses w/rt camera",nposes);
    Eigen::Affine3d affine_pose;
    for (int i=0;i<nposes;i++) {
        ROS_INFO("pose %d",i);
        cout<<gripper_affines_wrt_camera[i].linear()<<endl;
        cout<<"origin: "<<gripper_affines_wrt_camera[i].translation().transpose()<<endl;
    }
    needlePlanner.write_needle_drive_affines_to_file(gripper_affines_wrt_camera);
    
    return 0;
} 
