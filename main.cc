/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <iostream>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>

#include "helloworld.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using helloworld::HelloRequest;
using helloworld::HelloReply;
using helloworld::Greeter;

#include<opencv2/core/core.hpp>
#include<opencv2/highgui/highgui.hpp>
#include<opencv2/imgproc/imgproc.hpp>

#include<iostream>
// #include<curses.h>           // it may be necessary to change or remove this line if not using Windows

#include "Blob.h"

// #define SHOW_STEPS            // un-comment or comment this line to show steps or not



// global variables ///////////////////////////////////////////////////////////////////////////////
const cv::Scalar SCALAR_BLACK = cv::Scalar(0.0, 0.0, 0.0);
const cv::Scalar SCALAR_WHITE = cv::Scalar(255.0, 255.0, 255.0);
const cv::Scalar SCALAR_YELLOW = cv::Scalar(0.0, 255.0, 255.0);
const cv::Scalar SCALAR_GREEN = cv::Scalar(0.0, 200.0, 0.0);
const cv::Scalar SCALAR_RED = cv::Scalar(0.0, 0.0, 255.0);

// function prototypes ////////////////////////////////////////////////////////////////////////////
void matchCurrentFrameBlobsToExistingBlobs(std::vector<Blob> &existingBlobs, std::vector<Blob> &currentFrameBlobs);
void addBlobToExistingBlobs(Blob &currentFrameBlob, std::vector<Blob> &existingBlobs, int &intIndex);
void addNewBlob(Blob &currentFrameBlob, std::vector<Blob> &existingBlobs);
double distanceBetweenPoints(cv::Point point1, cv::Point point2);
void drawAndShowContours(cv::Size imageSize, std::vector<std::vector<cv::Point> > contours, std::string strImageName);
void drawAndShowContours(cv::Size imageSize, std::vector<Blob> blobs, std::string strImageName);
bool checkIfBlobsCrossedTheLine(std::vector<Blob> &blobs, int &intHorizontalLinePosition, int &carCount);
void drawBlobInfoOnImage(std::vector<Blob> &blobs, cv::Mat &imgFrame2Copy);
void drawCarCountOnImage(int &carCount, cv::Mat &imgFrame2Copy);
void RunServer();

// Logic and data behind the server's behavior.
class GreeterServiceImpl final : public Greeter::Service {
    Status SayHello(ServerContext* context,
                    const HelloRequest* request,
                    ServerWriter<HelloReply>* writer) override {
        // std::string prefix("Hello ");
        // // reply->set_message(prefix + request->name());
        // HelloReply r;
        // r.set_message(prefix + request->name());

        // for (int i = 0; i < 10; i++){
        //     writer->Write(r);
        // };
        cv::VideoCapture capVideo;
        
        cv::Mat imgFrame1;
        cv::Mat imgFrame2;
    
        std::vector<Blob> blobs;
    
        cv::Point crossingLine[2];
    
        int carCount = 0;
        
        capVideo.open("CarsDrivingUnderBridge.mp4");
    
        if (!capVideo.isOpened()) {                                                 // if unable to open video file
            std::cout << "error reading video file" << std::endl << std::endl;      // show error message
            // getch();                   // it may be necessary to change or remove this line if not using Windows
            // return(0);                                                              // and exit program
        }
    
        if (capVideo.get(CV_CAP_PROP_FRAME_COUNT) < 2) {
            std::cout << "error: video file must have at least two frames";
            // getch();                   // it may be necessary to change or remove this line if not using Windows
            // return(0);
        }
    
        capVideo.read(imgFrame1);
        capVideo.read(imgFrame2);
    
        int intHorizontalLinePosition = (int)std::round((double)imgFrame1.rows * 0.35);
    
        crossingLine[0].x = 0;
        crossingLine[0].y = intHorizontalLinePosition;
    
        crossingLine[1].x = imgFrame1.cols - 1;
        crossingLine[1].y = intHorizontalLinePosition;
    
        char chCheckForEscKey = 0;
    
        bool blnFirstFrame = true;
    
        int frameCount = 2;
    
        while (capVideo.isOpened() && chCheckForEscKey != 27) {
            
            std::vector<Blob> currentFrameBlobs;
    
            cv::Mat imgFrame1Copy = imgFrame1.clone();
            cv::Mat imgFrame2Copy = imgFrame2.clone();
    
            cv::Mat imgDifference;
            cv::Mat imgThresh;
    
            cv::cvtColor(imgFrame1Copy, imgFrame1Copy, CV_BGR2GRAY);
            cv::cvtColor(imgFrame2Copy, imgFrame2Copy, CV_BGR2GRAY);
    
            cv::GaussianBlur(imgFrame1Copy, imgFrame1Copy, cv::Size(5, 5), 0);
            cv::GaussianBlur(imgFrame2Copy, imgFrame2Copy, cv::Size(5, 5), 0);
    
            cv::absdiff(imgFrame1Copy, imgFrame2Copy, imgDifference);
    
            cv::threshold(imgDifference, imgThresh, 30, 255.0, CV_THRESH_BINARY);
    
            // cv::imshow("imgThresh", imgThresh);
    
            cv::Mat structuringElement3x3 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
            cv::Mat structuringElement5x5 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
            cv::Mat structuringElement7x7 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(7, 7));
            cv::Mat structuringElement15x15 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(15, 15));
    
