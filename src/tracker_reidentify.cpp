#include "tracker_reidentify.hpp"

#include <utility>
#include <iostream>

#include <opencv2/imgproc.hpp>


//  ----------- TRACK ---------------

class TrackRI {
public:
    /**
     * Initialises a Track
     * @param box the bounding box of where the track is
     * @param conf the confidence that the track still exists
     * @param index a unique index for the track
     */
    TrackRI(cv::Rect box, float conf, int index, std::vector<float> features);

    /**
     * Updates the status of this Track. Updates the world count.
     */
    bool update(const WorldConfig &config, WorldState &world);

    /**
     * Draws the Track onto the given image
     */
    void draw(cv::Mat &img) const;

    cv::Rect box;
    float confidence;
    int index;
    std::vector<float> features;

    bool been_inside = false;
    bool been_outside = false;
    bool counted_in = false;
    bool counted_out = false;

    int8_t color; // the hue of the path
    std::vector<cv::Point> path;

};

TrackRI::TrackRI(cv::Rect box, float conf, int index, std::vector<float> features):
          box(std::move(box)),
          confidence(conf),
          index(index),
          features(std::move(features)),
          color(rand() % 256)
{}

bool TrackRI::update(const WorldConfig &config, WorldState &world)
{
    // look only at the boxes center point
    int x = box.x + box.width/2;
    int y = box.y + box.height/4;
    cv::Point p(x, y);

    if (config.in_bounds(p))
    {
        // Count the people based on which direction they pass
        if (config.inside.side(p))
        {
            been_inside = true;
            if (been_outside && !counted_in && !counted_out)
            {
                world.in_count++;
                counted_in = true;
            }

            if (been_outside && counted_out)
            {
                world.out_count--;
                counted_out = false;
                been_outside = false;
            }
        }      

        if (config.outside.side(p))
        {
            been_outside = true;
            if (been_inside && !counted_out && !counted_in)
            {
                world.out_count++;
                counted_out = true;
            }

            if (been_inside && counted_in)
            {
                world.in_count--;
                counted_in = false;
                been_inside = false;
            }
        }
    }

    path.push_back(p);

    // decrease the confidence (will increase when merging with a detection)
    confidence -= 0.01;
    return confidence > 0.2;
}

void TrackRI::draw(cv::Mat &img) const {
    cv::Scalar clr = hsv2rgb(cv::Scalar(color, 255, confidence*255));
    cv::rectangle(img, box, clr, 1);
    
    int x = box.x + box.width/2;
    int y = box.y + box.height/4;
    cv::Point p(x, y);
    cv::circle(img, p, 3, clr, 2);
    
    std::string txt = std::to_string(index);
    cv::Point txt_loc(box.x+5, box.y+15);
    cv::putText(img, txt, txt_loc, cv::FONT_HERSHEY_SIMPLEX, 0.5, clr, 2);

    cv::Point last = path.front();
    for (const cv::Point& pt : path) {
        cv::line(img, pt, last, clr);
        last = pt;
    }
}


//  -----------  TRACKER ---------------

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


Tracker_RI::Tracker_RI(const NetConfig& netConfig, WorldConfig world, InferenceEngine::InferencePlugin &plugin):
    netConfig(netConfig), worldConfig(std::move(world)), state(WorldState(0, 0)), index_count(0)
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

Tracker_RI::~Tracker_RI() {

}

std::vector<float> Tracker_RI::identify(const cv::Mat &person) {
    using namespace InferenceEngine;

    InferRequest request = network.CreateInferRequest();
    Blob::Ptr inputBlob = wrapMat2Blob(person.clone()); // clone is needed so the Mat is dense
    request.SetBlob(inputName, inputBlob);

    request.StartAsync();
    request.Wait(IInferRequest::WaitMode::RESULT_READY);

    Blob::Ptr attribsBlob = request.GetBlob(outputName);

    auto numOfChannels = attribsBlob->getTensorDesc().getDims().at(1);
    /* output descriptor of Person Reidentification Recognition network has size 256 */
    if (numOfChannels != 256) {
        throw std::logic_error("Output size (" + std::to_string(numOfChannels) + ") of the "
                                                                                 "Person Reidentification network is not equal to 256");
    }

    auto outputValues = attribsBlob->buffer().as<float*>();
    return std::vector<float>(outputValues, outputValues + 256);
}

WorldState Tracker_RI::process(const Detections &detections, const cv::Mat& frame)
{
    merge(detections, frame);
    update();

    // Note: World State is purposely copied out so it is a snapshot
    return state;
}

/**
 * Looks for detections that are near current tracks. If there is something
 * close by, then the detection will be merged into it.
 */
