/* 
    This file is part of GIFT.

    GIFT is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    GIFT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with GIFT.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "FeatureTracker.h"
#include "Configure.h"

#include "opencv2/highgui/highgui.hpp"

int main(int argc, char *argv[]) {

    GIFT::FeatureTracker ft(GIFT::readCameraConfig(cv::String(argv[1])));

    cv::VideoCapture cap;
    cap.open(cv::String(argv[2]));
    cv::Mat image;
    while (cap.read(image)) {;

        ft.processImage(image);
        std::vector<GIFT::Landmark> landmarks = ft.outputLandmarks();

        cv::Mat featureImage = ft.drawFeatureImage(Scalar(0,0,255), 5, 3);

        cv::imshow("debug", featureImage);
        int k = cv::waitKey(1);
        if (k == 's') cv::imwrite("FeatureImage.png", featureImage);
        if (k == 27) break;

    }

}