            for (unsigned int i = 0; i < 2; i++) {
                cv::dilate(imgThresh, imgThresh, structuringElement5x5);
                cv::dilate(imgThresh, imgThresh, structuringElement5x5);
                cv::erode(imgThresh, imgThresh, structuringElement5x5);
            }
    
            cv::Mat imgThreshCopy = imgThresh.clone();
    
            std::vector<std::vector<cv::Point> > contours;
    
            cv::findContours(imgThreshCopy, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
            drawAndShowContours(imgThresh.size(), contours, "imgContours");
    
            std::vector<std::vector<cv::Point> > convexHulls(contours.size());
    
            for (unsigned int i = 0; i < contours.size(); i++) {
                cv::convexHull(contours[i], convexHulls[i]);
            }
    
            drawAndShowContours(imgThresh.size(), convexHulls, "imgConvexHulls");
    
            for (auto &convexHull : convexHulls) {
                Blob possibleBlob(convexHull);
    
                if (possibleBlob.currentBoundingRect.area() > 400 &&
                    possibleBlob.dblCurrentAspectRatio > 0.2 &&
                    possibleBlob.dblCurrentAspectRatio < 4.0 &&
                    possibleBlob.currentBoundingRect.width > 30 &&
                    possibleBlob.currentBoundingRect.height > 30 &&
                    possibleBlob.dblCurrentDiagonalSize > 60.0 &&
                    (cv::contourArea(possibleBlob.currentContour) / (double)possibleBlob.currentBoundingRect.area()) > 0.50) {
                    currentFrameBlobs.push_back(possibleBlob);
                }
            }
    
            drawAndShowContours(imgThresh.size(), currentFrameBlobs, "imgCurrentFrameBlobs");
    
            if (blnFirstFrame == true) {
                for (auto &currentFrameBlob : currentFrameBlobs) {
                    blobs.push_back(currentFrameBlob);
                }
            } else {
                matchCurrentFrameBlobsToExistingBlobs(blobs, currentFrameBlobs);
            }
    
            drawAndShowContours(imgThresh.size(), blobs, "imgBlobs");
    
            imgFrame2Copy = imgFrame2.clone();          // get another copy of frame 2 since we changed the previous frame 2 copy in the processing above
    
            drawBlobInfoOnImage(blobs, imgFrame2Copy);
    
            bool blnAtLeastOneBlobCrossedTheLine = checkIfBlobsCrossedTheLine(blobs, intHorizontalLinePosition, carCount);
    
            if (blnAtLeastOneBlobCrossedTheLine == true) {
                cv::line(imgFrame2Copy, crossingLine[0], crossingLine[1], SCALAR_GREEN, 2);
            } else {
                cv::line(imgFrame2Copy, crossingLine[0], crossingLine[1], SCALAR_RED, 2);
            }
    
            drawCarCountOnImage(carCount, imgFrame2Copy);
            std::string carCountString = std::to_string(carCount);
            std::cout << "Count : " << carCountString << std::endl;
            HelloReply r;
            r.set_message(carCountString);
            writer->Write(r);

            // std::cout << "car count : " << carCount << std::endl;
    
            // cv::imshow("imgFrame2Copy", imgFrame2Copy); // THIS IS MAIN SHOW
    
            //cv::waitKey(0);                 // uncomment this line to go frame by frame for debugging
    
            // now we prepare for the next iteration
    
            currentFrameBlobs.clear();
    
            imgFrame1 = imgFrame2.clone();           // move frame 1 up to where frame 2 is
    
            if ((capVideo.get(CV_CAP_PROP_POS_FRAMES) + 1) < capVideo.get(CV_CAP_PROP_FRAME_COUNT)) {
                capVideo.read(imgFrame2);
            } else {
                std::cout << "end of video\n";
                break;
            }
    
            blnFirstFrame = false;
            frameCount++;
            chCheckForEscKey = cv::waitKey(1);
        }
    
