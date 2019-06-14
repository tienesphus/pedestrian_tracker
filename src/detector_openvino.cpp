
#include "detector_openvino.hpp"

#include <iostream>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/cvstd.hpp>

#include <opencv2/imgproc.hpp>
#include <inference_engine.hpp>
#include <iomanip>

using namespace InferenceEngine;

/**
 * @brief Wraps data stored inside of a passed cv::Mat object by new Blob pointer.
 * @note: No memory allocation is happened. The blob just points to already existing
 *        cv::Mat data.
 * @param mat - given cv::Mat object with an image data.
 * @return resulting Blob pointer.
 */
static InferenceEngine::Blob::Ptr wrapMat2Blob(const cv::Mat &mat) {
    size_t channels = mat.channels();
    size_t height = mat.size().height;
    size_t width = mat.size().width;

    size_t strideH = mat.step.buf[0];
    size_t strideW = mat.step.buf[1];

    bool is_dense =
            strideW == channels &&
            strideH == channels * width;

    if (!is_dense) THROW_IE_EXCEPTION
                << "Doesn't support conversion from not dense cv::Mat";

    InferenceEngine::TensorDesc tDesc(InferenceEngine::Precision::U8,
                                      {1, channels, height, width},
                                      InferenceEngine::Layout::NHWC);

    return InferenceEngine::make_shared_blob<uint8_t>(tDesc, mat.data);
}

void frameToBlob(const cv::Mat& frame,
                 InferRequest::Ptr& inferRequest,
                 const std::string& inputName)
{
    inferRequest->SetBlob(inputName, wrapMat2Blob(frame));
}

DetectorOpenVino::DetectorOpenVino(const NetConfig &config):
        Detector(config.thresh, config.clazz),
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
    input->getPreProcess().setResizeAlgorithm(ResizeAlgorithm::RESIZE_BILINEAR);
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
    ExecutableNetwork network = plugin.LoadNetwork(netReader.getNetwork(), {});
    this->network = network;

    std::cout << "End Loading detector" << std::endl;
}

cv::Mat DetectorOpenVino::run(const cv::Mat &frame)
{
    std::cout << "Starting inference request" << std::endl;
    InferRequest::Ptr request = this->network.CreateInferRequestPtr();
    frameToBlob(frame, request, this->inputName);
    request->StartAsync();

    std::cout << "Waitiong for inference request" << std::endl;
    request->Wait(IInferRequest::WaitMode::RESULT_READY);

    int width = frame.cols;
    int height = frame.rows;

    // Push the data into an Mat so it can be retrieved in post_process
    // TODO find a way to get rid of this copying of data.
    cv::Mat results(maxProposalCount, 7, CV_32FC1);

    const float *detections = request->GetBlob(outputName)->buffer().as<PrecisionTrait<Precision::FP32>::value_type*>();
    for (size_t i = 0; i < maxProposalCount; i++) {
        float id, cf, x1, x2, y1, y2;
        results.at<float>(i, 0) = id = detections[i * objectSize + 0];
        results.at<float>(i, 0) =      detections[i * objectSize + 1];
        results.at<float>(i, 0) = cf = detections[i * objectSize + 2];
        results.at<float>(i, 0) = x1 = detections[i * objectSize + 3] * width;
        results.at<float>(i, 0) = y1 = detections[i * objectSize + 4] * height;
        results.at<float>(i, 0) = x2 = detections[i * objectSize + 5] * width;
        results.at<float>(i, 0) = y2 = detections[i * objectSize + 6] * height;

        if (id < 0) {
            std::cout << "Only " << i << " proposals found" << std::endl;
            break;
        }

        std::cout << "[" << i << "," << id << "] element, prob = " << cf <<
                  "    (" << x1 << "," << y1 << ")-(" << x2 << "," << y2 << ")"
                  << ((cf > config.thresh) ? " WILL BE RENDERED!" : "") << std::endl;
    }

    return results;
}

Detections DetectorOpenVino::post_process(const cv::Mat &data) const {

    std::vector<Detection> results;

    for (size_t i = 0; i < maxProposalCount; i++) {
        float confidence = data.at<float>(i, 2);
        if (confidence > config.thresh) {

            float id = static_cast<int>(data.at<float>(i, 0));
            float x1 = data.at<float>(i, 3);
            float y1 = data.at<float>(i, 4);
            float x2 = data.at<float>(i, 5);
            float y2 = data.at<float>(i, 6);

            cv::Rect r(cv::Point(x1, y1), cv::Point(x2, y2));

            std::cout << "    Found: " << id << "(" << confidence << "%) - " << r << std::endl;

            if (id == config.clazz)
                results.emplace_back(r, 1);
        }
    }

    return Detections(results);
}

