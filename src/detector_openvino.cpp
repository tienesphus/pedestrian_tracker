
#include "detector_openvino.hpp"

#include <iostream>
#include <iomanip>

#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>

#include <inference_engine.hpp>

using namespace InferenceEngine;

/**
* @brief Sets image data stored in cv::Mat object to a given Blob object.
* @param orig_image - given cv::Mat object with an image data.
* @param blob - Blob object which to be filled by an image data.
* @param batchIndex - batch index of an image inside of the blob.
*/
template <typename T>
void matU8ToBlob(const cv::Mat& orig_image, InferenceEngine::Blob::Ptr& blob, const NetConfig &config, int batchIndex = 0) {

    InferenceEngine::SizeVector blobSize = blob->getTensorDesc().getDims();
    const size_t width = blobSize[3];
    const size_t height = blobSize[2];
    const size_t channels = blobSize[1];
    T* blob_data = blob->buffer().as<T*>();

    //cv::Mat blobbed = cv::dnn::blobFromImage(orig_image, config.scale, config.networkSize, config.mean, false, false, CV_8U);

    cv::Mat resized_image(orig_image);
    if (width != orig_image.size().width || height!= orig_image.size().height) {
        cv::resize(orig_image, resized_image, cv::Size(width, height));
    }

    int batchOffset = batchIndex * width * height * channels;

    for (size_t c = 0; c < channels; c++) {
        for (size_t  h = 0; h < height; h++) {
            for (size_t w = 0; w < width; w++) {
                blob_data[batchOffset + c * width * height + h * width + w] =
                        resized_image.at<cv::Vec3b>(h, w)[c];
            }
        }
    }
}

void frameToBlob(const cv::Mat& frame,
                 InferRequest::Ptr& inferRequest,
                 const std::string& inputName,
                 const NetConfig &config)
{
    Blob::Ptr frameBlob = inferRequest->GetBlob(inputName);
    matU8ToBlob<uint8_t>(frame, frameBlob, config);


}

DetectorOpenVino::DetectorOpenVino(const NetConfig &config):
        Detector(),
        config(config)
{

    std::cout << "Loading plugin" << std::endl;
    InferencePlugin plugin = PluginDispatcher({""}).getPluginByDevice("MYRIAD");

    std::cout << "Reading network" << std::endl;
    CNNNetReader netReader;
    netReader.ReadNetwork(config.meta);
    //netReader.getNetwork().setBatchSize(1);
    netReader.ReadWeights(config.model);

    CNNNetwork net = netReader.getNetwork();

    std::cout << "Configure Input layer" << std::endl;
    InputsDataMap inputInfo(net.getInputsInfo());
    if (inputInfo.size() != 1) {
        throw std::logic_error("Network must have exactly one input");
    }
    this->inputName = inputInfo.begin()->first;
    InputInfo::Ptr& input = inputInfo.begin()->second;
    input->setPrecision(Precision::U8);
    input->getInputData()->setLayout(Layout::NCHW);
    
    std::cout << "Configure Output Layer" << std::endl;
    OutputsDataMap outputInfo(net.getOutputsInfo());
    if (outputInfo.size() != 1) {
        throw std::logic_error("This demo accepts networks having only one output");
    }
    this->outputName = outputInfo.begin()->first;
    DataPtr& output = outputInfo.begin()->second;
    const SizeVector outputDims = output->getTensorDesc().getDims();
    if (outputDims.size() != 4) {
        throw std::logic_error("Incorrect output dimensions for SSD");
    }
    this->maxProposalCount = outputDims[2];
    this->objectSize = outputDims[3];
    if (objectSize != 7) {
        throw std::logic_error("Output should have 7 as a last dimension");
    }
    output->setPrecision(Precision::FP32);
    output->setLayout(Layout::NCHW);

    std::cout << "Loading Model to Plugin" << std::endl;
    this->network = plugin.LoadNetwork(netReader.getNetwork(), {});

    std::cout << "End Loading detector" << std::endl;
}

Detections DetectorOpenVino::process(const cv::Mat &frame)
{
    std::cout << "Starting inference request" << std::endl;
    InferRequest::Ptr request = this->network.CreateInferRequestPtr();
    frameToBlob(frame, request, this->inputName, this->config);
    request->StartAsync();

    std::cout << "Waiting for inference request" << std::endl;
    request->Wait(IInferRequest::WaitMode::RESULT_READY);

    int width = frame.cols;
    int height = frame.rows;

    std::cout << "Interpreting results" << std::endl;
    std::vector<Detection> results;
    const float *detections = request->GetBlob(outputName)->buffer().as<PrecisionTrait<Precision::FP32>::value_type*>();
    for (size_t i = 0; i < maxProposalCount; i++) {
        float id  = detections[i * objectSize + 0];
        float lbl = detections[i * objectSize + 1];
        float con = detections[i * objectSize + 2];
        float x1  = detections[i * objectSize + 3] * width;
        float y1  = detections[i * objectSize + 4] * height;
        float x2  = detections[i * objectSize + 5] * width;
        float y2  = detections[i * objectSize + 6] * height;

        if (id < 0) {
            // negative id signifies end of valid proposals
            break;
        }

        cv::Rect r(cv::Point(x1, y1), cv::Point(x2, y2));

        if (lbl == config.clazz && con > config.thresh) {
            std::cout << "    Found: " << id << " " << lbl << "(" << con*100 << "%) - " << r << std::endl;
            results.emplace_back(r, 1);
        }
    }

    return Detections(results);
}