        if (chCheckForEscKey != 27) {               // if the user did not press esc (i.e. we reached the end of the video)
            cv::waitKey(0);                         // hold the windows open to allow the "end of video" message to show
        }
        
        return Status::OK;
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
int main(void) {
    // note that if the user did press esc, we don't need to hold the windows open, we can simply let the program end which will close the windows
    RunServer();
    return(0);
}

/////////////////////////////////////////////////////////////////////////////////////////////////// 
void RunServer() {
    std::string server_address("0.0.0.0:50051");
    GreeterServiceImpl service;
  
    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service);
    // Finally assemble the server.
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
  
    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
}
///////////////////////////////////////////////////////////////////////////////////////////////////
void matchCurrentFrameBlobsToExistingBlobs(std::vector<Blob> &existingBlobs, std::vector<Blob> &currentFrameBlobs) {

    for (auto &existingBlob : existingBlobs) {

        existingBlob.blnCurrentMatchFoundOrNewBlob = false;

        existingBlob.predictNextPosition();
    }

    for (auto &currentFrameBlob : currentFrameBlobs) {

        int intIndexOfLeastDistance = 0;
        double dblLeastDistance = 100000.0;

        for (unsigned int i = 0; i < existingBlobs.size(); i++) {

            if (existingBlobs[i].blnStillBeingTracked == true) {

                double dblDistance = distanceBetweenPoints(currentFrameBlob.centerPositions.back(), existingBlobs[i].predictedNextPosition);

                if (dblDistance < dblLeastDistance) {
                    dblLeastDistance = dblDistance;
                    intIndexOfLeastDistance = i;
                }
            }
        }

        if (dblLeastDistance < currentFrameBlob.dblCurrentDiagonalSize * 0.5) {
            addBlobToExistingBlobs(currentFrameBlob, existingBlobs, intIndexOfLeastDistance);
        }
        else {
            addNewBlob(currentFrameBlob, existingBlobs);
        }

    }

    for (auto &existingBlob : existingBlobs) {

        if (existingBlob.blnCurrentMatchFoundOrNewBlob == false) {
            existingBlob.intNumOfConsecutiveFramesWithoutAMatch++;
        }

        if (existingBlob.intNumOfConsecutiveFramesWithoutAMatch >= 5) {
            existingBlob.blnStillBeingTracked = false;
        }

    }

}

