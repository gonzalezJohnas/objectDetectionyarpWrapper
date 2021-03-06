// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

/*
  * Copyright (C)2017  Department of Robotics Brain and Cognitive Sciences - Istituto Italiano di Tecnologia
  * Author: jonas gonzalez
  * email: jonas.gonzalez@iit.it
  * Permission is granted to copy, distribute, and/or modify this program
  * under the terms of the GNU General Public License, version 2 or any
  * later version published by the Free Software Foundation.
  *
  * A copy of the license can be found at
  * http://www.robotcub.org/icub/license/gpl.txt
  *
  * This program is distributed in the hope that it will be useful, but
  * WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
  * Public License for more details
*/

/**
 * @file ObjectDetectionThread.cpp
 * @brief Implementation of the eventDriven thread (see ObjectDetectionThread.h).
 */

#include <utility>

#include "../include/iCub/ObjectDetectionThread.h"

using namespace yarp::dev;
using namespace yarp::os;
using namespace yarp::sig;
using namespace std;

#define THRATE 400 //ms

//********************interactionEngineRatethread******************************************************

ObjectDetectionThread::ObjectDetectionThread(yarp::os::ResourceFinder &rf) : RateThread(THRATE) {
    robot = rf.check("robot",
                     Value("icub"),
                     "Robot name (string)").asString();

    graphPath = rf.find("graph_path").asString().c_str();
    labelsPath = rf.find("labels_path").asString().c_str();
    modelName = rf.find("model_name").asString().c_str();


    tfObjectDetection = std::unique_ptr<tensorflowObjectDetection>(
            new tensorflowObjectDetection(graphPath, labelsPath, modelName));

    runRealTime = rf.check("realTime",
                           Value("false"),
                           "Run the module in realTime (boolean)").asBool();
}



ObjectDetectionThread::ObjectDetectionThread(yarp::os::ResourceFinder &rf, string _robot)
        : RateThread(THRATE) {
    robot = std::move(_robot);
   

    const string graphPath = rf.find("graph_path").asString().c_str();
    const string labelsPath = rf.find("labels_path").asString().c_str();
    const string modelName = rf.find("model_name").asString().c_str();

    tfObjectDetection = std::unique_ptr<tensorflowObjectDetection>(
            new tensorflowObjectDetection(graphPath, labelsPath, modelName));

    runRealTime = rf.check("realTime",
                           Value("false"),
                           "Run the module in realTime (boolean)").asBool();


}

ObjectDetectionThread::~ObjectDetectionThread() {
    // do nothing
}

bool ObjectDetectionThread::threadInit() {

    if (!inputImagePort.open(getName("/imageRGB:i").c_str())) {
        std::cout << ": unable to open port /imageRGB:i " << std::endl;
        return false;  // unable to open; let RFModule know so that it won't run
    }

    if (!outputImageBoxesPort.open(getName("/imageBoxes:o").c_str())) {
        std::cout << ": unable to open port /imageBoxes:o " << std::endl;
        return false;  // unable to open; let RFModule know so that it won't run
    }


    if (!outputLabelPort.open(getName("/label:o").c_str())) {
        std::cout << ": unable to open port /label:o " << std::endl;
        return false;  // unable to open; let RFModule know so that it won't run
    }

    tensorflow::Status initGraphStatus = tfObjectDetection->initGraph();

    if (initGraphStatus != tensorflow::Status::OK()) {
        yError("%s", initGraphStatus.ToString().c_str());
        return false;
    }

    outputBoxesImage = new ImageOf<PixelRgb>;


    yInfo("Initialization of the processing thread correctly ended");

    return true;
}

void ObjectDetectionThread::setName(string str) {
    this->name = std::move(str);
}


std::string ObjectDetectionThread::getName(const char *p) {
    string str(name);
    str.append(p);
    return str;
}



void ObjectDetectionThread::run() {
    

    if (runRealTime && this->isRunning()) {
        const string predictedClass = predictTopClass();

        this->writeToLabelPort(predictedClass);
        this->sendImageBoxesDetected();

        yInfo("Run graph success");
        

       
    }
    
}




void ObjectDetectionThread::threadRelease() {
    outputImageBoxesPort.interrupt();
    outputImageBoxesPort.close();

    inputImagePort.interrupt();
    inputImagePort.close();


}

