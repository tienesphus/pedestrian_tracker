#pragma once

#include "core.hpp"



class DistanceEstimate{
    public:
        cv::Mat &frame; ///< reference to the video frame for drawing distance estimation.
        
        /// \brief default constructor
        DistanceEstimate(cv::Mat& video_frame);

        /// \brief constructor that take in arguments necessary for distance estimation
        /// \param[in] video_frame reference frame for drawing lines
        /// \param[in] config_points list of point from -config file 
        /// \param[in] dst_points list of reference points from the video frame.
        /// \param[in] threshold the threshold for showing calculated distance between pedestrians
        explicit DistanceEstimate(cv::Mat& video_frame,std::vector<cv::Point2f> config_points,float threshold);

        
        /// \brief draw the line between all detected objects below a THRESHOLD and show the distance between them
        /// \param[in] boxes a list of  detected objects.
        void DrawDistance(const TrackedObjects &boxes);

        /// \brief overloading a assignment operator 
        /// \param[in] other a distanceestimate object.
        DistanceEstimate& operator=(const DistanceEstimate &other);
    private:
        
        cv::Mat perspective_tran;           ///< the perspective transformation.
        std::vector<cv::Point2f> warped_pt; ///< result vector of the perspective transformation
        float distance_w;                   ///< the distance width after perspective transformation from selected points. 
        float distance_h;                   ///< distance height after perspective transformation from selected points.
        float threshold_;

        /// \brief calculate distance between two points given their coordinates (euclidean distance)
        /// \param[in] point_1 coordinate of the first point
        /// \param[in] point_2 coordinate of the second point
        /// \return the distance between those points
        float CalculateDist(const cv::Point2f &point_1, const cv::Point2f &point_2);

        /// \brief calculate the middle coordinates given two points
        /// \param[in] point_1 coordinate of the first point
        /// \param[in] point_2 coordinate of the second point
        /// \return the middle point
        cv::Point GetMiddle(const cv::Point2f &point_1, const cv::Point2f &point_2);

        /// \brief scaling point to a top down view perspective
        /// \param[in] boxes a list of tracked predestrians
        /// \return the bottom middle points of the detected boxes. points are scaled to top down perspective
        std::vector<cv::Point2f> GetTransformedPoints(const TrackedObjects &boxes);
	
        /// \brief estimate real distance in meter between two points
        /// \param[in] point_1 coordinate of the first point
        /// \param[in] point_2 coordinate of the second point
        /// \param[in] distance_w scaled vertical distance from perspective transform
        /// \param[in] distance_h scaled horizontal distance from perspective transform
        /// \return actual distance between two points
        float EstimateRealDist(const cv::Point2f &point_1, const cv::Point2f &point_2, const float &distance_w, const float &distance_h) const;
        
        /// \brief estimate distance between all detected objects
        /// \param[in] detected_obj a list of bottom middle points of all detected boxes. points are scaled to top down perspective
        /// \param[in] boxes a list of detected boxes
        /// \return a list of pairs of detected objects along with the distance below a THRESHOLD
        std::vector<std::pair<TrackedObjects, float>> EstimateDistAllObj(const std::vector<cv::Point2f> &detected_pts,const TrackedObjects &boxes);


};