void Tracker_RI::merge(const Detections &detection_results, const cv::Mat& frame)
{

    std::cout << "MERGING" << std::endl;


    struct DetectionExtra {
        const Detection detection;
        std::vector<float> features;
        bool merged;
        int index;

        DetectionExtra(Detection d, std::vector<float> features, size_t index):
            detection(std::move(d)), features(std::move(features)), merged(false), index(index)
        {}
    };

    struct MergeOption {
        float confidence;
        TrackRI * track;
        DetectionExtra * extra;

        MergeOption(float conf, TrackRI* track, DetectionExtra* detect):
            confidence(conf), track(track), extra(detect)
        {}
    };

    std::cout << "Calc features" << std::endl;
    std::vector<DetectionExtra> detections;
    int index = 0;
    for (Detection detection : detection_results.get_detections()) {

        // Get the track/detection merge confidence
        // Note: attempting to crop outside of Mat bounds crashes program, thus,
        // take the intersection of the detection box and the full frame to ensure that never happens
        cv::Mat crop = frame(intersection(detection.box, cv::Rect(0, 0, frame.cols, frame.rows)));

        // Add the extra detection info
        detections.emplace_back(detection, identify(crop), index++);
    }


    std::cout << "Calculating differences" << std::endl;

    std::vector<MergeOption> merges;

    for (DetectionExtra &extra : detections) {
        for (std::unique_ptr<TrackRI> &track : this->tracks) {

            float confidence = cosine_similarity(extra.features, track->features);
            std::cout << " d" << extra.index << " x t" << track->index << ": " << confidence << std::endl;
            merges.emplace_back(confidence, track.get(), &extra);

        }
    }

    std::cout << "Sorting differences" << std::endl;
    std::sort(merges.begin(), merges.end(),
            [](const MergeOption &a, const MergeOption &b) -> auto { return a.confidence > b.confidence; }
    );

    std::cout << "Sorted Options: " << std::endl;
    for (const MergeOption &m : merges) {
        std::cout << " d" << m.extra->index << " x t" << m.track->index << ": " << m.confidence << std::endl;
    }

    std::cout << "Finding top merges" << std::endl;

    // iteratively merge the closest of the detection/track combinations
    for (size_t i = 0; i < merges.size(); i++) {
        MergeOption& merge = merges[i];

        // threshold box's that are too far away
        if (merge.confidence < netConfig.thresh) {
            std::cout << " Differences too high" << std::endl;
            break;
        }

        const Detection& d = merge.extra->detection;
        TrackRI* track = merge.track;

        std::cout << "Merging d" << merge.extra->index <<  " and t" << track->index << std::endl;

        // Merge the data
        track->box = d.box;
        track->confidence = std::max(d.confidence, track->confidence);
        track->features = std::move(merge.extra->features);

        // delete all values in the map with the same track
        // (so we do not merge them twice accidentally)
        merge.extra->merged = true;
        for (size_t j = i+1; j < merges.size(); j++) {
            const MergeOption& merge2 = merges[j];
            if (merge2.track == track || merge2.extra->merged) {
                std::cout << " Ignore option " << j << std::endl;
                merges.erase(merges.begin()+j);
                j--;
            }
        }
    }

    std::cout << "Adding new tracks" << std::endl;
    // Make new tracks for detections that have not merged
    for (const DetectionExtra &extra : detections) {
        bool dealt_with = extra.merged;
        if (!dealt_with) {
            const Detection &d = extra.detection;
            std::cout << "Making a new box for detection " << d.box << std::endl;

            this->tracks.push_back(std::make_unique<TrackRI>(d.box, d.confidence, index_count++, extra.features));
        }
    }
    
    // Delete tracks with very high overlap
    // This occurs when two detections are produced for the same person
    std::cout << "Deleting similar tracks " << std::endl;
    for (size_t i = 0; i < this->tracks.size(); i++) {
        std::unique_ptr<TrackRI> &track_i = this->tracks[i];
        for (size_t j = i+1; j < this->tracks.size(); j++) {
            std::unique_ptr<TrackRI> &track_j = this->tracks[j];
            
            if (IoU(track_i->box, track_j->box) > 0.95 &&
                    cosine_similarity(track_i->features, track_j->features) > netConfig.thresh) {
                if (track_i->confidence > track_j->confidence) {
                    std::cout << "Deleting j: " << j << std::endl;
                    this->tracks.erase(this->tracks.begin()+j);
                    --j;
                } else {
                    std::cout << "Deleting i: " << i << std::endl;
                    this->tracks.erase(this->tracks.begin()+i);
                    --i;
                    break;
                }
            }
        }
    }
}

/**
 * Updates the status of each Track. Updates the world count.
 * Deletes old tracks.
 */
void Tracker_RI::update()
{
    this->tracks.erase(std::remove_if(std::begin(this->tracks), std::end(this->tracks),
            [this](std::unique_ptr<TrackRI>& t) {
                return !t->update(this->worldConfig, this->state);
            }
            ), std::end(this->tracks));
}

void Tracker_RI::draw(cv::Mat& img) const
{
    for (const std::unique_ptr<TrackRI>& track : tracks)
        track->draw(img);
}

