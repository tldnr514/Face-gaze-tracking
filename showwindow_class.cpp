#include <opencv2/opencv.hpp>
#include <opencv2/face.hpp>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <sstream>
#include <string>

const int INIT_FRAME = 60;
const int WIN_WIDTH = 1000, WIN_HEIGHT = 718;
std::vector<cv::Point2d> clothesFrom = {cv::Point2d(75, 150), cv::Point2d(275, 150), cv::Point2d(512, 150), cv::Point2d(712, 150)};
std::vector<cv::Point2d> clothesTo = {cv::Point2d(250, 637), cv::Point2d(412, 637), cv::Point2d(662, 637), cv::Point2d(856, 637)};
std::vector<std::vector<std::string>> clothesPrice = {{"Shirt:2000YEN", "Pants:4000YEN", "Slim fit for you"}, {"Shirt:2000YEN", "Sweater: 4000YEN", "Pants:4000YEN", "Make you look slimmer"}, {"Shirt:2000YEN", "Jacket:6000YEN", "Pants: 4000YEN", "Style for all seasons"}, {"Shirt:2000YEN", "Pants:4000YEN", "Make your legs look longer"}};

cv::Mat windowImage;

const std::string FACE_DETECTION_MODEL = "resource/haarcascade_frontalface_alt2.xml";
const std::string FACEMARK_MODEL = "resource/lbfmodel2.yaml";

class FaceAngle
{
  public:
    double headX, headY;
    bool initialized;

    static FaceAngle &create()
    {
        if (instance_ == nullptr)
        {
            instance_ = new FaceAngle();
        }
        return *instance_;
    }

    FaceAngle()
    {
        faceDetector.load(FACE_DETECTION_MODEL);
        facemark = cv::face::FacemarkLBF::create();
        facemark->loadModel(FACEMARK_MODEL);
        hRatio = 1.0;
        vRatio = 1.0;
        hAngle = 0.0;
        vAngle = 0.0;
        initialized = true;
    }

    cv::Point2f getFacePoint(cv::Mat &frame)
    {
        getFaceVector(frame);

        double leftLen = sqrt(leftVec.dot(leftVec)), rightLen = sqrt(rightVec.dot(rightVec));
        double hAngle_temp = 0.0, vAngle_temp = 0.0;
        if (leftLen > rightLen)
        {
            rightVec.z = sqrt(leftLen * leftLen - rightLen * rightLen);
            cv::Point3f vertVec = leftVec.cross(rightVec);
            hAngle_temp = acos(vertVec.dot(cv::Point3f(0, 0, 1.0)) / sqrt(vertVec.dot(vertVec))) * 180 / M_PI;
        }
        else
        {
            leftVec.z = sqrt(rightLen * rightLen - leftLen * leftLen);
            cv::Point3f vertVec = leftVec.cross(rightVec);
            hAngle_temp = -acos(vertVec.dot(cv::Point3f(0, 0, 1.0)) / sqrt(vertVec.dot(vertVec))) * 180 / M_PI;
        }
        if(!hAngle_temp != hAngle_temp)
            hAngle = (hAngle * 11 + hAngle_temp) / 12;

        double upLen = sqrt(upVec.dot(upVec)), downLen = sqrt(downVec.dot(downVec));
        if (upLen > downLen * vRatio)
        {
            downVec.z = sqrt(pow((upLen / vRatio), 2) - pow(downLen, 2));
            vAngle_temp = -asin(downVec.dot(cv::Point3f(0, 0, 1.0)) / sqrt(downVec.dot(downVec))) * 180 / M_PI;
        }
        else
        {
            upVec.z = sqrt(pow(downLen * vRatio, 2) - pow(upLen, 2));
            vAngle_temp = asin(upVec.dot(cv::Point3f(0, 0, 1.0)) / sqrt(upVec.dot(upVec))) * 180 / M_PI;
        }
        vAngle_temp /= 2;
        if(!vAngle_temp != vAngle_temp)
            vAngle = (vAngle * 11 + vAngle_temp) / 12;

        if(hAngle != hAngle)
            hAngle = 0.0;
        if(vAngle != vAngle)
            vAngle = 0.0;

        distance = 1000.0 * 80 / sqrt(faceVec.dot(faceVec));

        

        headX = distance * tan(hAngle / 180 * M_PI) + WIN_WIDTH / 2;
        headY = -distance * tan(vAngle / 180 * M_PI) + WIN_HEIGHT / 2;

        return cv::Point2d(headX, headY);
    }

    static void atExit()
    {
        if (instance_ != nullptr)
        {
            delete instance_;
        }
    }

  private:
    static FaceAngle *instance_;
    double hAngle, vAngle, distance;
    double hRatio, vRatio;
    cv::Point3f leftVec, rightVec, upVec, downVec, mouthVec, faceVec;
    cv::CascadeClassifier faceDetector;
    cv::Ptr<cv::face::Facemark> facemark;

