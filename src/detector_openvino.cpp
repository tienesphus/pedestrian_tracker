
#include "detector_openvino.hpp"

#include <iostream>
#include <iomanip>

#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>

#include <inference_engine.hpp>

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

    if (!is_dense)
        throw "Detector doesn't support conversion from not dense cv::Mat";

    InferenceEngine::TensorDesc tDesc(InferenceEngine::Precision::U8,
                                      {1, channels, height, width},
                                      InferenceEngine::Layout::NHWC);

    return InferenceEngine::make_shared_blob<uint8_t>(tDesc, mat.data);
}


DetectorOpenVino::DetectorOpenVino(const NetConfig &config):
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
    input->getInputData()->setLayout(Layout::NHWC);
    
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

DetectorOpenVino::~DetectorOpenVino() {}

Detections DetectorOpenVino::process(const cv::Mat &frame)
{
    std::cout << "Preprocess" << std::endl;
    InferRequest::Ptr request = this->network.CreateInferRequestPtr();
    request->SetBlob(inputName, wrapMat2Blob(frame));

    std::cout << "Starting inference request" << std::endl;
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
            results.emplace_back(r, con);
        }
    }

    return Detections(results);
}

