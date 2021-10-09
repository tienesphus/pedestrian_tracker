# Time spent in Region of Interest Feature
To configure the region of interest
```
./pedestrian_tracker -m_det '<path_to_model>' -m_reid 'path_to_model>' -i '<path_to_video>' -reconfig 'roi'
```
To use the feature
```
./pedestrian_tracker -m_det '<path_to_model>' -m_reid 'path_to_model>' -i '<path_to_video>' -out_a "<path_to_file>"
```

## Implementation Details
This feature utils tracking from the main system. It checks pedestrains who are being **tracked** only, not all detected pedestrains. In any given frame, the system will check whether the tracked pedestrains are in the region of interest (defined by users). To check whether the pedestrains are in the ROI, opencv's [pointPolygonTest()](https://docs.opencv.org/3.4.14/d3/dc0/group__imgproc__shape.html#ga1a539e8db2135af2566103705d7a5722) function. Real time is being used for the calculation of time spent in that region of interest. System would log every 100th frame to the file given by `-out_a` flag. 

## Configuring the ROI
To configuring this feature is straightforward. use `-reconfig 'roi'`, then pick the 4 points (Similar to [\[Camera-Config\]](https://github.com/tienesphus/pedestrian_tracker/blob/streamDev/docs/camera-config.md)).

## Finding from testing
1. After testing, we have found that if the tracking system fails to track when people are out of the region of interest, The log will not be produced at all. 
   - To minimise this issue, we suggest that lowering the `min_track_duration` in `tracker_.cpp`, so that the system would track pedestrains faster.
2. An unstable frame rate may also affect the logs since the underlying calculation is using real time. 
   - unstable frame can occur when there are too many pedestrains on any given frame.
3. depend heavily on the tracker system, if the tracker fails, incorrect logs are produced.
   - improving the tracking would also improve this feature.
