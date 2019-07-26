#include "iostream"
#include "string"
#include "vector"

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "FeatureTracker.h"
#include "Configure.h"

int main(int argc, char *argv[]) {
    cv::String folderName = "/home/pieter/datasets/";

    // Set up a monocular feature tracker
    GIFT::FeatureTracker ft = GIFT::FeatureTracker(GIFT::TrackerMode::MONO);
    GIFT::CameraParameters cam0 = GIFT::readCameraConfig(folderName+"euroc.yaml");
    ft.setCameraConfiguration(cam0, 0);
    ft.maxFeatures = 50;

    cv::VideoCapture cap(folderName+"euroc.mp4");
    cv::namedWindow("debug");

    cv::Mat image;

    while (cap.read(image)) {

        // Track the features
        ft.processImage(image);
        std::vector<GIFT::Landmark> landmarks = ft.outputLandmarks();

        // Draw the keypoints
        std::vector<cv::Point2f> features;
        for (const auto &lm : landmarks) {
            features.emplace_back(lm.camCoordinates[0]);
        }
        std::vector<cv::KeyPoint> keypoints;
        cv::KeyPoint::convert(features, keypoints);
        cv::Mat kpImage;
        cv::drawKeypoints(image, keypoints, kpImage, cv::Scalar(0,0,255));
        cv::imshow("points", kpImage);

        // Draw the optical flow
        cv::Mat flowImage;
        image.copyTo(flowImage);
        for (const auto &lm : landmarks) {
            Point2f p1 = lm.camCoordinates[0];
            Point2f p0 = p1 - Point2f(lm.opticalFlowRaw[0].x(), lm.opticalFlowRaw[0].y());
            cv::line(flowImage, p0, p1, cv::Scalar(255,0,255));
            cv::circle(flowImage, p0, 2, cv::Scalar(255,0,0));
        }
        cv::imshow("flow", flowImage);


        cv::waitKey(1);
    }

    std::cout << "Complete." << std::endl;

}