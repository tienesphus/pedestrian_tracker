#include "feature_affinity.hpp"

FeatureData::FeatureData(std::vector<float> features): features(std::move(features))
{}

FeatureAffinity::FeatureAffinity(const NetConfig& netConfig, InferenceEngine::InferencePlugin &plugin):
        netConfig(netConfig)
{
    using namespace InferenceEngine;

    std::cout << "[ INFO ] Loading network files for PersonREID" << std::endl;
    CNNNetReader netReader;
    /** Read network model **/
    netReader.ReadNetwork(netConfig.meta);
    /** Set batch size to 1 **/
    std::cout << "[ INFO ] Batch size is forced to  1" << std::endl;
    netReader.getNetwork().setBatchSize(1);
    /** Extract model name and load it's weights **/
    netReader.ReadWeights(netConfig.model);
    // -----------------------------------------------------------------------------------------------------

    /** SSD-based network should have one input and one output **/
    // ---------------------------Check inputs ------------------------------------------------------
    std::cout << "[ INFO ] Checking Person Reidentification inputs" << std::endl;
    InputsDataMap inputInfo(netReader.getNetwork().getInputsInfo());
    if (inputInfo.size() != 1) {
        throw std::logic_error("Person Reidentification network should have only one input");
    }
    inputName = inputInfo.begin()->first;
    InputInfo::Ptr& inputInfoFirst = inputInfo.begin()->second;
    inputInfoFirst->setInputPrecision(Precision::U8);

    inputInfoFirst->getPreProcess().setResizeAlgorithm(ResizeAlgorithm::RESIZE_BILINEAR);
    inputInfoFirst->getInputData()->setLayout(Layout::NHWC);

    // -----------------------------------------------------------------------------------------------------

    // ---------------------------Check outputs ------------------------------------------------------
    std::cout << "[ INFO ] Checking Person Reidentification outputs" << std::endl;
    OutputsDataMap outputInfo(netReader.getNetwork().getOutputsInfo());
    if (outputInfo.size() != 1) {
        throw std::logic_error("Person Reidentification network should have only one output");
    }
    outputName = outputInfo.begin()->first;

    this->network = plugin.LoadNetwork(netReader.getNetwork(), {});
}


FeatureAffinity::~FeatureAffinity() = default;

std::unique_ptr<FeatureData> FeatureAffinity::init(const Detection& d, const cv::Mat& frame) const
{
    cv::Mat crop = frame(intersection(d.box, cv::Rect(0, 0, frame.cols, frame.rows)));
    return std::make_unique<FeatureData>(identify(crop));
}

float FeatureAffinity::affinity(const FeatureData &detectionData, const FeatureData &trackData) const
{
    return cosine_similarity(detectionData.features, trackData.features);
}

void FeatureAffinity::merge(const FeatureData& detectionData, FeatureData& trackData) const
{
    trackData.features = detectionData.features;
}

void FeatureAffinity::draw(const FeatureData&, cv::Mat&) const
{
}

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
        throw std::logic_error("Detector doesn't support conversion from not dense cv::Mat");

    InferenceEngine::TensorDesc tDesc(InferenceEngine::Precision::U8,
                                      {1, channels, height, width},
                                      InferenceEngine::Layout::NHWC);

    return InferenceEngine::make_shared_blob<uint8_t>(tDesc, mat.data);
}


std::vector<float> FeatureAffinity::identify(const cv::Mat &person) const {
    using namespace InferenceEngine;

    std::cout << "Identify started" << std::endl;

    InferRequest request = const_cast<FeatureAffinity*>(this)->network.CreateInferRequest();
    cv::Mat person_scaled;
    // TODO if the person input image is too big, the reid network starts segfaulting?
    // Thus, I manually scale here. Only occurs on Pi with images taking nearly entire frame.
    cv::resize(person, person_scaled, cv::Size(48, 96));
    cv::Mat person_clone = person_scaled.clone(); // clone is needed so the Mat is dense
    Blob::Ptr inputBlob = wrapMat2Blob(person_clone);
    request.SetBlob(inputName, inputBlob);
    request.StartAsync();
    request.Wait(IInferRequest::WaitMode::RESULT_READY);
    
    Blob::Ptr attribsBlob = request.GetBlob(outputName);

    auto numOfChannels = attribsBlob->getTensorDesc().getDims().at(1);
    /* output descriptor of Person Reidentification Recognition network has size 256 */
    if (numOfChannels != 256) {
        throw std::logic_error("Output size (" + std::to_string(numOfChannels) + ") "
                                                                                 "of the Person Reidentification network is not equal to 256");
    }
    
    auto outputValues = attribsBlob->buffer().as<float*>();
    std::cout << "Identify finished" << std::endl;
    return std::vector<float>(outputValues, outputValues + 256);
}
