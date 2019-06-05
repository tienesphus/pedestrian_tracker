
#include <iostream>

#include <opencv2/core/mat.hpp>
#include <opencv2/core/cvstd.hpp>
#include <opencv2/imgproc.hpp>

#include "detection.hpp"
#include <inference_engine.hpp>

void frameToBlob(const cv::Mat& frame,
                 InferRequest::Ptr& inferRequest,
                 const std::string& inputName) 
{
    Blob::Ptr frameBlob = inferRequest->GetBlob(inputName);
    matU8ToBlob<uint8_t>(frame, frameBlob);
}

DetectorOpenVino::DetectorOpenVino(const NetConfigOpenVino &config):
        config(config)
{
    
    using namespace InferenceEngine;
    
    std::cout << "Reading network" << std::endl;
    CNNNetReader netReader;
    netReader.readNetwork(config.meta);
    //netReader.getNetwork().setBatchSize(1);
    netReader.ReadWeights(config.model);
    
    CNNNetwork net = netReader.getNetwork();
    
    std::cout << "Configure Input layer" << std::endl;
    InputsDataMap inputInfo(net.getInputsInfo());
    this->inputName = inputInfo.begin()->first;
    InputInfo::Ptr& input = inputInfo.begin()->second;
    input->setPrecision(Precision::U8);
    input->getInputData()->setLayout(Layout::NCHW);
    
    std::cout << "Configure Output Layer" << std::endl;
    OutputsDataMap outputInfo(net.getOutputsInfo());
    DataPtr& output = outputInfo.begin()->second;
    output->setPrecision(Precision::FP32);
    output->setLayout(Layout::NCHW);
    
    std::cout << "Loading plugin" << std::endl;
    InferencePlugin plugin = InferenceEngine::PluginDispatcher({"../../../lib/intel64", ""}).getPluginByDevice(FLAGS_d);
    
    std::cout << "Loading Model to Plugin" << std::endl;
    ExecutableNetwork network = plugin.LoadNetwork(net, {});
    this->network = network;
    
    std::cout << "Loading blobs"
}

InferenceEngine::InferRequest::Ptr DetectorOpenVino::pre_process(const cv::Mat &image) 
{
    std::cout << "Starting inference request" << endl;
    InferRequest::Ptr request = this->network.CreateInferRequestPtr();
    frameToBlob(image, request, this->inputName);
    request->StartAsync();
    return request;
}

cv::Ptr<cv::Mat> DetectorOpenVino::process(InferenceEngine::InferRequest::Ptr &request)
{
    std::cout << "Waitiong for inference request" << endl;
    requets->Wait(IInferRequest::WaitMode::RESULT_READY);
    
    Blob::Ptr result = request->GetBlob(outputName);
    SizeVector size = result.dims();
    const float *raw_data = result->buffer().as<PrecisionTrait<Precision::FP32>::value_type*>();
    
    // convert the raw data into a Mat
    std::vector<int> sizes;
    for (size_t dimSize : size)
        sizes.push_back(dimSize);    
    
    return cv::make_ptr<Mat>(sizes, CV_32F, raw_data);
}

cv::Ptr<Detections> DetectorOpenVino::post_process(const cv::Mat &original, const cv::Mat &data) const
{   
    // result is of size [nimages, nchannels, a, b]
    // nimages = 1 (as only one image at a time)
    // nchannels = 1 (for RGB/BW?, not sure why we only have one, or why there would be a channel dimension)
    // a is number of predictions (depends on archutecture)
    // b is 7 floats of data. The order is:
    //  0 - ? (always 0?)
    //  1 - class index
    //  2 - confidennce
    //  3 - x1 (co-ords are scaled between 0 - 1)
    //  4 - y1
    //  5 - x2
    //  6 - y2
    
    // Therefore, we discard first two dimensions to only have the data
    cv::Mat detections(data.size[2], data.size[3], CV_32F, (void*)data.ptr<float>());

    std::vector<Detection> results;
    int w = original.cols;
    int h = original.rows;
    
    for (int i = 0; i < detections.size[0]; i++) {
        float confidence = detections.at<float>(i, 2);
        if (confidence > this->config.thresh) {
            int id = int(detections.at<float>(i, 1));
            int label = int(detections.at<float>(i, 2));
            int x1 = int(detections.at<float>(i, 3) * w);
            int y1 = int(detections.at<float>(i, 4) * h);
            int x2 = int(detections.at<float>(i, 5) * w);
            int y2 = int(detections.at<float>(i, 6) * h);
            cv::Rect2d r(cv::Point2d(x1, y1), cv::Point2d(x2, y2));
            
            std::cout << "    Found: " << id << "(" << confidence << "%) - " << r << std::endl;
            if (id == this->config.clazz)
                results.push_back(Detection(r, confidence));
        }
    }
    
    return cv::Ptr<Detections>(cv::makePtr<Detections>(results));
}