    void getFaceVector(cv::Mat &frame)
    {
        cv::Mat gray;
        std::vector<cv::Rect> faces;
        cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

        faceDetector.detectMultiScale(gray, faces, 1.1, 3, CV_HAAR_SCALE_IMAGE, cv::Size(40, 40));
        std::vector<std::vector<cv::Point2f>> landmarks;
        bool success = facemark->fit(frame, faces, landmarks);

        if (success)
        {
            double max=0.0;
            int max_i = 0;
            for(int i=0; i<landmarks.size(); i++){
                cv::Point2f faceVec = landmarks[i][16] - landmarks[i][0];
                if(max < faceVec.dot(faceVec)){
                    max = faceVec.dot(faceVec);
                    max_i = i;
                }
            }
            cv::Point2f leftEye, rightEye, nose, topNose, topMouth, bottomMouth, mouth;
            leftEye = (landmarks[max_i][36] + landmarks[max_i][39]) / 2;
            rightEye = (landmarks[max_i][42] + landmarks[max_i][45]) / 2;
            nose = landmarks[max_i][30];
            topNose = (leftEye + rightEye) / 2;
            topMouth = (landmarks[max_i][61] + landmarks[max_i][62] + landmarks[max_i][63]) / 3;
            bottomMouth = (landmarks[max_i][65] + landmarks[max_i][66] + landmarks[max_i][67]) / 3;
            mouth = (topMouth + bottomMouth) / 2;

            leftVec = cv::Point3f(leftEye - nose);
            rightVec = cv::Point3f(rightEye - nose);
            upVec = cv::Point3f(topNose - nose);
            downVec = cv::Point3f(topMouth - nose);
            mouthVec = cv::Point3f(topMouth - bottomMouth);
            faceVec = cv::Point3f(landmarks[max_i][16] - landmarks[max_i][0]);

            if (initialized)
            {
                cv::circle(frame, leftEye, 5, cv::Scalar(255, 0, 0), -1);
                cv::circle(frame, rightEye, 5, cv::Scalar(0, 255, 0), -1);
                cv::circle(frame, nose, 5, cv::Scalar(0, 0, 255), -1);
                cv::circle(frame, topNose, 5, cv::Scalar(0, 255, 255), -1);
                cv::circle(frame, topMouth, 5, cv::Scalar(255, 255, 0), -1);

                cv::line(frame, leftEye, nose, cv::Scalar(255, 255, 255), 3);
                cv::line(frame, rightEye, nose, cv::Scalar(255, 255, 255), 3);
                cv::line(frame, topNose, nose, cv::Scalar(255, 255, 255), 3);
                cv::line(frame, topMouth, nose, cv::Scalar(255, 255, 255), 3);

                std::ostringstream buf;
                buf << '(' << hAngle << ", " << vAngle << ')';
                std::ostringstream buf2;
                buf2 << '(' << headX << ", " << headY << ") ";

                cv::putText(frame, buf.str(), cv::Point2d(10, 50), 2, 1.0, cv::Scalar(255, 0, 0));
                cv::putText(frame, buf2.str(), cv::Point2d(10, 100), 2, 1.0, cv::Scalar(255, 0, 0));
            }
        }
    }
};
FaceAngle *FaceAngle::instance_ = nullptr;

int checkClothes(int x, int y)
{
    for (int i = 0; i < clothesFrom.size(); i++)
    {
        if (clothesFrom[i].x < x && x < clothesTo[i].x)
        {
            if (clothesFrom[i].y < y && y < clothesTo[i].y)
            {
                return i;
            }
        }
    }
    return -1;
}

int main(int argc, char *argv[])
{
    windowImage = cv::imread("resource/display.jpg");
    cv::resize(windowImage, windowImage, cv::Size(WIN_WIDTH, WIN_HEIGHT), 0, 0, cv::INTER_LANCZOS4);

    cv::VideoCapture cam;
    if (argc > 1)
    {
        std::string in_video_file(argv[1]);
        cam.open(in_video_file);
    }
    else
    {
        cam.open(0);
    }

    cv::Mat frame, gray;
    cv::namedWindow("win1", 1);
    cv::namedWindow("win2", 1);

    bool loop_flag = true;

    FaceAngle faceAngle = FaceAngle::create();
    while (loop_flag && cam.read(frame))
    {
        cv::flip(frame, frame, 1);
        cv::Mat win2 = windowImage.clone();

        cv::Point2d facePoint = faceAngle.getFacePoint(frame);

        cv::circle(win2, cv::Point2f(facePoint.x, facePoint.y), 10, cv::Scalar(0, 0, 255), -1);
        int clothesSelected = checkClothes(facePoint.x, facePoint.y);
        if (clothesSelected != -1)
        {
            int y0 = clothesFrom[clothesSelected].y + 300;
            for (int i = 0; i < clothesPrice[clothesSelected].size(); i++)
            {
                cv::putText(win2, clothesPrice[clothesSelected][i], cv::Point2d(clothesFrom[clothesSelected].x, y0 + 30 * i), 2, 0.6, cv::Scalar(255, 255, 255));
            }
        }

        cv::imshow("win2", win2);
        cv::imshow("win1", frame);

        int c = cv::waitKey(33);

        switch (c)
        {
        case 27: //ESC
        case 'q':
        case 'Q':
            loop_flag = false;
            break;
        }
    }

    FaceAngle::atExit();
    return 0;
}