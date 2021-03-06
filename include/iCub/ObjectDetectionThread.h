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
 * @file ObjectDetectionThread.h
 * @brief Definition of a thread that receives an data from input port and sends it to the output port.
 */


#ifndef _ObjectDetectionThread_RATETHREAD_H_
#define _ObjectDetectionThread_RATETHREAD_H_


#include <yarp/sig/all.h>
#include <yarp/os/all.h>
#include <yarp/dev/all.h>
#include <yarp/os/RateThread.h>
#include <yarp/os/Log.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <time.h>

#include "tensorflowObjectDetection.h"


struct Color{
    unsigned int red;
    unsigned int green;
    unsigned int blue;
};

class ObjectDetectionThread : public yarp::os::RateThread {
private:
    bool runRealTime;                    //result of the processing

    std::string robot;              // name of the robot
    std::string name;               // rootname of all the ports opened by this thread
    std::string graphPath;         // path to the graph to be loaded
    std::string labelsPath;        // path to the associated graph labels
    std::string modelName;        // path to the associated graph labels



    std::unique_ptr<tensorflowObjectDetection> tfObjectDetection;

    yarp::sig::ImageOf<yarp::sig::PixelRgb>* outputBoxesImage;

    yarp::os::BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelRgb> > inputImagePort;
    yarp::os::BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelRgb> > outputImageBoxesPort;
    yarp::os::BufferedPort<yarp::os::Bottle> outputLabelPort;

    std::map<std::string, Color> objectsColor;


    
    const int fontFace = CV_FONT_HERSHEY_TRIPLEX;
    const double fontScale = 0.8;
    const int thickness = 1.5;

public:
    /**
    * constructor default
    */
    explicit ObjectDetectionThread(yarp::os::ResourceFinder &rf);


    /**
    * constructor 
    * @param robotname name of the robot
    */
    ObjectDetectionThread(yarp::os::ResourceFinder &rf, std::string robotname);


    /**
     * destructor
     */
    ~ObjectDetectionThread() override;


    /**
    *  initialises the thread
    */
    bool threadInit() override;


    /**
    *  correctly releases the thread
    */
    void threadRelease() override;


    /**
    *  active part of the thread
    */
    void run() override;



    /**
    * function that sets the rootname of all the ports that are going to be created by the thread
    * @param str rootnma
    */
    void setName(std::string str);

    /**
    * function that returns the original root name and appends another string iff passed as parameter
    * @param p pointer to the string that has to be added
    * @return rootname 
    */
    std::string getName(const char *p);


    /**
     * Function to write into the Bottle outputLabelPort
     * @param label
     */
    void writeToLabelPort(std::string label);

    std::string predictTopClass();


    /**
     * Set detection threshold for ObjectDetection DeepNetwork
     * @param t_thresholdInference
     */
    void setDetectionThreshold(double t_thresholdInference);


    /**
    * Get the detection threshold for ObjectDetection DeepNetwork
    */
    double getDetectionThreshold();

    void drawDetectedBoxes(IplImage* t_imageToDraw);

    /**
     * Send to the ouputBoxPort the image with the detected boxes
     */
    void sendImageBoxesDetected();



    /**

     * method for the processing in the ratethread
     **/
    bool processing();

    Color getObjectColor(std::string t_objectLabel);

    Color getRandomColor();


};

#endif  //_ObjectDetectionThread_THREAD_H_

//----- end-of-file --- ( next line intentionally left blank ) ------------------

