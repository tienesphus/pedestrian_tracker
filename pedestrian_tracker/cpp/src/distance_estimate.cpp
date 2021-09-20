#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include "core.hpp"


#include "distance_estimate.hpp"
#include "utils.hpp"

DistanceEstimate::DistanceEstimate(cv::Mat& video_frame):frame(video_frame){}
DistanceEstimate::DistanceEstimate(cv::Mat& video_frame, std::vector<cv::Point2f> config_points,float threshold):frame(video_frame){

    std::vector<cv::Point2f> src(4); // 4 points for perspective tranform
    std::vector<cv::Point2f> dst(4);
    std::vector<cv::Point2f> points_len; // 3 points for vertical and horizontal calculation
    int height,width;
    height = int(video_frame.size().height);
	width = int(video_frame.size().width);
    dst[0] = cv::Point2f(0.f, height);
    dst[1] = cv::Point2f(width, height);
    dst[2] = cv::Point2f(width, 0);
    dst[3] = cv::Point2f(0, 0);
    for (unsigned int i = 0; i < config_points.size(); i++) {
            if (i < 4) {
                src[i] = config_points[i];
            }
            else if (i < 7) {
                points_len.push_back(config_points[i]);
            }
    }
    perspective_tran = getPerspectiveTransform(src,dst);
    perspectiveTransform(points_len,warped_pt,perspective_tran);
    distance_w = CalculateDist(warped_pt[0], warped_pt[1]);
    distance_h = CalculateDist(warped_pt[0], warped_pt[2]);
    threshold_ = threshold;
}
DistanceEstimate& DistanceEstimate::operator=(const DistanceEstimate &other){
    if(this == &other){
        return *this;
    }
    perspective_tran = other.perspective_tran;
    warped_pt = other.warped_pt;
    distance_h = other.distance_h;
    distance_w = other.distance_w;
    threshold_ = other.threshold_;
    return *this;
}
std::vector<cv::Point2f> DistanceEstimate::GetTransformedPoints(TrackedObjects boxes){
    std::vector<cv::Point2f> bottom_points;
	for (unsigned int i = 0; i < boxes.size(); i++) {
		std::vector<cv::Point2f> temp_pnt(1); 
		std::vector<cv::Point2f> bd_pnt(1);
        temp_pnt[0] = GetBottomPoint(boxes[i].rect);
		// circle(frame, temp_pnt[0], 
        // 5, 
        // cv::Scalar(0, 0, 255),
        // 5);
		perspectiveTransform(temp_pnt, bd_pnt,perspective_tran);
		temp_pnt[0] = cv::Point2f(bd_pnt[0].x, bd_pnt[0].y);
		bottom_points.push_back(temp_pnt[0]);
		
	}
	
	return bottom_points;
}
cv::Point DistanceEstimate::GetMiddle(cv::Point2f point_1, cv::Point2f point_2) {
	cv::Point result;
	result.x = (point_1.x + point_2.x) / 2;
	result.y = (point_1.y + point_2.y) / 2;
	return result;
}
void DistanceEstimate::DrawDistance(TrackedObjects boxes){
    std::vector<std::pair<TrackedObjects, float>> distances_mat;
    std::vector<cv::Point2f> detected_pts;
    detected_pts = GetTransformedPoints(boxes);
    distances_mat = EstimateDistAllObj(detected_pts,boxes);
	if (distances_mat.size() > 1) {
		for (unsigned int i = 0; i < distances_mat.size(); i++) {
			cv::Rect pp1;
			cv::Rect pp2;
			float dist = 0;
			//float mark1_w, mark2_w;
			//float mark1_h, mark2_h;
			cv::Point pp1_center, pp2_center;
            pp1 = distances_mat[i].first[0].rect;
            pp2 = distances_mat[i].first[1].rect;
            pp1_center.x = (pp1.x + (pp1.x+pp1.width))/2;
            pp1_center.y = (pp1.y + (pp1.y+pp1.height))/2;
            pp2_center.x = (pp2.x + (pp2.x+pp2.width))/2;
            pp2_center.y = (pp2.y + (pp2.y+pp2.height))/2;
            dist = distances_mat[i].second;
            
            dist = ceilf(dist * 100)/100;
            std::string dist_str = std::to_string(dist);
            dist_str.erase(dist_str.find_last_not_of('0')+1,std::string::npos);
            cv::line(frame,
                pp1_center,
                pp2_center,
                cv::Scalar(0, 255, 0),
                2		
            );
            cv::putText(frame, //target image
                dist_str + "m",
                GetMiddle(pp1_center, pp2_center),
                cv::FONT_HERSHEY_DUPLEX,
                1.0,
                CV_RGB(118, 185, 0), //font color
                1);
            
		}	
	}
}
float DistanceEstimate::CalculateDist(cv::Point2f point_1, cv::Point2f point_2){
    float dist;
	float x = 0.0, y = 0.0;
	x = pow((point_1.x - point_2.x), 2);
	y = pow((point_1.y - point_2.y), 2);

	dist = sqrt(x + y);
	return dist;
}
float DistanceEstimate::EstimateRealDist(cv::Point2f point_1, cv::Point2f point_2, float distance_w, float distance_h){
    float x, y,dist_y,dist_x;
	float result;
	y = abs(point_1.y - point_2.y);
	x = abs(point_1.x - point_2.x);
    //getting the pixel ratio for 150cm = 1.5m
	dist_y = (float)(y / distance_h) * 150;
	dist_x = (float)(x / distance_w) * 150;

	result = (float)sqrtf((powf(dist_x, 2) + powf(dist_y, 2)));
    //convert cm to m
    result = result/100;
	return result;
}

std::vector<std::pair<TrackedObjects, float>> DistanceEstimate::EstimateDistAllObj(std::vector<cv::Point2f> detected_pts,TrackedObjects boxes){

    std::vector<std::pair<TrackedObjects, float>> bxs;
    for (unsigned int i = 0; i < detected_pts.size(); i++) {
        for (unsigned int j = 0; j < detected_pts.size(); j++) {
            if (i != j) {
                float dist = 0.0f;
                dist = EstimateRealDist(detected_pts[i], detected_pts[j], distance_w, distance_h);  
                if (dist <=threshold_){
                    bxs.push_back(make_pair((TrackedObjects){boxes[i],boxes[j]},dist));      
                }else{
                    continue;
                }
                
            }
        }
    }
    return bxs;
}