std::string ObjectDetectionThread::predictTopClass() {

    yarp::sig::ImageOf<yarp::sig::PixelRgb> *inputImage = inputImagePort.read();

    if (inputImage != nullptr) {
        cv::Mat inputImageMat = cv::cvarrToMat(inputImage->getIplImage());
        cv::cvtColor(inputImageMat, inputImageMat, CV_BGR2RGB);

        return tfObjectDetection->inferObject(inputImageMat);

    }


    return std::__cxx11::string();
}

void ObjectDetectionThread::writeToLabelPort(string label) {
    Bottle &labelOutput = outputLabelPort.prepare();
    labelOutput.clear();

    labelOutput.addString(label);
    outputLabelPort.write();


}

void ObjectDetectionThread::setDetectionThreshold(const double t_thresholdInference) {
    this->tfObjectDetection->setM_detectionThreshold(t_thresholdInference);
}

double ObjectDetectionThread::getDetectionThreshold() {
    this->tfObjectDetection->getM_detecttionThreshold();
}

void ObjectDetectionThread::drawDetectedBoxes(IplImage *t_imageToDraw) {

    cv::Point originBox, endBox, displayTextPos;

    for (auto &it : tfObjectDetection->m_objectsDetected) {

        originBox = cvPoint(it.second.coordinate[0], it.second.coordinate[1]);
        endBox = cvPoint(it.second.coordinate[2], it.second.coordinate[3]);
        const Color objectColor = getObjectColor(it.second.className);

        cvRectangle(t_imageToDraw, originBox, endBox, cvScalar(objectColor.red, objectColor.green, objectColor.blue), 3);

        const std::string textToDisplay = it.first + " " + std::to_string(it.second.probabilityDetection);
        const cv::Size textSize = cv::getTextSize(textToDisplay, fontFace, fontScale, thickness, 0);                
        displayTextPos = cvPoint(it.second.coordinate[0] , it.second.coordinate[1] );



        cvRectangle(t_imageToDraw, cvPoint(it.second.coordinate[0] , it.second.coordinate[1] - (textSize.height * 2) ), cvPoint(displayTextPos.x + textSize.width, displayTextPos.y + textSize.height * 0.8), cvScalar(objectColor.red, objectColor.green, objectColor.blue), -1);
        cv::putText(cv::cvarrToMat(t_imageToDraw), textToDisplay, displayTextPos, fontFace, fontScale, cvScalar(255, 255, 255), thickness);
    }


}


bool ObjectDetectionThread::processing() {
    // here goes the processing...
    return true;
}

Color ObjectDetectionThread::getObjectColor(std::string t_objectLabel) {

    if(this->objectsColor.find(t_objectLabel) != this->objectsColor.end()){
       return this->objectsColor.at(t_objectLabel);
    }

    const Color randomColor =  getRandomColor();
    this->objectsColor.insert(std::pair<string, Color>(t_objectLabel,randomColor));

    return randomColor;
}

Color ObjectDetectionThread::getRandomColor() {
    const unsigned int redChannel = (0 + (rand() % static_cast<unsigned int>(255 - 0 + 1)));
    const unsigned int greenChannel =  (0 + (rand() % static_cast<unsigned int>(255 - 0 + 1)));
    const unsigned int blueChannel =  (0 + (rand() % static_cast<unsigned int>(255 - 0 + 1)));
    const Color randomColor = {redChannel, greenChannel, blueChannel};

    return randomColor;
}

void ObjectDetectionThread::sendImageBoxesDetected() {

    if (outputImageBoxesPort.getOutputCount() && inputImagePort.getInputCount()) {

        yarp::sig::ImageOf<yarp::sig::PixelRgb> *inputImage = inputImagePort.read();
        if (inputImage != nullptr) {


            auto *outputIplBoxes = (IplImage *) inputImage->getIplImage();
            drawDetectedBoxes(outputIplBoxes);

            inputImage = &outputImageBoxesPort.prepare();
            inputImage->resize(outputIplBoxes->width, outputIplBoxes->height);

            inputImage->wrapIplImage(outputIplBoxes);
            outputImageBoxesPort.write();
            
            this->tfObjectDetection->clearSetOfObject();


        }

    }
}