///////////////////////////////////////////////////////////////////////////////////////////////////
void addBlobToExistingBlobs(Blob &currentFrameBlob, std::vector<Blob> &existingBlobs, int &intIndex) {

    existingBlobs[intIndex].currentContour = currentFrameBlob.currentContour;
    existingBlobs[intIndex].currentBoundingRect = currentFrameBlob.currentBoundingRect;

    existingBlobs[intIndex].centerPositions.push_back(currentFrameBlob.centerPositions.back());

    existingBlobs[intIndex].dblCurrentDiagonalSize = currentFrameBlob.dblCurrentDiagonalSize;
    existingBlobs[intIndex].dblCurrentAspectRatio = currentFrameBlob.dblCurrentAspectRatio;

    existingBlobs[intIndex].blnStillBeingTracked = true;
    existingBlobs[intIndex].blnCurrentMatchFoundOrNewBlob = true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void addNewBlob(Blob &currentFrameBlob, std::vector<Blob> &existingBlobs) {

    currentFrameBlob.blnCurrentMatchFoundOrNewBlob = true;

    existingBlobs.push_back(currentFrameBlob);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
double distanceBetweenPoints(cv::Point point1, cv::Point point2) {

    int intX = abs(point1.x - point2.x);
    int intY = abs(point1.y - point2.y);

    return(sqrt(pow(intX, 2) + pow(intY, 2)));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void drawAndShowContours(cv::Size imageSize, std::vector<std::vector<cv::Point> > contours, std::string strImageName) {
    cv::Mat image(imageSize, CV_8UC3, SCALAR_BLACK);

    cv::drawContours(image, contours, -1, SCALAR_WHITE, -1);

    // cv::imshow(strImageName, image);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void drawAndShowContours(cv::Size imageSize, std::vector<Blob> blobs, std::string strImageName) {

    cv::Mat image(imageSize, CV_8UC3, SCALAR_BLACK);

    std::vector<std::vector<cv::Point> > contours;

    for (auto &blob : blobs) {
        if (blob.blnStillBeingTracked == true) {
            contours.push_back(blob.currentContour);
        }
    }

    cv::drawContours(image, contours, -1, SCALAR_WHITE, -1);

    // cv::imshow(strImageName, image);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool checkIfBlobsCrossedTheLine(std::vector<Blob> &blobs, int &intHorizontalLinePosition, int &carCount) {
    bool blnAtLeastOneBlobCrossedTheLine = false;

    for (auto blob : blobs) {

        if (blob.blnStillBeingTracked == true && blob.centerPositions.size() >= 2) {
            int prevFrameIndex = (int)blob.centerPositions.size() - 2;
            int currFrameIndex = (int)blob.centerPositions.size() - 1;

            if (blob.centerPositions[prevFrameIndex].y > intHorizontalLinePosition && blob.centerPositions[currFrameIndex].y <= intHorizontalLinePosition) {
                carCount++;
                blnAtLeastOneBlobCrossedTheLine = true;
            }
        }

    }

    return blnAtLeastOneBlobCrossedTheLine;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void drawBlobInfoOnImage(std::vector<Blob> &blobs, cv::Mat &imgFrame2Copy) {

    for (unsigned int i = 0; i < blobs.size(); i++) {

        if (blobs[i].blnStillBeingTracked == true) {
            cv::rectangle(imgFrame2Copy, blobs[i].currentBoundingRect, SCALAR_RED, 2);

            int intFontFace = CV_FONT_HERSHEY_SIMPLEX;
            double dblFontScale = blobs[i].dblCurrentDiagonalSize / 60.0;
            int intFontThickness = (int)std::round(dblFontScale * 1.0);

            cv::putText(imgFrame2Copy, std::to_string(i), blobs[i].centerPositions.back(), intFontFace, dblFontScale, SCALAR_GREEN, intFontThickness);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void drawCarCountOnImage(int &carCount, cv::Mat &imgFrame2Copy) {

    int intFontFace = CV_FONT_HERSHEY_SIMPLEX;
    double dblFontScale = (imgFrame2Copy.rows * imgFrame2Copy.cols) / 300000.0;
    int intFontThickness = (int)std::round(dblFontScale * 1.5);

    cv::Size textSize = cv::getTextSize(std::to_string(carCount), intFontFace, dblFontScale, intFontThickness, 0);

    cv::Point ptTextBottomLeftPosition;

    ptTextBottomLeftPosition.x = imgFrame2Copy.cols - 1 - (int)((double)textSize.width * 1.25);
    ptTextBottomLeftPosition.y = (int)((double)textSize.height * 1.25);

    cv::putText(imgFrame2Copy, std::to_string(carCount), ptTextBottomLeftPosition, intFontFace, dblFontScale, SCALAR_GREEN, intFontThickness);

}
